/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "game.h"
#include "act.h"
#include "map.h"

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

MenuState Gam_GetMenuState( void ) {
	return menuState;
}

static Actor *playerActor = NULL;

Actor *Gam_GetPlayer( void ) {
	return playerActor;
}

void Gam_Start( void ) {
	menuState = MENU_STATE_HUD;
	inputTarget = INPUT_TARGET_GAME;

	Map_Load( "Maps/Test.map" ); /* load the map from the global wad */

	Act_SpawnActors();

	/* spawn the player in */
	playerActor = Act_SpawnActor( ACTOR_PLAYER, PLVector3( 500, 0, 1276 ), -90.0f );
}

void Gam_End( void ) {

}

void Gam_Tick( void ) {
	if ( gameState == GAME_STATE_PAUSED ) {
		return;
	}

	Act_TickActors();
}

void Gam_Keyboard( unsigned char key ) {
	if ( inputTarget == INPUT_TARGET_MENU ) {
		switch( menuState ) {
			case MENU_STATE_START:
				/* if any key was hit here, just switch to the game */
				Gam_Start();
				gameState = GAME_STATE_ACTIVE;
				break;
			default:
			PrintError( "Unhandled menu state, %d!\n", menuState );
		}
		return;
	}
}

void Gam_Initialize( void ) {}

void Gam_Shutdown( void ) {}
