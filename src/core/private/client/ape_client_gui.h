// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "gui/gui_private.h"

PL_EXTERN_C

void ape_initialize_gui_( void );
void ape_shutdown_gui_( void );
void ss_arl_draw_gui_( ApeViewport *viewport );
void ape_tick_gui_( void );
void ss_acl_resize_gui_( int w, int h );

PL_EXTERN_C_END
