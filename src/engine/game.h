/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

typedef enum MenuState {
	MENU_STATE_START, /* draw start screen */
	MENU_STATE_HUD,   /* hud/overlay mode */
} MenuState;
MenuState Game_GetMenuState( void );

typedef struct Actor Actor;

Actor *Game_GetPlayer( void );

void Game_Initialize( void );
void Game_Shutdown( void );
void Game_Display( void );
void Game_Tick( void );
void Game_Keyboard( unsigned char key );
