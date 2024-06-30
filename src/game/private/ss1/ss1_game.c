// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "yin/node.h"

#include "ss1_game.h"

#include "game/private/ss1/menu/ss1_menu.h"

SS1GameState ss1_gameState;

#define SS1_CONFIG "pm_user"

const SS1Profession ss1_professions[ SS1_MAX_PROFESSIONS ] = {
        [SS1_PROFESSION_SHAMAN] = {
                                   .name = "Shaman",
                                   .description = "Temp",
                                   },
        [SS1_PROFESSION_MACHINIST] = {
                                   .name = "Machinist",
                                   .description = "Temp",
                                   },
        [SS1_PROFESSION_TRICKSTER] = {
                                   .name = "Trickster",
                                   .description = "Temp",
                                   },
        [SS1_PROFESSION_POUNDER] = {
                                   .name = "Pounder",
                                   .description = "Temp",
                                   },
};

static ApeLight *sun;

static void sun_command( unsigned int argc, char **argv )
{
	if ( sun == nullptr )
	{
		return;
	}

	float x = ( float ) atof( argv[ 1 ] );
	float y = ( float ) atof( argv[ 2 ] );
	float z = ( float ) atof( argv[ 3 ] );

	PLVector3 sunPos = { x, y, z };
	ape_light_set_position( sun, &sunPos );
}

static void sun_colour_command( unsigned int argc, char **argv )
{
	if ( sun == nullptr )
	{
		return;
	}

	float x = ( float ) atof( argv[ 1 ] );
	float y = ( float ) atof( argv[ 2 ] );
	float z = ( float ) atof( argv[ 3 ] );
	float w = ( float ) atof( argv[ 4 ] );

	ape_light_set_colour( sun, &PL_COLOURF32( x, y, z, w ) );
}

static bool ss1_initialize( void )
{
	PL_ZERO_( ss1_gameState );

	ss_game_register_standard_entity_components_();

	PlRegisterConsoleCommand( "sun", "Set the global sun position.", 3, sun_command );
	PlRegisterConsoleCommand( "sun_colour", "Sets the global sun colour.", 4, sun_colour_command );

#if !defined( NDEBUG )
	// validate all the professions are setup correctly
	for ( uint i = 0; i < SS1_MAX_PROFESSIONS; ++i )
	{
		assert( ss1_professions[ i ].name != nullptr );
	}
#endif

	ss1_menu_initialize();

	ss1_gameState.config = com_get_config( SS1_CONFIG );

	// determine if it's the first time we've launched
	const char *name = nd_branch_get_child_string( ss1_gameState.config, "name", nullptr );
	if ( name != nullptr )
	{
		snprintf( ss1_gameState.players[ 0 ].name, sizeof( ss1_gameState.players[ 0 ].name ), "%s", name );
	}
	else
	{
		ss1_gameState.isFirstLaunch = true;
	}

	ss1_gameState.camera = ape_create_camera( nullptr, &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
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
	PlAnglesAxes( ang, &left, NULL, &forward );

	PLVector2 leftStick = ape_client_input_get_controller_axis_state( 0, 0 );
	pos = PlSubtractVector3( pos, PlScaleVector3F( forward, leftStick.y ) );
	pos = PlSubtractVector3( pos, PlScaleVector3F( left, leftStick.x ) );

	ape_camera_set_position( ss1_gameState.camera, &pos );
	ape_camera_set_angles( ss1_gameState.camera, &ang );
}

static bool ss1_tick( void )
{
	ss1_menu_handle_input();
	ss1_menu_tick();

	handle_input();

	world_simulation_tick( &ss1_gameState.simulation );

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

static bool ss1_spawn_world( ApeWorld *world )
{
	world_simulation_initialize( &ss1_gameState.simulation );

	ape_world_node_attach( ( ApeWorldNode * ) ( ss1_gameState.camera ), &world->base );

	return true;
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
			return ss1_draw( ( ApeViewport * ) user );
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI:
			return ss1_draw_menu( user );
		case APE_GAME_INTERFACE_REQUEST_TICK_SERVER:
			return ss1_tick();
		case APE_GAME_INTERFACE_REQUEST_HANDLE_INPUT:
			break;
		case APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD:
			return ss1_spawn_world( ( ApeWorld * ) user );
		default:
			break;
	}

	return false;
}

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	static ApeGameInterfaceImport gameMode = {
	        .version = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion = SS1_GAME_PROTOCOL_VERSION + GAME_NET_PROTOCOL_VERSION,
	        .identifier = "ss1",
	        .requestCallbackMethod = request_handler,
	};
	return &gameMode;
}
