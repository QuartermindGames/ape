// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "yin/node.h"

#include "pm_game.h"

#include "menu/fw_menu.h"

PMGameState pm_gameState;

#define PM_CONFIG "pm_user"

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

static void generate_world_command( unsigned int argc, char **argv )
{
	if ( pm_gameState.world != nullptr )
	{
		ape_world_node_destroy( ape_world_get_world_node( pm_gameState.world ) );
	}

	pm_gameState.world = ape_create_world();

	ApeRoom *room = ape_create_room( pm_gameState.world->header.node );
	pm_gameState.terrain = ape_create_brush( ape_room_get_world_node( room ), "pm_terrainBrushClass", &pl_vecOrigin3, &pl_vecOrigin3 );

	//HACK: this should be created in the level instead!!
	sun = ape_create_light( ape_room_get_world_node( room ), &PLVector3( -2.0f, -2.0f, 1.0f ), &PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.85f ), 0.0f, APE_LIGHT_TYPE_SUN, SS_ARL_LIGHT_FLAG_ENABLED | SS_ARL_LIGHT_FLAG_DYNAMIC | SS_ARL_LIGHT_FLAG_RUNTIME_SHADOWS );
}

extern ApeBrushClass pm_terrainBrushClass;
static bool pm_initialize( void )
{
	PL_ZERO_( pm_gameState );

	ss_game_register_standard_entity_components_();

	ape_register_brush_class( &pm_terrainBrushClass );

	PlRegisterConsoleCommand( "sun", "Set the global sun position.", 3, sun_command );
	PlRegisterConsoleCommand( "sun_colour", "Sets the global sun colour.", 4, sun_colour_command );
	PlRegisterConsoleCommand( "generate_world", "Generate a new world.", 0, generate_world_command );

	pm_menu_initialize();

	pm_gameState.config = com_get_config( PM_CONFIG );

	// determine if it's the first time we've launched
	const char *name = nd_branch_get_child_string( pm_gameState.config, "name", nullptr );
	if ( name != nullptr )
	{
		snprintf( pm_gameState.players[ 0 ].name, sizeof( pm_gameState.players[ 0 ].name ), "%s", name );
	}
	else
	{
		pm_gameState.isFirstLaunch = true;
	}

	pm_gameState.camera = ape_create_camera( nullptr, &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
	if ( pm_gameState.camera == nullptr )
	{
		Game_Error( "Failed to create player camera!\n" );
		return false;
	}

	return true;
}

static bool pm_shutdown( void )
{
	//TODO: need mechanism for removing components

	com_write_config( pm_gameState.config, PM_CONFIG );

	if ( pm_gameState.camera != nullptr )
	{
		ape_world_node_destroy( ape_camera_get_world_node( pm_gameState.camera ) );
		pm_gameState.camera = nullptr;
	}

	if ( pm_gameState.world != nullptr )
	{
		ape_world_node_destroy( ape_world_get_world_node( pm_gameState.world ) );
		pm_gameState.world = nullptr;
	}

	return true;
}

static void handle_input( void )
{
	PLVector3 ang = ape_camera_get_angles( pm_gameState.camera );
	PLVector3 pos = ape_camera_get_position( pm_gameState.camera );

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

	ape_camera_set_position( pm_gameState.camera, &pos );
	ape_camera_set_angles( pm_gameState.camera, &ang );
}

static bool pm_tick( void )
{
	pm_menu_handle_input();
	pm_menu_tick();

	handle_input();

	world_simulation_tick( &pm_gameState.simulation );

	return true;
}

static bool pm_draw( ApeViewport *viewport )
{
	ape_camera_make_active( pm_gameState.camera );
	ape_camera_draw_perspective( pm_gameState.camera, viewport );
	return true;
}

static bool pm_draw_menu( const ApeViewport *viewport )
{
	pm_menu_draw( viewport );
	return true;
}

static bool pm_spawn_world( ApeWorld *world )
{
	world_simulation_initialize( &pm_gameState.simulation );

	ape_world_node_attach( ape_camera_get_world_node( pm_gameState.camera ),
	                       world->header.node );

	ApeWorldNode *worldNode;
	// attempt to fetch the terrain
	if ( ( worldNode = ape_world_node_get_child_by_name( world->root, "terrain" ) ) != nullptr )
	{
		pm_gameState.terrain = ape_world_node_get_brush_data( worldNode );
		if ( pm_gameState.terrain == nullptr )
		{
			Game_Warning( "Terrain node is not a valid brush!\n" );
		}
	}
	if ( ( worldNode = ape_world_node_get_child_by_name( world->root, "sun" ) ) != nullptr )
	{
		sun = ape_world_node_get_light_data( worldNode );
		if ( sun == nullptr )
		{
			Game_Warning( "Sun node is not a valid light!\n" );
		}
	}

	return true;
}

static bool request_handler( ApeGameInterfaceRequest gameModeRequest, void *user )
{
	switch ( gameModeRequest )
	{
		case APE_GAME_INTERFACE_REQUEST_INITIALIZE:
			return pm_initialize();
		case APE_GAME_INTERFACE_REQUEST_SHUTDOWN:
			return pm_shutdown();
		case APE_GAME_INTERFACE_REQUEST_DRAW:
			return pm_draw( ( ApeViewport * ) user );
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI:
			return pm_draw_menu( user );
		case APE_GAME_INTERFACE_REQUEST_TICK_SERVER:
			return pm_tick();
		case APE_GAME_INTERFACE_REQUEST_HANDLE_INPUT:
			break;
		case APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD:
			return pm_spawn_world( ( ApeWorld * ) user );
		default:
			break;
	}

	return false;
}

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	static ApeGameInterfaceImport gameMode = {
	        .version = APE_GAME_INTERFACE_VERSION,
	        .requestCallbackMethod = request_handler,
	};
	return &gameMode;
}
