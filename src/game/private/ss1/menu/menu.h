// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "game/private/ss1/ss1_game.h"

// base menu impl.
#include "../../shared/game_menu.h"

typedef enum SS1MenuState
{
	SS1_MENU_STATE_INTRO,
	SS1_MENU_STATE_NAME_INPUT,
	SS1_MENU_STATE_MAIN,
	SS1_MENU_STATE_GAME,
} SS1MenuState;

extern SS1MenuState ss1_menuState_;

void ss1_menu_initialize( void );
void ss1_menu_shutdown();
void ss1_menu_tick( double delta );
void ss1_menu_draw( const ApeViewport *viewport );
