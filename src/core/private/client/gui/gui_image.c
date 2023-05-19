// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "gui_panel.h"

typedef struct GUIImage
{
	PLGTexture *texture;
} GUIImage;

GUIPanel *GUI_Image_Create( GUIPanel *parent, int x, int y, int w, int h, PLGTexture *texture )
{
	GUIPanel *panel = GUI_Panel_Create( parent, x, y, w, h, GUI_PANEL_BACKGROUND_NONE, GUI_PANEL_BORDER_NONE );

	GUIImage *image = PL_NEW( GUIImage );
	image->texture = texture;
	panel->extendedData = image;

	return panel;
}
