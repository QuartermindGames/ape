// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "gui/gui_private.h"

PL_EXTERN_C

void apeInitializeGUI_( void );
void apeShutdownGUI_( void );
void ss_arl_draw_gui_( SSArlViewport *viewport );
void ss_acl_tick_gui_( void );
void apeResizeGUI( int w, int h );

PL_EXTERN_C_END
