// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "ss1_game.h"
#include "game/private/ss1/menu/menu.h"

#include "../shared/integrations/integrations.h"
#include "../shared/physics/physics.h"
#include "../shared/game_team.h"

SS1GameState ss1_gameState;

#define SS1_CONFIG "game_ss1"

const SS1Profession ss1_professions[ SS1_MAX_PROFESSIONS ] = {
        [SS1_PROFESSION_SHAMAN] = {
                                   .name        = "Shaman",
                                   .description = "Temp",
                                   },
        [SS1_PROFESSION_MACHINIST] = {
                                   .name        = "Machinist",
                                   .description = "Temp",
                                   },
        [SS1_PROFESSION_TRICKSTER] = {
                                   .name        = "Trickster",
                                   .description = "Temp",
                                   },
        [SS1_PROFESSION_POUNDER] = {
                                   .name        = "Pounder",
                                   .description = "Temp",
                                   },
};

extern ApeEntityClassDefinition ss1_airshipEntityClass;
extern ApeEntityClassDefinition ss1_pawnEntityClass;

static bool ss1_initialize()
{
	static constexpr int64_t DISCORD_CLIENT_ID = 822170320169074719;
	game_integrations_discord_initialize_( DISCORD_CLIENT_ID );
	game_integrations_discord_update_activity_( G_STR_( "Testing 123" ), G_STR_( "Hello World!" ), "ape_logo", "Blah!" );

	game_register_standard_entity_components_();

	ape_register_entity_class( &ss1_airshipEntityClass );
	ape_register_entity_class( &ss1_pawnEntityClass );

	PL_ZERO_( ss1_gameState );

#if !defined( NDEBUG )
	// validate all the professions are setup correctly
	for ( uint i = 0; i < SS1_MAX_PROFESSIONS; ++i )
	{
		assert( ss1_professions[ i ].name != nullptr && ss1_professions[ i ].description != nullptr );
	}
#endif

	ss1_gameState.config = com_get_config( SS1_CONFIG );

	ss1_menu_initialize();

	// determine if it's the first time we've launched
	const char *name = acm_get_string( ss1_gameState.config, "name", nullptr );
	if ( name != nullptr )
	{
		snprintf( ss1_gameState.players[ 0 ].name, sizeof( ss1_gameState.players[ 0 ].name ), "%s", name );
	}
	else
	{
		ss1_gameState.isFirstLaunch = true;
	}

	ss1_gameState.camera = ape_create_camera( nullptr, nullptr, &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
	if ( ss1_gameState.camera == nullptr )
	{
		game_error_( "Failed to create player camera!\n" );
		return false;
	}

	return true;
}

static bool ss1_shutdown( void )
{
	//TODO: need mechanism for removing components

	com_write_config( ss1_gameState.config, SS1_CONFIG );

	if ( ss1_gameState.camera != nullptr )
	{
		ape_world_node_destroy( ( ApeWorldNode * ) ss1_gameState.camera );
		ss1_gameState.camera = nullptr;
	}

	if ( ss1_gameState.world != nullptr )
	{
		ape_world_node_destroy( ( ApeWorldNode * ) ss1_gameState.world );
		ss1_gameState.world = nullptr;
	}

	ss1_menu_shutdown();

	game_integrations_discord_shutdown_();

	return true;
}

static void handle_input( void )
{
	PLVector3 ang = ape_camera_get_angles( ss1_gameState.camera );
	PLVector3 pos = ape_camera_get_position( ss1_gameState.camera );

	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value )
	{
		int mx, my;
		ape_client_input_get_mouse_delta( &mx, &my );
		ang.y += ( float ) mx;
		ang.x += ( float ) my;
		ang.x = PlClamp( -90.0f, ang.x, 90.0f );
	}

	PLVector2 rightStick = ape_client_input_get_controller_axis_state( 0, 1 );
	ang.x -= rightStick.y * 2.0f;
	ang.y -= rightStick.x * 2.0f;

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, nullptr, &forward );

	PLVector2 leftStick = ape_client_input_get_controller_axis_state( 0, 0 );
	pos                 = PlAddVector3( pos, PlScaleVector3F( forward, ( leftStick.y * 4 ) ) );
	pos                 = PlAddVector3( pos, PlScaleVector3F( left, ( leftStick.x * 4 ) ) );

	ape_camera_set_position( ss1_gameState.camera, &pos );
	ape_camera_set_angles( ss1_gameState.camera, &ang );
}

/**
 * This is a very convoluted way to set the pitch and yaw, but
 * unfortunately *this* idiot decided to make the sun a position
 */
static PLVector3 pitch_yaw_to_position( float pitch, float yaw )
{
	PLVector3 position = { 1.0f, pitch, 0.0f };
	PLMatrix4 matrix   = PlMatrix4Identity();
	PLMatrix4 m2;
	m2         = PlTranslateMatrix4( position );
	matrix     = PlMultiplyMatrix4( &m2, &matrix );
	m2         = PlRotateMatrix4( PL_DEG2RAD( yaw ), &( PLVector3 ) { 0.0f, 1.0f, 0.0f } );
	matrix     = PlMultiplyMatrix4( &m2, &matrix );
	position.x = matrix.m[ 0 ];
	position.z = matrix.m[ 8 ];
	return position;
}

static bool ss1_tick( void )
{
	ss1_menu_tick();

	handle_input();

	ApeWorld *world = ss_game_get_current_world();
	if ( world != nullptr )
	{
		world_simulation_tick( &ss1_gameState.simulation );
	}

	game_integrations_discord_tick_();

	return true;
}

static bool ss1_draw( ApeViewport *viewport )
{
	ape_camera_make_active( ss1_gameState.camera );
	ape_camera_draw_perspective( ss1_gameState.camera, viewport );
	return true;
}

static bool ss1_draw_menu( const ApeViewport *viewport )
{
	ss1_menu_draw( viewport );
	return true;
}

static void ss1_spawn_world( ApeRoom *room )
{
	game_world_simulation_initialize( &ss1_gameState.simulation );

	game_team_init( SS1_MAX_TEAMS );

	ApeWorldNode *roomNode = APE_WORLD_NODE( room );
	ape_world_node_attach( ( ApeWorldNode * ) ss1_gameState.camera, roomNode );

	//ApeAudioSample *sample = ape_audio_sample_cache( "sounds/water/water_waves_lapping_05.wav" );
	//ApeAudioSource *source = ape_audio_source_create( &PL_VECTOR3( 0.0f, 0.0f, 0.0f ), &PL_VECTOR3( 0.0f, 0.0f, 0.0f ), APE_AUDIO_SOURCE_GROUP_GENERIC );
	//ape_audio_source_emit( source, sample );

	ape_entity_create( roomNode, "ss1_airship", "airship_0", nullptr, &pl_vecOrigin3, &pl_vecOrigin3 );
	ape_entity_create( roomNode, "ss1_pawn", "player_0", nullptr, &PL_VECTOR3( 0.0f, 128.0f, 0.0f ), &pl_vecOrigin3 );
}

static bool request_handler( ApeGameInterfaceRequest gameModeRequest, void *user )
{
	switch ( gameModeRequest )
	{
		case APE_GAME_INTERFACE_REQUEST_INITIALIZE:
			return ss1_initialize();
		case APE_GAME_INTERFACE_REQUEST_SHUTDOWN:
			return ss1_shutdown();
		case APE_GAME_INTERFACE_REQUEST_DRAW:
			return ss1_draw( user );
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI:
			return ss1_draw_menu( user );
		case APE_GAME_INTERFACE_REQUEST_TICK_SERVER:
			return ss1_tick();
		default:
			break;
	}

	return false;
}

static void server_client_connected( ApeServerClientHandle *clientHandle )
{
	game_server_client_connected_( clientHandle );
}

static void server_client_disconnected( ApeServerClientHandle *clientHandle )
{
	game_server_client_disconnected_( clientHandle );
}

static void server_process_message( ApeServerClientHandle *clientHandle, const void *buf, size_t bufSize )
{
	game_server_process_message_( clientHandle, buf, bufSize );
}

static void client_process_message( const void *buf, size_t bufSize )
{
	game_client_process_message_( buf, bufSize );
}

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	static ApeGameInterfaceImport gameMode = {
	        .version               = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion       = SS1_GAME_PROTOCOL_VERSION + GAME_NET_PROTOCOL_VERSION,
	        .identifier            = "ss1",
	        .requestCallbackMethod = request_handler,
	        .spawnWorld            = ss1_spawn_world,

	        .serverClientConnected    = server_client_connected,
	        .serverClientDisconnected = server_client_disconnected,
	        .serverProcessMessage     = server_process_message,

	        .clientProcessMessage = client_process_message,
	};
	return &gameMode;
}
