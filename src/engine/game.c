/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

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

static void Game_Start( void ) {
	menuState = MENU_STATE_HUD;
	inputTarget = INPUT_TARGET_GAME;

	Map_Load( "Maps/Test.map" ); /* load the map from the global wad */

	Act_SpawnActors();

	/* spawn the player in */
	playerActor = Act_SpawnActor( ACTOR_PLAYER, PLVector3( 500, 0, 1276 ), -90.0f );
}

void Game_Tick( void ) {
	if ( gameState == GAME_STATE_PAUSED ) {
		if( g_system.GetKeyState( 'z' ) ) {
			/* if any key was hit here, just switch to the game */
			Game_Start();
			gameState = GAME_STATE_ACTIVE;
		}

		return;
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
	Gfx_DrawMenu();
}

void Game_Initialize( void ) {
	//if( !plHasCommandLineArgument( "editor" ) ) {
		/* if editor mode isn't specified, just launch the game */
	//	Game_Start();
	//}
}

void Game_Shutdown( void ) {}
