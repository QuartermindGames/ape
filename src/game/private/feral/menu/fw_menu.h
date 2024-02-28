// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../fw_game.h"

// base menu impl.
#include "../../shared/game_menu.h"

void fw_menu_initialize( void );
void fw_menu_tick( void );
void fw_menu_draw( const ApeViewport *viewport );
bool fw_menu_handle_input( void );
