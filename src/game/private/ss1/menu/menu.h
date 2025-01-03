// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "game/private/ss1/ss1_game.h"

// base menu impl.
#include "../../shared/game_menu.h"

void ss1_menu_initialize( void );
void ss1_menu_shutdown();
void ss1_menu_tick( void );
void ss1_menu_draw( const ApeViewport *viewport );
bool ss1_menu_handle_input( void );
