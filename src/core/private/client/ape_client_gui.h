// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "gui/gui_private.h"

PL_EXTERN_C

void ape_initialize_gui_( void );
void ape_shutdown_gui_( void );
void ape_draw_gui_( ApeViewport *viewport );
void ape_tick_gui_( void );
void ss_acl_resize_gui_( int w, int h );

PL_EXTERN_C_END
