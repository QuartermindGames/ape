// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "gui/gui_private.h"

PL_EXTERN_C

void apeInitializeGUI_( void );
void apeShutdownGUI_( void );
void apeDrawGUI_( const ApeViewport *viewport );
void apeTickGUI_( void );
void apeResizeGUI( int w, int h );

PL_EXTERN_C_END
