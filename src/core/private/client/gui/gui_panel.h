/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

#include "gui_private.h"

#define GUI_DEFAULT_INSET_COLOUR \
	( PLColour )                 \
	{                            \
		122, 122, 122, 255       \
	}
#define GUI_DEFAULT_OUTSET_COLOUR \
	( PLColour )                  \
	{                             \
		192, 192, 192, 255        \
	}

typedef struct GUIPanel
{
	int  x, y;
	int  w, h;
	bool isDrawing;// Flag on whether the panel is actually in view
	bool isVisible;// User flag, specifying if the panel should show or not

	int z;

	GUIPanelBorder     border;
	GUIPanelBackground background;

	const GUIStyleSheet *styleSheet;

	bool     bgColourOverride;
	PLColour backgroundColour;

	GUIPanel         *parent;
	PLLinkedList     *children;
	PLLinkedListNode *node;

	GUIPanel *cursor;

	void ( *Destroy )( GUIPanel *self );                 /* called on destruction */
	void ( *PreDraw )( GUIPanel *self, bool *override ); /* called before all children are drawn */
	void ( *PostDraw )( GUIPanel *self );                /* called after all children are drawn */
	void ( *DrawBackground )( GUIPanel *self, bool *override );
	void ( *Tick )( GUIPanel *self, bool *override );

	bool ( *HandleMouseEvent )( GUIPanel *self, int mx, int my, int wheel, int button, bool buttonUp );
	bool ( *HandleKeyboardEvent )( GUIPanel *self, int button, bool buttonUp );

	void *extendedData;
} GUIPanel;
