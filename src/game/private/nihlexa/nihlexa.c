// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "qmos/public/qm_os_string.h"

#include "nihlexa.h"
#include "nihlexa_menu.h"

#include "game_entity.h"

#include "integrations/integrations.h"
#include "physics/physics.h"
#include "game_team.h"

#include "components/component_movement.h"
#include "components/component_camera.h"

#include "entities/entity_player_spawn.h"
#include "entities/qm1/qm1_entity_player.h"

NihServerState nih_serverState_;

void nih_actions_register_();

NihGameMode nih_get_game_mode()
{
	PL_GET_CVAR( "game.mode", gameMode );
	if ( gameMode->i_value < 0 || gameMode->i_value >= NIH_GAME_MODE_MAX )
	{
		return NIH_GAME_MODE_SP;
	}

	return gameMode->i_value;
}

static void damage_player_command( [[maybe_unused]] const unsigned int argc, char **argv )
{
	int16_t value;
	if ( argc > 1 )
	{
		value = ( int16_t ) atoi( argv[ 1 ] );
	}
	else
	{
		value = 10;
	}

	//TODO: pump the event...
}

static void print_camera_pos_command( unsigned int argc, char **argv )
{
	GamePlayer *player = game_server_get_local_player_();
	if ( player == nullptr || player->camera == nullptr )
	{
		game_print_( "No valid camera.\n" );
		return;
	}

	char tmp[ 64 ];

	QmMathVector3f cameraPos = ape_camera_get_position( player->camera );
	game_print_( "Camera Pos: %s\n", qm_math_vector3f_print( cameraPos, tmp, sizeof( tmp ) ) );
	QmMathVector3f cameraAngles = ape_camera_get_angles( player->camera );
	game_print_( "Camera Ang: %s\n", qm_math_vector3f_print( cameraAngles, tmp, sizeof( tmp ) ) );
}

static void camera_save_pos_command( [[maybe_unused]] unsigned int argc, [[maybe_unused]] char **argv )
{
	GamePlayer *player = game_server_get_local_player_();
	if ( player == nullptr || player->camera == nullptr )
	{
		game_print_( "No valid camera.\n" );
		return;
	}

	QmMathVector3f pos = ape_camera_get_position( player->camera );
	QmMathVector3f ang = ape_camera_get_angles( player->camera );

	char *path = qm_os_string_alloc( "%s/camera.dat", com_get_app_data_directory() );
	FILE *file = fopen( path, "w" );
	if ( file != nullptr )
	{
		fprintf( file, "%f %f %f %f %f %f",
		         pos.x, pos.y, pos.z,
		         ang.x, ang.y, ang.z );
		fclose( file );
	}
	else
	{
		game_warning_( "Failed to create camera file (%s)!\n", path );
	}

	qm_os_memory_free( path );
}

static void camera_restore_pos_command( [[maybe_unused]] unsigned int argc, [[maybe_unused]] char **argv )
{
	GamePlayer *player = game_server_get_local_player_();
	if ( player == nullptr || player->camera == nullptr )
	{
		game_print_( "No valid camera.\n" );
		return;
	}

	char *path = qm_os_string_alloc( "%s/camera.dat", com_get_app_data_directory() );
	FILE *file = fopen( path, "r" );
	if ( file != nullptr )
	{
		QmMathVector3f pos = {};
		QmMathVector3f ang = {};

		fscanf( file, "%f %f %f %f %f %f",
		        &pos.x, &pos.y, &pos.z,
		        &ang.x, &ang.y, &ang.z );

		ape_camera_set_position( player->camera, &pos );
		ape_camera_set_angles( player->camera, &ang );
	}
	else
	{
		game_warning_( "Failed to open camera file (%s)!\n", path );
	}

	qm_os_memory_free( path );
}

static void register_entities()
{
	extern ApeEntityClassDefinition ss1_pawnEntityClass;
	extern ApeEntityClassDefinition ss1_playerEntityClass;

	ape_register_entity_class( &ss1_pawnEntityClass );
	ape_register_entity_class( &ss1_playerEntityClass );
}

static bool nih_initialize()
{
	if ( !game_initialize_() )
	{
		return false;
	}

	nih_serverState_ = ( NihServerState ) {};

	static constexpr int64_t DISCORD_CLIENT_ID = 822170320169074719;
	game_integrations_discord_initialize_( DISCORD_CLIENT_ID );
	game_integrations_discord_update_activity_( G_STR_( "Idling" ), nullptr, "qm1-logo", "Temp" );

	nih_actions_register_();

	register_entities();

	PlRegisterConsoleCommand( "nih_damage_player", "Damage the player by a specific amount.", -1, damage_player_command );
	PlRegisterConsoleCommand( "nih_print_camera_pos", "Print the camera position and angles.", 0, print_camera_pos_command );
	PlRegisterConsoleCommand( "nih_camera_save_pos", "Save the current camera position.", 0, camera_save_pos_command );
	PlRegisterConsoleCommand( "nih_camera_restore_pos", "Restore the camera position.", 0, camera_restore_pos_command );

	nih_serverState_.config        = com_get_config( NIH_GAME_CONFIG );
	nih_serverState_.isFirstLaunch = acm_get_bool( nih_serverState_.config, "isFirstLaunch", true );

	nih_menu_initialize_();

	return true;
}

static void serialize_config()
{
	acm_set_variable( nih_serverState_.config, "isFirstLaunch", nih_serverState_.isFirstLaunch ? "true" : "false", ACM_PROPERTY_TYPE_BOOL, true );

	com_write_config( nih_serverState_.config, NIH_GAME_CONFIG );
}

static void ss1_shutdown()
{
	//TODO: need mechanism for removing components

	serialize_config();

	if ( nih_serverState_.world != nullptr )
	{
		ape_world_node_destroy( ( ApeWorldNode * ) nih_serverState_.world );
		nih_serverState_.world = nullptr;
	}

	nih_menu_shutdown_();

	game_shutdown_();
}

static void world_tick( const double delta )
{
	ApeWorld *world = game_get_current_world();
	if ( world == nullptr )
	{
		return;
	}

	ape_audio_clear_listener();

	GamePlayer *player = game_server_get_local_player_();
	if ( player != nullptr && player->camera != nullptr )
	{
		QmMathVector3f cpos = ape_camera_get_position( player->camera );
		QmMathVector3f cang = ape_camera_get_angles( player->camera );
		ape_audio_update_listener( &cpos, &cang, nullptr );
	}
}

static void ss1_tick( double delta )
{
	delta = game_get_delta_mod_( delta );

	game_server_tick_( delta );

	world_tick( delta );
}

static void on_destroy_room( ApeRoom *room )
{
}

static bool server_client_validate( ApeServerClient *clientHandle )
{
	return game_server_client_validate_( clientHandle );
}

static void spawn_player( GamePlayer *player )
{
	QmOsLinkedList *playerSpawns = game_player_spawn_get_spawn_points();
	if ( playerSpawns == nullptr )
	{
		game_warning_( "Unable to spawn player, no spawn points!\n" );
		return;
	}

	ApeEntity *entity = nullptr;

	NihGameMode mode = nih_get_game_mode();
	if ( mode == NIH_GAME_MODE_SP )
	{
		QmOsLinkedListNode *node = qm_os_linked_list_get_front( playerSpawns );
		assert( node != nullptr );

		entity = qm_os_linked_list_node_get_data( node );
		assert( entity != nullptr );
	}
	else
	{
		// pick a randomised spot
		QM_OS_LINKED_LIST_ITERATE( entity, playerSpawns, i )
		{
			GamePlayerSpawnEntity *spawnEntity = PLAYER_SPAWN_ENTITY( entity );
			if ( spawnEntity->team != player->team )
			{
				continue;
			}

			//TODO: check if it's occupied or not...

			break;
		}
	}

	if ( entity == nullptr )
	{
		game_warning_( "Failed to find player spawn!\n" );
		return;
	}

	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( entity ) );
	if ( room == nullptr )
	{
		game_warning_( "Encountered a player spawn without a room!\n" );
		return;
	}

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );
	QmMathVector3f ang = ape_world_node_get_angles( APE_WORLD_NODE( entity ) );

	player->entity = ape_entity_create( APE_WORLD_NODE( room ), NIH_PLAYER_CLASS_NAME, "local_player", nullptr, &pos, &ang );
	if ( player->entity == nullptr )
	{
		game_warning_( "Failed to spawn in player!\n" );
		return;
	}

	ape_entity_spawn( player->entity );

	game_entity_place_on_ground( player->entity );
}

static void server_client_connected( ApeServerClient *clientHandle )
{
	// now attempt to spawn the player into the room
	GameServerClient *gameClient = game_server_client_connected_( clientHandle );
	if ( gameClient == nullptr )
	{
		game_print_( "Received invalid client handle after connect!\n" );
		return;
	}

	if ( gameClient->playerSlot != nullptr )
	{
		spawn_player( gameClient->playerSlot );
	}
}

static void server_client_disconnected( ApeServerClient *clientHandle )
{
	game_server_client_disconnected_( clientHandle );
}

static void server_process_message( ApeServerClient *clientHandle, const void *buf, size_t bufSize )
{
	game_server_process_message_( clientHandle, buf, bufSize );
}

static void client_process_message( const void *buf, size_t bufSize )
{
	game_client_process_message_( buf, bufSize );
}

void nih_client_connected_();
void nih_client_tick_( double delta );
void nih_client_draw_( const ApeViewport *viewport );
void nih_client_draw_ui_( const ApeViewport *viewport );

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	static ApeGameInterfaceImport gameMode = {
	        .version         = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion = NIH_GAME_PROTOCOL_VERSION,
	        .identifier      = "nih",

	        .initialize = nih_initialize,
	        .shutdown   = ss1_shutdown,
	        .draw       = nih_client_draw_,
	        .drawUI     = nih_client_draw_ui_,

	        .spawnWorld = nih_world_spawn_,

	        .onDestroyRoom = on_destroy_room,

	        .serverClientValidate     = server_client_validate,
	        .serverClientConnected    = server_client_connected,
	        .serverClientDisconnected = server_client_disconnected,
	        .serverProcessMessage     = server_process_message,
	        .serverTick               = ss1_tick,

	        .clientConnected      = nih_client_connected_,
	        .clientProcessMessage = client_process_message,
	        .clientTick           = nih_client_tick_,
	};

	return &gameMode;
}
