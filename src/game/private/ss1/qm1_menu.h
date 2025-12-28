// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "../menu/menu.h"

#include "ss1_game.h"

void ss1_menu_initialize_( void );
void ss1_menu_shutdown_();
void ss1_menu_tick( double delta );
void ss1_menu_draw( const ApeViewport *viewport );
