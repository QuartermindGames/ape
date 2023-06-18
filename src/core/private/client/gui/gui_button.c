// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "gui_private.h"

typedef void( *GUIButtonCallback )( void *userData );
typedef struct GUIButton
{
	GUIButtonCallback callback;
} GUIButton;

GuiPanel *GUI_Button_Create( GuiPanel *parent, const char *label, int x, int y, int w, int h )
{
	GuiPanel *panel = guiCreatePanel( parent, x, y, w, h, GUI_PANEL_BACKGROUND_DEFAULT, GUI_PANEL_BORDER_OUTSET );

	return panel;
}

void GUI_Button_SetCallback( GuiPanel *panel, GUIButtonCallback callback )
{
}
