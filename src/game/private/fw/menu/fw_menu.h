// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "../fw_game.h"

// base menu impl.
#include "../../game_menu.h"

void fw_menu_initialize( void );
void fw_menu_tick( void );
void fw_menu_draw( const ApeViewport *viewport );
bool fw_menu_handle_input( void );
