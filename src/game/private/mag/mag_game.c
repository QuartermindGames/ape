// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for demo game project.

#include "mag_game.h"

static ApeCamera *playerCamera = NULL;

static void move_camera_callback( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	PLVector3 pos = arl_camera_get_position( playerCamera );
	PLVector3 ang = arl_camera_get_angles( playerCamera );
	if ( strcmp( id, "rotateLeft" ) == 0 )
		ang.y += 1.5f;
	else if ( strcmp( id, "rotateRight" ) == 0 )
		ang.y -= 1.5f;

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	if ( strcmp( id, "moveForward" ) == 0 )
		pos = PlAddVector3( pos, PlScaleVector3F( forward, 0.5f ) );
	else if ( strcmp( id, "moveBackward" ) == 0 )
		pos = PlSubtractVector3( pos, PlScaleVector3F( forward, 0.5f ) );
	else if ( strcmp( id, "moveLeft" ) == 0 )
		pos = PlAddVector3( pos, PlScaleVector3F( left, 0.5f ) );
	else if ( strcmp( id, "moveRight" ) == 0 )
		pos = PlSubtractVector3( pos, PlScaleVector3F( left, 0.5f ) );
	else if ( strcmp( id, "moveUp" ) == 0 )
		pos.y += 0.5f;
	else if ( strcmp( id, "moveDown" ) == 0 )
		pos.y -= 0.5f;

	arl_camera_set_position( playerCamera, &pos );
	arl_camera_set_angles( playerCamera, &ang );
}

static void initialize_game( void )
{
	game_register_standard_entity_components();

	PlParseConsoleString( "level L01S2.rfl" );

	playerCamera = arl_camera_create( "test", &PLVector3( 0.0f, 0.0f, 0.0f ), &pl_vecOrigin3 );
	arl_camera_make_active( playerCamera );

	acl_input_register_action( "moveForward", NULL, 0, ( ApeInputKey[] ){ KEY_UP, 'w' }, 2, move_camera_callback );
	acl_input_register_action( "moveBackward", NULL, 0, ( ApeInputKey[] ){ KEY_DOWN, 's' }, 2, move_camera_callback );
	acl_input_register_action( "moveLeft", NULL, 0, ( ApeInputKey[] ){ 'a' }, 1, move_camera_callback );
	acl_input_register_action( "moveRight", NULL, 0, ( ApeInputKey[] ){ 'd' }, 1, move_camera_callback );
	acl_input_register_action( "moveDown", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, move_camera_callback );
	acl_input_register_action( "moveUp", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, move_camera_callback );
	acl_input_register_action( "rotateLeft", NULL, 0, ( ApeInputKey[] ){ KEY_LEFT }, 1, move_camera_callback );
	acl_input_register_action( "rotateRight", NULL, 0, ( ApeInputKey[] ){ KEY_RIGHT }, 1, move_camera_callback );
}

static void shutdown_game( void )
{
	arl_camera_destroy( playerCamera );
	playerCamera = NULL;
}

static void tick_game( void )
{
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value )
	{
		int mx, my;
		apeGetMouseDelta( &mx, &my );

		PLVector3 ang = arl_camera_get_angles( playerCamera );
		ang.y += mx;
		ang.x += my;
		ang.x = PlClamp( -90.0f, ang.x, 90.0f );
		arl_camera_set_angles( playerCamera, &ang );
	}
}

static bool handle_request( GameModeRequest modeRequest, void *user )
{
	switch ( modeRequest )
	{
		case GAMEMODE_REQUEST_INITIALIZE:
		{
			initialize_game();
			return true;
		}
		case GAMEMODE_REQUEST_SHUTDOWN:
		{
			shutdown_game();
			return true;
		}
		case GAMEMODE_REQUEST_TICK:
		{
			tick_game();
			break;
		}
		case GAMEMODE_REQUEST_HANDLE_INPUT:
		{
			break;
		}
		case GAMEMODE_REQUEST_SPAWN_LEVEL:
		{
			break;
		}
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameModeInterface;
const GameModeInterface *gameGetModeInterface( void )
{
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );
	gameMode.requestCallbackMethod = handle_request;
	return &gameMode;
}
