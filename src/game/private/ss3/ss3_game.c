// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: SS3 game implementation.
// Author:  Mark E. Sowden

#include "ss3_game.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

void ss3_ui_initialize( void );
void ss3_ui_shutdown( void );
void ss3_ui_tick( void );
bool ss3_ui_draw( ApeViewport *viewport );

static ApeCamera *playerCamera = NULL;
static ApeLight *testLight;

static void move_camera_action( ApeInputState state, const char *id )
{
	static const float MOVE_SPEED = 2.0f;
	static const float TURN_SPEED = 1.5f;

	if ( !( state & APE_INPUT_STATE_DOWN ) )
	{
		return;
	}

	PLVector3 pos = ape_camera_get_position( playerCamera );
	PLVector3 ang = ape_camera_get_angles( playerCamera );
	if ( strcmp( id, "ss3_turn_left" ) == 0 )
	{
		ang.y += TURN_SPEED;
	}
	else if ( strcmp( id, "ss3_turn_right" ) == 0 )
	{
		ang.y -= TURN_SPEED;
	}

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	if ( strcmp( id, "ss3_move_forward" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( forward, MOVE_SPEED ) );
	}
	else if ( strcmp( id, "ss3_move_backward" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( forward, MOVE_SPEED ) );
	}
	else if ( strcmp( id, "ss3_move_left" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( left, MOVE_SPEED ) );
	}
	else if ( strcmp( id, "ss3_move_right" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( left, MOVE_SPEED ) );
	}
	else if ( strcmp( id, "ss3_move_up" ) == 0 )
	{
		pos.y += 0.5f;
	}
	else if ( strcmp( id, "ss3_move_down" ) == 0 )
	{
		pos.y -= 0.5f;
	}

	ape_camera_set_position( playerCamera, &pos );
	ape_camera_set_angles( playerCamera, &ang );
}

static void sun_command( unsigned int argc, char **argv )
{
	if ( testLight == NULL )
	{
		return;
	}

	float x = ( float ) atof( argv[ 1 ] );
	float y = ( float ) atof( argv[ 2 ] );
	float z = ( float ) atof( argv[ 3 ] );

	PLVector3 sunPos = { x, y, z };
	ss_arl_light_set_position( testLight, &sunPos );
}

static void sun_colour_command( unsigned int argc, char **argv )
{
	if ( testLight == NULL )
	{
		return;
	}

	float x = ( float ) atof( argv[ 1 ] );
	float y = ( float ) atof( argv[ 2 ] );
	float z = ( float ) atof( argv[ 3 ] );
	float w = ( float ) atof( argv[ 4 ] );

	ss_arl_light_set_colour( testLight, &PL_COLOURF32( x, y, z, w ) );
}

static bool initialize( void )
{
	ss_game_register_standard_entity_components_();

	playerCamera = ape_camera_create( "ss3_camera_player", &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE );
	if ( playerCamera == NULL )
	{
		Game_Error( "Failed to create player camera!\n" );
		return false;
	}

	PlRegisterConsoleCommand( "ss3_sun", "Set the global sun position.", 3, sun_command );
	PlRegisterConsoleCommand( "ss3_sun_colour", "Sets the global sun colour.", 4, sun_colour_command );

	// movement actions
	ape_client_input_register_action( "ss3_move_forward", ( ApeInputButton[] ){ APE_INPUT_UP }, 1, ( ApeInputKey[] ){ APE_INPUT_KEY_UP, 'w' }, 2, move_camera_action );
	ape_client_input_register_action( "ss3_move_backward", ( ApeInputButton[] ){ APE_INPUT_DOWN }, 1, ( ApeInputKey[] ){ APE_INPUT_KEY_DOWN, 's' }, 2, move_camera_action );
	ape_client_input_register_action( "ss3_move_left", ( ApeInputButton[] ){ INPUT_LEFT }, 1, ( ApeInputKey[] ){ 'a' }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_move_right", ( ApeInputButton[] ){ INPUT_RIGHT }, 1, ( ApeInputKey[] ){ 'd' }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_move_down", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_move_up", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_turn_left", NULL, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_LEFT }, 1, move_camera_action );
	ape_client_input_register_action( "ss3_turn_right", NULL, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_RIGHT }, 1, move_camera_action );

	ss3_ui_initialize();

	return true;
}

static bool shutdown( void )
{
	ss3_ui_shutdown();

	return true;
}

static bool draw_game( ApeViewport *viewport )
{
	ape_camera_make_active( playerCamera );
	ape_camera_draw_perspective( playerCamera, viewport );
	return true;
}

static void handle_input( void )
{
	PLVector3 ang = ape_camera_get_angles( playerCamera );
	PLVector3 pos = ape_camera_get_position( playerCamera );

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

	ape_camera_set_position( playerCamera, &pos );
	ape_camera_set_angles( playerCamera, &ang );
}

static bool tick_game( void )
{
	handle_input();

	return true;
}

static bool handle_request( ApeGameInterfaceRequest request, void *user )
{
	switch ( request )
	{
		case APE_GAME_INTERFACE_REQUEST_INITIALIZE:
			return initialize();
		case APE_GAME_INTERFACE_REQUEST_SHUTDOWN:
			return shutdown();
		case APE_GAME_INTERFACE_REQUEST_DRAW:
			return draw_game( ( ApeViewport * ) user );
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI:
			return ss3_ui_draw( ( ApeViewport * ) user );
		case APE_GAME_INTERFACE_REQUEST_TICK:
			return tick_game();
		case APE_GAME_INTERFACE_REQUEST_HANDLE_INPUT: break;
		case APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD:
		{
			ApeWorld *world = ( ApeWorld * ) user;

			ape_camera_assign_world( playerCamera, world );

			testLight = ape_light_create( &PLVector3( -2.0f, -2.0f, 1.0f ), &PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.85f ), 0.0f, APE_LIGHT_TYPE_SUN, SS_ARL_LIGHT_FLAG_ENABLED | SS_ARL_LIGHT_FLAG_DYNAMIC | SS_ARL_LIGHT_FLAG_RUNTIME_SHADOWS );
			ape_world_attach_light( world, testLight );

			return true;
		}
		case APE_GAME_INTERFACE_REQUEST_DISCONNECT: break;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

static const ApeGameInterfaceImport apeImport = {
        .requestCallbackMethod = handle_request,
};

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	return &apeImport;
}
