// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "gui/gui_private.h"

PL_EXTERN_C

void YnCore_InitializeGUI( void );
void YnCore_ShutdownGUI( void );
void YnCore_DrawGUI( const YNCoreViewport *viewport );
void YnCore_TickGUI( void );
void YnCore_ResizeGUI( int w, int h );
GUIPanel *YnCore_GetGUIRootPanel( void );

PL_EXTERN_C_END
