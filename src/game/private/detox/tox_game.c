// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Main file for Detox game project.

#include "tox_game.h"
#include "tox_character.h"
#include "tox_world.h"

#include "ui/tox_ui.h"

ToxGlobalVars tox_globalVars;

static ApeCamera *playerCamera = NULL;

ApeCamera *tox_get_player_camera( void )
{
	return playerCamera;
}

static void move_camera_iso_callback( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	PLVector3 ang = ape_camera_get_angles( playerCamera );

	ang.x = 0.0f;
	if ( strcmp( id, "rotateLeft" ) == 0 )
		ang.y += 1.5f;
	else if ( strcmp( id, "rotateRight" ) == 0 )
		ang.y -= 1.5f;

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	static const float SPEED = 0.5f;

	PLVector3 pos = ape_camera_get_position( playerCamera );
	if ( strcmp( id, "moveForward" ) == 0 )
		pos = PlAddVector3( pos, PlScaleVector3F( forward, SPEED ) );
	else if ( strcmp( id, "moveBackward" ) == 0 )
		pos = PlSubtractVector3( pos, PlScaleVector3F( forward, SPEED ) );
	else if ( strcmp( id, "moveLeft" ) == 0 )
		pos = PlAddVector3( pos, PlScaleVector3F( left, SPEED ) );
	else if ( strcmp( id, "moveRight" ) == 0 )
		pos = PlSubtractVector3( pos, PlScaleVector3F( left, SPEED ) );
	else if ( strcmp( id, "moveUp" ) == 0 )
		pos.y += 0.5f;
	else if ( strcmp( id, "moveDown" ) == 0 )
		pos.y -= 0.5f;

	ape_camera_set_position( playerCamera, &pos );
	ape_camera_set_angles( playerCamera, &ang );
}

static void move_camera_callback( ApeInputState state, const char *id )
{
	if ( !( state & APE_INPUT_STATE_DOWN ) )
	{
		return;
	}

	PLVector3 pos = ape_camera_get_position( playerCamera );
	PLVector3 ang = ape_camera_get_angles( playerCamera );
	if ( strcmp( id, "rotateLeft" ) == 0 )
	{
		ang.y += 1.5f;
	}
	else if ( strcmp( id, "rotateRight" ) == 0 )
	{
		ang.y -= 1.5f;
	}

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	static const float SPEED = 0.5f;

	if ( strcmp( id, "moveForward" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( forward, SPEED ) );
	}
	else if ( strcmp( id, "moveBackward" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( forward, SPEED ) );
	}
	else if ( strcmp( id, "moveLeft" ) == 0 )
	{
		pos = PlAddVector3( pos, PlScaleVector3F( left, SPEED ) );
	}
	else if ( strcmp( id, "moveRight" ) == 0 )
	{
		pos = PlSubtractVector3( pos, PlScaleVector3F( left, SPEED ) );
	}
	else if ( strcmp( id, "moveUp" ) == 0 )
	{
		pos.y += 0.5f;
	}
	else if ( strcmp( id, "moveDown" ) == 0 )
	{
		pos.y -= 0.5f;
	}

	ape_camera_set_position( playerCamera, &pos );
	ape_camera_set_angles( playerCamera, &ang );
}

static void rotate_camera_action( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	PLVector3 ang = ape_camera_get_angles( playerCamera );
	if ( strcmp( id, "rotateUp" ) == 0 )
	{
		ang.x += 1.5f;
	}
	else if ( strcmp( id, "rotateDown" ) == 0 )
	{
		ang.x -= 1.5f;
	}

	ang.x = PlClamp( -90.0f, ang.x, 90.0f );

	ape_camera_set_angles( playerCamera, &ang );
}

static void progress_time_action( ApeInputState state, PL_UNUSED const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
	{
		return;
	}

	ToxWorldState *worldState = tox_world_get_state();
	if ( worldState == NULL )
	{
		return;
	}

	if ( strcmp( id, "time_forward" ) == 0 )
	{
		worldState->seconds += TOX_WORLD_SECONDS_TO_HOUR / 100;
	}
	else
	{
		worldState->seconds -= TOX_WORLD_SECONDS_TO_HOUR / 100;
	}
}

static void print_pos_command( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	PLVector3 cameraPos = ape_camera_get_position( playerCamera );
	Game_Print( "Camera Pos: %s\n", PlPrintVector3( &cameraPos, PL_VAR_F32 ) );
	PLVector3 cameraAngles = ape_camera_get_angles( playerCamera );
	Game_Print( "Camera Ang: %s\n", PlPrintVector3( &cameraAngles, PL_VAR_F32 ) );
}

static void set_time_command( unsigned int argc, char **argv )
{
	ToxWorldState *worldState = tox_world_get_state();
	worldState->seconds = strtoul( argv[ 1 ], NULL, 10 );
}

static bool initialize_game( void )
{
	PlRegisterConsoleVariable( "tox_time_speed", "Sets the speed of time.", "2000", PL_VAR_F32, &tox_globalVars.timeSpeed, NULL, false );

	PlRegisterConsoleCommand( "tox_print_pos", "Print the camera position and angles.", 0, print_pos_command );
	PlRegisterConsoleCommand( "tox_set_time", "Sets the world time.", 1, set_time_command );

	// movement actions
	ape_client_input_register_action( "moveForward", ( ApeInputButton[] ){ APE_INPUT_UP }, 1, ( ApeInputKey[] ){ APE_INPUT_KEY_UP, 'w' }, 2, move_camera_callback );
	ape_client_input_register_action( "moveBackward", ( ApeInputButton[] ){ APE_INPUT_DOWN }, 1, ( ApeInputKey[] ){ APE_INPUT_KEY_DOWN, 's' }, 2, move_camera_callback );
	ape_client_input_register_action( "moveLeft", ( ApeInputButton[] ){ INPUT_LEFT }, 1, ( ApeInputKey[] ){ 'a' }, 1, move_camera_callback );
	ape_client_input_register_action( "moveRight", ( ApeInputButton[] ){ INPUT_RIGHT }, 1, ( ApeInputKey[] ){ 'd' }, 1, move_camera_callback );
	ape_client_input_register_action( "moveDown", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, move_camera_callback );
	ape_client_input_register_action( "moveUp", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, move_camera_callback );
	ape_client_input_register_action( "rotateLeft", NULL, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_LEFT }, 1, move_camera_callback );
	ape_client_input_register_action( "rotateRight", NULL, 0, ( ApeInputKey[] ){ APE_INPUT_KEY_RIGHT }, 1, move_camera_callback );

	// this remaining bunch are for debugging purposes...
	ape_client_input_register_action( "time_forward", NULL, 0, ( ApeInputKey[] ){ 'z' }, 1, progress_time_action );
	ape_client_input_register_action( "time_backward", NULL, 0, ( ApeInputKey[] ){ 'x' }, 1, progress_time_action );

	ape_client_input_register_action( "rotateUp", NULL, 0, ( ApeInputKey[] ){ 'r' }, 1, rotate_camera_action );
	ape_client_input_register_action( "rotateDown", NULL, 0, ( ApeInputKey[] ){ 'f' }, 1, rotate_camera_action );

	ss_game_register_standard_entity_components_();

	ape_register_entity_class( tox_character_get_class_table() );

	playerCamera = ape_camera_create( "tox_camera_player", &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE );
	if ( playerCamera == NULL )
	{
		Game_Error( "Failed to create player camera!\n" );
		return false;
	}

	return true;
}

static void shutdown_game( void )
{
	ape_camera_destroy( playerCamera );
	playerCamera = NULL;
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

	tox_world_tick();
	tox_ui_tick();

	return true;
}

static bool draw_game( ApeViewport *viewport )
{
	ape_camera_make_active( playerCamera );
	ape_camera_draw_perspective( playerCamera, viewport );
	return true;
}

static bool handle_request( ApeGameInterfaceRequest modeRequest, void *user )
{
	switch ( modeRequest )
	{
		case APE_GAME_INTERFACE_REQUEST_INITIALIZE:
			return initialize_game();
		case APE_GAME_INTERFACE_REQUEST_TICK:
			return tick_game();
		case APE_GAME_INTERFACE_REQUEST_DRAW:
			return draw_game( ( ApeViewport * ) user );
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI:
			return tox_ui_draw( ( ApeViewport * ) user );
		case APE_GAME_INTERFACE_REQUEST_HANDLE_INPUT:
			break;
		case APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD:
			tox_world_spawn( ( ApeWorld * ) user );
			return true;
		default:
			break;
	}

	return false;
}

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	static ApeGameInterfaceImport gameMode;
	PL_ZERO_( gameMode );
	gameMode.requestCallbackMethod = handle_request;
	return &gameMode;
}
