// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

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

typedef struct GuiPanel
{
	int x, y;
	int w, h;
	bool isDrawing;// Flag on whether the panel is actually in view
	bool isVisible;// User flag, specifying if the panel should show or not

	int z;

	GuiPanelBorder border;
	GuiPanelBackground background;

	const GuiStyleSheet *styleSheet;

	bool bgColourOverride;
	PLColour backgroundColour;

	GuiPanel *parent;
	PLLinkedList *children;
	PLLinkedListNode *node;

	GuiPanel *cursor;

	void ( *Destroy )( GuiPanel *self );                 /* called on destruction */
	void ( *PreDraw )( GuiPanel *self, bool *override ); /* called before all children are drawn */
	void ( *PostDraw )( GuiPanel *self );                /* called after all children are drawn */
	void ( *DrawBackground )( GuiPanel *self, bool *override );
	void ( *Tick )( GuiPanel *self, bool *override );

	bool ( *HandleMouseEvent )( GuiPanel *self, int mx, int my, int wheel, int button, bool buttonUp );
	bool ( *HandleKeyboardEvent )( GuiPanel *self, int button, bool buttonUp );

	void *extendedData;
} GuiPanel;
