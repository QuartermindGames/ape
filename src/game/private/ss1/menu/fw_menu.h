// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../pm_game.h"

// base menu impl.
#include "../../shared/game_menu.h"

void pm_menu_initialize( void );
void pm_menu_tick( void );
void pm_menu_draw( const ApeViewport *viewport );
bool pm_menu_handle_input( void );
