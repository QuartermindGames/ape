// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for demo game project.

#include "demo_game.h"

static ApeCamera *playerCamera = NULL;

static void MoveCameraCallback( ApeInputState state, const char *id ) {
	if ( state != OGE_INPUT_STATE_DOWN ) {
		return;
	}

	PLVector3 pos = apeGetCameraPosition( playerCamera );
	PLVector3 ang = apeGetCameraAngles( playerCamera );
	if ( strcmp( id, "rotateLeft" ) == 0 ) {
		ang.y += 1.5f;
	} else if ( strcmp( id, "rotateRight" ) == 0 ) {
		ang.y -= 1.5f;
	}

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, NULL, &forward );

	if ( strcmp( id, "moveForward" ) == 0 ) {
		pos = PlAddVector3( pos, PlScaleVector3F( forward, 0.5f ) );
	} else if ( strcmp( id, "moveBackward" ) == 0 ) {
		pos = PlSubtractVector3( pos, PlScaleVector3F( forward, 0.5f ) );
	} else if ( strcmp( id, "moveLeft" ) == 0 ) {
		pos = PlAddVector3( pos, PlScaleVector3F( left, 0.5f ) );
	} else if ( strcmp( id, "moveRight" ) == 0 ) {
		pos = PlSubtractVector3( pos, PlScaleVector3F( left, 0.5f ) );
	} else if ( strcmp( id, "moveUp" ) == 0 ) {
		pos.y += 0.5f;
	} else if ( strcmp( id, "moveDown" ) == 0 ) {
		pos.y -= 0.5f;
	}

	apeSetCameraPosition( playerCamera, &pos );
	apeSetCameraAngles( playerCamera, &ang );
}

static const char *vppPaths[] = {
        "pc_rf_demo/audio.vpp",
        "pc_rf_demo/levels1.vpp",
        "pc_rf_demo/maps.vpp",
        "pc_rf_demo/meshes.vpp",
        "pc_rf_demo/motions.vpp",
        "pc_rf_demo/music.vpp",
        "pc_rf_demo/tables.vpp",
        "pc_rf_demo/ui.vpp",
};
#define NUM_VPP_PACKS PL_ARRAY_ELEMENTS( vppPaths )
static PLFileSystemMount *vppPackages[ NUM_VPP_PACKS ];

static void InitializeDemoGame( void ) {
	for ( unsigned int i = 0; i < NUM_VPP_PACKS; ++i ) {
		vppPackages[ i ] = PlMountLocation( vppPaths[ i ] );
		if ( vppPackages[ i ] == NULL ) {
			Game_Warning( "Failed to open package (%s): %s\n", vppPaths[ i ], PlGetError() );
		}
	}

	gameRegisterStandardEntityComponents();

	PlParseConsoleString( "world worlds/test_geo.rfl" );

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
}

static void ShutdownDemoGame( void ) {
	for ( unsigned int i = 0; i < NUM_VPP_PACKS; ++i ) {
		if ( vppPackages[ i ] == NULL ) {
			continue;
		}

		PlClearMountedLocation( vppPackages[ i ] );
		vppPackages[ i ] = NULL;
	}

	apeDestroyCamera( playerCamera );
	playerCamera = NULL;
}

static void TickDemoGame( void ) {
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value ) {
		int mx, my;
		apeGetMouseDelta( &mx, &my );

		PLVector3 ang = apeGetCameraAngles( playerCamera );
		ang.y += mx;
		ang.x += my;
		ang.x = PlClamp( -90.0f, ang.x, 90.0f );
		apeSetCameraAngles( playerCamera, &ang );
	}
}

static bool HandleRequest( GameModeRequest modeRequest, void *user ) {
	switch ( modeRequest ) {
		case GAMEMODE_REQUEST_INITIALIZE: {
			InitializeDemoGame();
			return true;
		}
		case GAMEMODE_REQUEST_TICK: {
			TickDemoGame();
			break;
		}
		case GAMEMODE_REQUEST_HANDLEINPUT: {
			break;
		}
		case GAMEMODE_REQUEST_SPAWNWORLD: {
			for ( unsigned int i = 0; i < 4; ++i ) {
				apeCreateEntityFromPrefab( "base/test" );
			}
			break;
		}
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameModeInterface;
const GameModeInterface *gameGetModeInterface( void ) {
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );

	gameMode.Initialize = InitializeDemoGame;
	gameMode.Shutdown = ShutdownDemoGame;
	//gameMode.NewGame               = FW_Game_NewGame;
	//gameMode.SaveGame              = FW_Game_SaveGame;
	//gameMode.RestoreGame           = FW_Game_RestoreGame;
	//gameMode.Precache              = FW_Game_Precache;
	//gameMode.Draw                  = FW_Game_Draw;
	//gameMode.DrawMenu              = FW_Game_DrawMenu;
	gameMode.RequestCallbackMethod = HandleRequest;

	return &gameMode;
}
