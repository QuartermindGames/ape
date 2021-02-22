/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "game.h"
#include "actor.h"
#include "map.h"
#include "renderer/renderer.h"

/* game specific implementation goes here! */

typedef enum InputTarget {
	INPUT_TARGET_MENU, /* menu mode */
	INPUT_TARGET_GAME, /* game mode */
} InputTarget;
static InputTarget inputTarget = INPUT_TARGET_MENU;
static MenuState menuState = MENU_STATE_START;

typedef enum GameState {
	GAME_STATE_PAUSED,
	GAME_STATE_ACTIVE,
} GameState;
GameState gameState = GAME_STATE_PAUSED;

MenuState Game_GetMenuState( void ) {
	return menuState;
}

static Actor *playerActor = NULL;

Actor *Game_GetPlayer( void ) {
	return playerActor;
}

void Game_Tick( void ) {
	if ( gameState == GAME_STATE_PAUSED ) {
		if( g_system.GetKeyState( 'z' ) ) {
			/* if any key was hit here, just switch to the game */
			gameState = GAME_STATE_ACTIVE;
		}

		return;
	}

	static unsigned int spawnDelay = 0;
	if( g_system.GetKeyState( 'z' ) && spawnDelay < Engine_GetNumTicks() ) {
		Act_SpawnActor( ACTOR_PLAYER, PLVector3( 0, 0, 0 ), 0.0f );
		spawnDelay = Engine_GetNumTicks() + 50;
	}

	Act_TickActors();
}

void Game_Display( void ) {
	if ( playerActor == NULL ) {
		return;
	}

	GfxCamera *playerCamera = Player_GetCamera( playerActor );
	if ( playerCamera == NULL ) {
		return;
	}

	Gfx_DrawPerspective( playerCamera );
}

void Game_SpawnWorld( const char *worldPath ) {
    Map_Load( worldPath ); /* load the map from the global wad */

    Act_SpawnActors();

    /* spawn the player in */
    playerActor = Act_SpawnActor( ACTOR_PLAYER, PLVector3( 0, 0, 0 ), 0.0f );

    gameState = GAME_STATE_ACTIVE;
    menuState = MENU_STATE_HUD;
    inputTarget = INPUT_TARGET_GAME;
}

static void Cmd_SpawnWorld( unsigned int argc, char **argv ) {
	if ( argc <= 1 ) {
		PrintWarn( "Invalid argument, please specify world!\n" );
		return;
	}

	Game_SpawnWorld( argv[ 1 ] );
}

void Game_Initialize( void ) {
	plRegisterConsoleCommand( "game.spawn", Cmd_SpawnWorld, "Load in and spawn the specified world." );
}

void Game_Shutdown( void ) {}
