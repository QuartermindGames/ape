// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "gui_private.h"
#include "gui_panel.h"

/****************************************
 * PRIVATE
 ****************************************/

static void DrawCursorBackground( GUIPanel *self, bool *override )
{
	*override = false;
}

static void TickCursor( GUIPanel *self, bool *override )
{
	*override = false;
	GUI_Panel_SetPosition( self, guiState.mousePos.x, guiState.mousePos.y );
}

/****************************************
 * PUBLIC
 ****************************************/

GUIPanel *GUI_Cursor_Create( GUIPanel *parent, int x, int y )
{
	if ( parent != NULL && parent->cursor != NULL )
	{
		GUI_Warning( "Only one cursor allowed per panel!\n" );
		return NULL;
	}

	GUIPanel *panel = GUI_Panel_Create( parent, x, y, 32, 32, GUI_PANEL_BACKGROUND_DEFAULT, GUI_PANEL_BORDER_NONE );
	panel->DrawBackground = DrawCursorBackground;
	panel->Tick = TickCursor;

	if ( parent != NULL )
		parent->cursor = panel;

	return panel;
}

void GUI_Cursor_Destroy( GUIPanel *self )
{
	if ( self->parent != NULL )
		self->parent->cursor = NULL;

	GUI_Panel_Destroy( self );
}
