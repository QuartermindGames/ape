// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "gui/gui_private.h"

PL_EXTERN_C

void ape_initialize_gui_( void );
void ape_shutdown_gui_( void );
void ape_gui_draw_( ApeViewport *viewport );
void ape_tick_gui_( double delta );

PL_EXTERN_C_END
