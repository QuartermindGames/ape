// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for Detox game project.

#include "detox_game.h"
#include "detox_character.h"

static ApeCamera *playerCamera = NULL;

static void MoveCameraCallback( ApeInputState state, const char *id ) {
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

static void SpawnLight( ApeInputState state, const char *id ) {
}

static void InitializeGame( void ) {
	gameRegisterStandardEntityComponents();

	apeRegisterEntityClass( toxGetCharacterClassTable() );

	PlParseConsoleString( "world worlds/train02.rfl" );

	playerCamera = apeCreateCamera( "test", &PLVector3( 0.0f, 0.0f, 0.0f ), &pl_vecOrigin3 );
	apeMakeCameraActive( playerCamera );

	apeRegisterInputAction( "moveForward", NULL, 0, ( ApeInputKey[] ){ KEY_UP, 'w' }, 2, MoveCameraCallback );
	apeRegisterInputAction( "moveBackward", NULL, 0, ( ApeInputKey[] ){ KEY_DOWN, 's' }, 2, MoveCameraCallback );
	apeRegisterInputAction( "moveLeft", NULL, 0, ( ApeInputKey[] ){ 'a' }, 1, MoveCameraCallback );
	apeRegisterInputAction( "moveRight", NULL, 0, ( ApeInputKey[] ){ 'd' }, 1, MoveCameraCallback );
	apeRegisterInputAction( "moveDown", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, MoveCameraCallback );
	apeRegisterInputAction( "moveUp", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, MoveCameraCallback );
	apeRegisterInputAction( "rotateLeft", NULL, 0, ( ApeInputKey[] ){ KEY_LEFT }, 1, MoveCameraCallback );
	apeRegisterInputAction( "rotateRight", NULL, 0, ( ApeInputKey[] ){ KEY_RIGHT }, 1, MoveCameraCallback );
	apeRegisterInputAction( "spawnLight", NULL, 0, ( ApeInputKey[] ){ KEY_ENTER }, 1, SpawnLight );
}

static void ShutdownGame( void ) {
	apeDestroyCamera( playerCamera );
	playerCamera = NULL;
}

static void TickGame( void ) {
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value ) {
		int mx, my;
		apeGetMouseDelta( &mx, &my );

		PLVector3 ang = apeGetCameraAngles( playerCamera );
		ang.y += ( float ) mx;
		ang.x += ( float ) my;
		ang.x = PlClamp( -90.0f, ang.x, 90.0f );
		apeSetCameraAngles( playerCamera, &ang );
	}
}

static bool HandleRequest( GameModeRequest modeRequest, void *user ) {
	switch ( modeRequest ) {
		case GAMEMODE_REQUEST_INITIALIZE: {
			InitializeGame();
			return true;
		}
		case GAMEMODE_REQUEST_TICK: {
			TickGame();
			break;
		}
		case GAMEMODE_REQUEST_HANDLEINPUT: {
			break;
		}
		case GAMEMODE_REQUEST_SPAWNWORLD: {
			break;
		}
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameGetModeInterface( void ) {
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );

	gameMode.RequestCallbackMethod = HandleRequest;

	return &gameMode;
}
