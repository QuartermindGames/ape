// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "gui/gui_private.h"

PL_EXTERN_C

void apeInitializeGUI_( void );
void apeShutdownGUI_( void );
void ogeDrawGUI_( const ApeViewport *viewport );
void apeTickGUI_( void );
void YnCore_ResizeGUI( int w, int h );
GuiPanel *YnCore_GetGUIRootPanel( void );

PL_EXTERN_C_END
