/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "game.h"
#include "act.h"
#include "map.h"
#include "gfx.h"

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
		return;
	}

	Act_TickActors();
}

void Game_Keyboard( unsigned char key ) {
#if 1
	if ( inputTarget == INPUT_TARGET_MENU ) {
		switch( menuState ) {
			case MENU_STATE_START:
				/* if any key was hit here, just switch to the game */
				Game_Start();
				gameState = GAME_STATE_ACTIVE;
				break;
			default:
			PrintError( "Unhandled menu state, %d!\n", menuState );
		}
		return;
	}
#endif
}

void Game_Initialize( void ) {
	/* initialize core services */
	Gfx_Initialize();
	Act_Initialize();
}

void Game_Display( void ) {
	if ( playerActor == NULL ) {
		return;
	}

	GfxCamera *playerCamera = Player_GetCamera( playerActor );
	if ( playerCamera == NULL ) {
		return;
	}

	Sys_MakeWindowActive( playerCamera->viewportPtr );

	Gfx_SetupDefaultState();

	Gfx_EnableShaderProgram( SHADER_GENERIC );

	Gfx_DrawPerspective( playerCamera );
	Gfx_DisplayMenu();

	Sys_SwapWindow( playerCamera->viewportPtr );
}

void Game_Shutdown( void ) {
	/* shutdown core services */
	Act_Shutdown();
	Gfx_Shutdown();
}

void Game_SetupInterface( EngineInterface *interface ) {
	interface->Tick = Game_Tick;
	interface->Shutdown = Game_Shutdown;
	interface->Initialize = Game_Initialize;
	interface->Display = Game_Display;
}
