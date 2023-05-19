// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "gui_private.h"

typedef void( *GUIButtonCallback )( void *userData );
typedef struct GUIButton
{
	GUIButtonCallback callback;
} GUIButton;

GUIPanel *GUI_Button_Create( GUIPanel *parent, const char *label, int x, int y, int w, int h )
{
	GUIPanel *panel = GUI_Panel_Create( parent, x, y, w, h, GUI_PANEL_BACKGROUND_DEFAULT, GUI_PANEL_BORDER_OUTSET );

	return panel;
}

void GUI_Button_SetCallback( GUIPanel *panel, GUIButtonCallback callback )
{
}
