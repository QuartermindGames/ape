// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for Detox game project.

#include "detox_game.h"
#include "detox_character.h"
#include "detox_world.h"

ToxGlobalVars tox_globalVars;

static ApeCamera *playerCamera = NULL;

static void move_camera_callback( ApeInputState state, const char *id )
{
	if ( state != APE_INPUT_STATE_DOWN )
		return;

	PLVector3 pos = apeGetCameraPosition( playerCamera );
	PLVector3 ang = apeGetCameraAngles( playerCamera );
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

	apeSetCameraPosition( playerCamera, &pos );
	apeSetCameraAngles( playerCamera, &ang );
}

static void SpawnLight( ApeInputState state, const char *id )
{
}

static void initialize_game( void )
{
	PlRegisterConsoleVariable( "tox/sunYaw", "Set the yaw of the sun.", "0", PL_VAR_F32, &tox_globalVars.sunYaw, NULL, false );

	gameRegisterStandardEntityComponents();

	apeRegisterEntityClass( toxGetCharacterClassTable() );

	PlParseConsoleString( "level ship/worlds/alive_intro.wld.n" );
	PlParseConsoleString( "r/fov 45" );

	playerCamera = ar_camera_create( "test", &PLVector3( 0.0f, 0.0f, 0.0f ), &pl_vecOrigin3 );
	ar_camera_make_active( playerCamera );

	apeRegisterInputAction( "moveForward", NULL, 0, ( ApeInputKey[] ){ KEY_UP, 'w' }, 2, move_camera_callback );
	apeRegisterInputAction( "moveBackward", NULL, 0, ( ApeInputKey[] ){ KEY_DOWN, 's' }, 2, move_camera_callback );
	apeRegisterInputAction( "moveLeft", NULL, 0, ( ApeInputKey[] ){ 'a' }, 1, move_camera_callback );
	apeRegisterInputAction( "moveRight", NULL, 0, ( ApeInputKey[] ){ 'd' }, 1, move_camera_callback );
	apeRegisterInputAction( "moveDown", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, move_camera_callback );
	apeRegisterInputAction( "moveUp", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, move_camera_callback );
	apeRegisterInputAction( "rotateLeft", NULL, 0, ( ApeInputKey[] ){ KEY_LEFT }, 1, move_camera_callback );
	apeRegisterInputAction( "rotateRight", NULL, 0, ( ApeInputKey[] ){ KEY_RIGHT }, 1, move_camera_callback );
	apeRegisterInputAction( "spawnLight", NULL, 0, ( ApeInputKey[] ){ KEY_ENTER }, 1, SpawnLight );
}

static void shutdown_game( void )
{
	apeDestroyCamera( playerCamera );
	playerCamera = NULL;
}

static void tick_game( void )
{
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value )
	{
		int mx, my;
		apeGetMouseDelta( &mx, &my );

		PLVector3 ang = apeGetCameraAngles( playerCamera );
		ang.y += ( float ) mx;
		ang.x += ( float ) my;
		ang.x = PlClamp( -90.0f, ang.x, 90.0f );
		apeSetCameraAngles( playerCamera, &ang );
	}

	tox_world_tick();
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
		case GAMEMODE_REQUEST_TICK:
		{
			tick_game();
			break;
		}
		case GAMEMODE_REQUEST_HANDLEINPUT:
		{
			break;
		}
		case GAMEMODE_REQUEST_SPAWNWORLD:
		{
			tox_world_spawn( ( ApeWorld * ) user );
			break;
		}
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameGetModeInterface( void )
{
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );
	gameMode.requestCallbackMethod = handle_request;
	return &gameMode;
}
