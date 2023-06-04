// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "../fw_game.h"

// base menu impl.
#include "../../game_menu.h"

void FW_Menu_Initialize( void );
void FW_Menu_Tick( void );
void FW_Menu_Draw( const OgeViewport *viewport );
bool FW_Menu_HandleInput( void );
