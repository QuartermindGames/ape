// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for demo game project.

#include "demo_game.h"

static ApeCamera *playerCamera = NULL;

static void MoveCameraCallback( ApeInputState state, const char *id )
{
	if ( state != OGE_INPUT_STATE_DOWN )
	{
		return;
	}

	PLVector3 pos = ogeGetCameraPosition( playerCamera );
	PLVector3 ang = ogeGetCameraAngles( playerCamera );
	if ( strcmp( id, "rotateLeft" ) == 0 )
	{
		ang.y += 0.5f;
	}
	else if ( strcmp( id, "rotateRight" ) == 0 )
	{
		ang.y -= 0.5f;
	}

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	if ( strcmp( id, "moveForward" ) == 0 ) { pos = PlAddVector3( pos, PlScaleVector3F( forward, 0.5f ) ); }
	else if ( strcmp( id, "moveBackward" ) == 0 ) { pos = PlSubtractVector3( pos, PlScaleVector3F( forward, 0.5f ) ); }
	else if ( strcmp( id, "moveLeft" ) == 0 ) { pos = PlAddVector3( pos, PlScaleVector3F( left, 0.5f ) ); }
	else if ( strcmp( id, "moveRight" ) == 0 ) { pos = PlSubtractVector3( pos, PlScaleVector3F( left, 0.5f ) ); }
	else if ( strcmp( id, "moveUp" ) == 0 ) { pos.y += 0.5f; }
	else if ( strcmp( id, "moveDown" ) == 0 ) { pos.y -= 0.5f; }

	ogeSetCameraPosition( playerCamera, &pos );
	ogeSetCameraAngles( playerCamera, &ang );
}

static void InitializeDemoGame( void )
{
	Game_RegisterStandardEntityComponents();

	PlParseConsoleString( "world worlds/glass_house.rfl" );

	playerCamera = ogeCreateCamera( "test", &PLVector3( 0.0f, 0.0f, 0.0f ), &pl_vecOrigin3 );
	ogeMakeCameraActive( playerCamera );

	apeRegisterInputAction( "moveForward", NULL, 0, ( ApeInputKey[] ){ KEY_UP, 'w' }, 2, MoveCameraCallback );
	apeRegisterInputAction( "moveBackward", NULL, 0, ( ApeInputKey[] ){ KEY_DOWN, 's' }, 2, MoveCameraCallback );
	apeRegisterInputAction( "moveLeft", NULL, 0, ( ApeInputKey[] ){ 'a' }, 1, MoveCameraCallback );
	apeRegisterInputAction( "moveRight", NULL, 0, ( ApeInputKey[] ){ 'd' }, 1, MoveCameraCallback );
	apeRegisterInputAction( "moveDown", NULL, 0, ( ApeInputKey[] ){ 'q' }, 1, MoveCameraCallback );
	apeRegisterInputAction( "moveUp", NULL, 0, ( ApeInputKey[] ){ 'e' }, 1, MoveCameraCallback );
	apeRegisterInputAction( "rotateLeft", NULL, 0, ( ApeInputKey[] ){ KEY_LEFT }, 1, MoveCameraCallback );
	apeRegisterInputAction( "rotateRight", NULL, 0, ( ApeInputKey[] ){ KEY_RIGHT }, 1, MoveCameraCallback );
}

static void ShutdownDemoGame( void )
{
	ogeDestroyCamera( playerCamera );
	playerCamera = NULL;
}

static void TickDemoGame( void )
{
}

static bool HandleRequest( GameModeRequest modeRequest, void *user )
{
	switch ( modeRequest )
	{
		case GAMEMODE_REQUEST_TICK:
		{
			TickDemoGame();
			break;
		}
		case GAMEMODE_REQUEST_HANDLEINPUT:
		{
			break;
		}
		case GAMEMODE_REQUEST_SPAWNWORLD:
		{
			break;
		}
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameModeInterface;
const GameModeInterface *Game_GetModeInterface( void )
{
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );

	gameMode.Initialize = InitializeDemoGame;
	gameMode.Shutdown   = ShutdownDemoGame;
	//gameMode.NewGame               = FW_Game_NewGame;
	//gameMode.SaveGame              = FW_Game_SaveGame;
	//gameMode.RestoreGame           = FW_Game_RestoreGame;
	//gameMode.Precache              = FW_Game_Precache;
	//gameMode.Draw                  = FW_Game_Draw;
	//gameMode.DrawMenu              = FW_Game_DrawMenu;
	gameMode.RequestCallbackMethod = HandleRequest;

	return &gameMode;
}
