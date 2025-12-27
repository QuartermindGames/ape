// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "game/private/ss1/ss1_game.h"

// base menu impl.
#include "../../game_menu.h"

typedef enum SS1MenuState
{
	SS1_MENU_STATE_INTRO,
	SS1_MENU_STATE_NAME_INPUT,
	SS1_MENU_STATE_MAIN,
	SS1_MENU_STATE_GAME,
} SS1MenuState;

extern SS1MenuState ss1_menuState_;

void ss1_menu_initialize_( void );
void ss1_menu_shutdown_();
void ss1_menu_tick( double delta );
void ss1_menu_draw( const ApeViewport *viewport );
