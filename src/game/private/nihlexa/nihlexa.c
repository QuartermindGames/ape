// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "qmos/public/qm_os_string.h"

#include "nihlexa.h"
#include "nihlexa_menu.h"

#include "integrations/integrations.h"
#include "physics/physics.h"
#include "game_team.h"

#include "components/component_movement.h"
#include "components/component_camera.h"

#include "entities/entity_player_spawn.h"
#include "entities/qm1/qm1_entity_player.h"

NihGameState qm1_state_;

#define SS1_CONFIG "game_ss1"

void ss1_actions_register_();

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
	if ( qm1_state_.camera == nullptr )
	{
		game_print_( "No valid camera.\n" );
		return;
	}

	char tmp[ 64 ];

	QmMathVector3f cameraPos = ape_camera_get_position( qm1_state_.camera );
	game_print_( "Camera Pos: %s\n", qm_math_vector3f_print( cameraPos, tmp, sizeof( tmp ) ) );
	QmMathVector3f cameraAngles = ape_camera_get_angles( qm1_state_.camera );
	game_print_( "Camera Ang: %s\n", qm_math_vector3f_print( cameraAngles, tmp, sizeof( tmp ) ) );
}

static void camera_save_pos_command( [[maybe_unused]] unsigned int argc, [[maybe_unused]] char **argv )
{
	if ( qm1_state_.camera == nullptr )
	{
		return;
	}

	QmMathVector3f pos = ape_camera_get_position( qm1_state_.camera );
	QmMathVector3f ang = ape_camera_get_angles( qm1_state_.camera );

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
	if ( qm1_state_.camera == nullptr )
	{
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

		ape_camera_set_position( qm1_state_.camera, &pos );
		ape_camera_set_angles( qm1_state_.camera, &ang );
	}
	else
	{
		game_warning_( "Failed to open camera file (%s)!\n", path );
	}

	qm_os_memory_free( path );
}

static void register_entities()
{
	extern ApeEntityClassDefinition ss1_airshipEntityClass;
	ape_register_entity_class( &ss1_airshipEntityClass );

	extern ApeEntityClassDefinition ss1_pawnEntityClass;
	ape_register_entity_class( &ss1_pawnEntityClass );

	extern ApeEntityClassDefinition ss1_playerEntityClass;
	ape_register_entity_class( &ss1_playerEntityClass );
}

static bool ss1_initialize()
{
	if ( !game_initialize_() )
	{
		return false;
	}

	QM_OS_ZERO_( qm1_state_ );

	ss1_actions_register_();

	register_entities();

	PlRegisterConsoleCommand( "qm1_damage_player", "Damage the player by a specific amount.", -1, damage_player_command );
	PlRegisterConsoleCommand( "qm1_print_camera_pos", "Print the camera position and angles.", 0, print_camera_pos_command );
	PlRegisterConsoleCommand( "qm1_camera_save_pos", "Save the current camera position.", 0, camera_save_pos_command );
	PlRegisterConsoleCommand( "qm1_camera_restore_pos", "Restore the camera position.", 0, camera_restore_pos_command );

	qm1_state_.config        = com_get_config( SS1_CONFIG );
	qm1_state_.isFirstLaunch = acm_get_bool( qm1_state_.config, "isFirstLaunch", true );

	nih_menu_initialize_();

	if ( !qm1_world_setup_() )
	{
		return false;
	}

	return true;
}

static void serialize_config()
{
	acm_set_variable( qm1_state_.config, "isFirstLaunch", qm1_state_.isFirstLaunch ? "true" : "false", ACM_PROPERTY_TYPE_BOOL, true );

	com_write_config( qm1_state_.config, SS1_CONFIG );
}

static void ss1_shutdown()
{
	//TODO: need mechanism for removing components

	serialize_config();

	if ( qm1_state_.camera != nullptr )
	{
		ape_world_node_destroy( ( ApeWorldNode * ) qm1_state_.camera );
		qm1_state_.camera = nullptr;
	}

	if ( qm1_state_.world != nullptr )
	{
		ape_world_node_destroy( ( ApeWorldNode * ) qm1_state_.world );
		qm1_state_.world = nullptr;
	}

	nih_menu_shutdown_();

	game_shutdown_();
}

static void handle_input( double delta )
{
	ApeEntity *entity = game_server_get_host_entity_();
	if ( entity == nullptr )
	{
		return;
	}

	GameCameraComponent *cameraComponent = ape_entity_get_component( entity, GAME_CAMERA_COMPONENT_NAME );
	if ( cameraComponent != nullptr )
	{
		game_component_camera_handle_input_( cameraComponent, delta );
	}
}

static void world_tick( const double delta )
{
	ApeWorld *world = game_get_current_world();
	if ( world == nullptr )
	{
		return;
	}

	ape_audio_clear_listener();

	ApeCamera *camera = qm1_state_.camera;
	if ( camera != nullptr )
	{
		QmMathVector3f cpos = ape_camera_get_position( camera );
		QmMathVector3f cang = ape_camera_get_angles( camera );

		qm1_state_.oldCameraPosition = cpos;

		// this is utterly dumb, but we'll use this to determine a vague "velocity"
		QmMathVector3f cdiff = qm_math_vector3f_sub( cpos, qm1_state_.oldCameraPosition );

		ape_camera_set_angles( camera, &cang );

		ape_audio_update_listener( &cpos, &cang, &cdiff );
	}
}

static void ss1_tick( double delta )
{
	delta = game_get_delta_mod_( delta );

	game_server_tick_( delta );

	world_tick( delta );

#if 0
	if ( qm1_state_.camera != nullptr )
	{
		QmMathVector3f cameraPos = ape_camera_get_position( qm1_state_.camera );
		game_test_cylinder_point_collision_( &cameraPos );
		game_test_cylinder_polygon_collision_( &cameraPos );
	}
#endif
}

static void ss1_draw( const ApeViewport *viewport )
{
	ape_camera_make_active( qm1_state_.camera );

	if ( qm1_state_.camera == nullptr )
	{
		return;
	}

	ape_camera_draw_perspective( qm1_state_.camera, viewport );
}

static void ss1_draw_menu( const ApeViewport *viewport )
{
	nih_menu_draw( viewport );
}

static void on_destroy_room( ApeRoom *room )
{
	if ( qm1_state_.camera == nullptr )
	{
		return;
	}

	ApeWorldNode *worldNode = ape_world_node_get_parent( APE_WORLD_NODE( qm1_state_.camera ) );
	if ( worldNode == APE_WORLD_NODE( room ) )
	{
		// save the camera from being destroyed!
		ape_world_node_dettach( APE_WORLD_NODE( qm1_state_.camera ) );
	}
}

static bool server_client_validate( ApeServerClient *clientHandle )
{
	return game_server_client_validate_( clientHandle );
}

static void server_client_connected( ApeServerClient *clientHandle )
{
	game_server_client_connected_( clientHandle );
}

static void server_client_disconnected( ApeServerClient *clientHandle )
{
	game_server_client_disconnected_( clientHandle );
}

static void server_process_message( ApeServerClient *clientHandle, const void *buf, size_t bufSize )
{
	game_server_process_message_( clientHandle, buf, bufSize );
}

static void client_connect()
{
}

static void client_process_message( const void *buf, size_t bufSize )
{
	game_client_process_message_( buf, bufSize );
}

static void client_tick( double delta )
{
	nih_menu_tick( delta );

	handle_input( delta );

	game_integrations_discord_tick_();
}

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	static ApeGameInterfaceImport gameMode = {
	        .version         = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion = NIH_GAME_PROTOCOL_VERSION,
	        .identifier      = "ss1",

	        .initialize = ss1_initialize,
	        .shutdown   = ss1_shutdown,
	        .draw       = ss1_draw,
	        .drawUI     = ss1_draw_menu,

	        .spawnWorld = qm1_world_spawn_,

	        .onDestroyRoom = on_destroy_room,

	        .serverClientValidate     = server_client_validate,
	        .serverClientConnected    = server_client_connected,
	        .serverClientDisconnected = server_client_disconnected,
	        .serverProcessMessage     = server_process_message,
	        .serverTick               = ss1_tick,

	        .clientConnect        = client_connect,
	        .clientProcessMessage = client_process_message,
	        .clientTick           = client_tick,
	};

	return &gameMode;
}
