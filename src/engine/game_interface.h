/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

typedef enum MenuState
{
	MENU_STATE_START, /* draw start screen */
	MENU_STATE_HUD,	  /* hud/overlay mode */
} MenuState;
MenuState Game_GetMenuState( void );

typedef struct Actor Actor;

Actor *Game_GetPlayer( void );

void Game_Initialize( void );
void Game_Shutdown( void );
void Game_Display( void );
void Game_Tick( void );
void Game_Keyboard( unsigned char key );

void		  Game_SpawnWorld( const char *worldPath );
struct World *Game_GetCurrentWorld( void );

typedef enum GameDifficulty
{
	GAME_DIFFICULTY_NORMAL,
	GAME_DIFFICULTY_EASY,
	GAME_DIFFICULTY_HARD,

	GAME_MAX_DIFFICULTY_MODES
} GameDifficulty;
void Game_SetDifficultyMode( const GameDifficulty difficulty );
GameDifficulty Game_GetDifficultyMode( void );
