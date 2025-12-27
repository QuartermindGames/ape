// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "menu.h"

void game_menu_compass_initialize_( ApeGuiFont *font );
void game_menu_compass_shutdown_();

void game_menu_compass_draw_( const ApeViewport *viewport );
void game_menu_compass_tick_( const ApeCamera *camera, double delta );
