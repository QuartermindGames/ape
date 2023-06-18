// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "gui_private.h"
#include "gui_panel.h"

/****************************************
 * PRIVATE
 ****************************************/

static void DrawCursorBackground( GuiPanel *self, bool *override )
{
	*override = false;
}

static void TickCursor( GuiPanel *self, bool *override )
{
	*override = false;
	guiSetPanelPosition( self, guiState.mousePos.x, guiState.mousePos.y );
}

/****************************************
 * PUBLIC
 ****************************************/

GuiPanel *guiCreateCursor( GuiPanel *parent, int x, int y )
{
	if ( parent != NULL && parent->cursor != NULL )
	{
		GUI_WARNING( "Only one cursor allowed per panel!\n" );
		return NULL;
	}

	GuiPanel *panel = guiCreatePanel( parent, x, y, 32, 32, GUI_PANEL_BACKGROUND_DEFAULT, GUI_PANEL_BORDER_NONE );
	panel->DrawBackground = DrawCursorBackground;
	panel->Tick = TickCursor;

	if ( parent != NULL )
		parent->cursor = panel;

	return panel;
}

void guiDestroyCursor( GuiPanel *self )
{
	if ( self->parent != NULL )
		self->parent->cursor = NULL;

	guiDestroyPanel( self );
}
