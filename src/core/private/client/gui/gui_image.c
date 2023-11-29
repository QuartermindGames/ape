// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "gui_panel.h"

typedef struct GUIImage {
	PLGTexture *texture;
} GUIImage;

GuiPanel *GUI_Image_Create( GuiPanel *parent, int x, int y, int w, int h, PLGTexture *texture ) {
	GuiPanel *panel = ss_gui_panel_create( parent, x, y, w, h, GUI_PANEL_BACKGROUND_NONE, GUI_PANEL_BORDER_NONE );

	GUIImage *image = PL_NEW( GUIImage );
	image->texture = texture;
	panel->extendedData = image;

	return panel;
}
