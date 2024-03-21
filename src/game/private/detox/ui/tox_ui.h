// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../tox_game.h"

PL_EXTERN_C

void tox_ui_initialize( void );
void tox_ui_shutdown( void );

void tox_ui_handle_damage_event( int16_t damage );

bool tox_ui_draw( ApeViewport *viewport );
void tox_ui_tick( void );

PL_EXTERN_C_END
