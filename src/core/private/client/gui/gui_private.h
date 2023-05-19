// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "yin/gui_public.h"

#include <plcore/pl_linkedlist.h>
#include <plcore/pl_console.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_texture.h>

typedef struct GUIState
{
	GUIVector2 mousePos, mouseOldPos;
	PLVector2  mouseWheel, mouseOldWheel;

	unsigned int numBatches, lastNumBatches;    // number of batches this frame
	unsigned int numTriangles, lastNumTriangles;// number of triangles drawn this frame
} GUIState;
extern GUIState guiState;

/****************************************
 * Logging
 ****************************************/

enum
{
	GUI_LOGLEVEL_DEFAULT,
	GUI_LOGLEVEL_WARNING,
	GUI_LOGLEVEL_ERROR,
	GUI_LOGLEVEL_DEBUG,

	GUI_MAX_LOG_LEVELS
};

extern int guiLogLevels[ GUI_MAX_LOG_LEVELS ];

#define GUI_Print( ... )   PlLogMessage( guiLogLevels[ GUI_LOGLEVEL_DEFAULT ], __VA_ARGS__ )
#define GUI_Warning( ... ) PlLogMessage( guiLogLevels[ GUI_LOGLEVEL_WARNING ], __VA_ARGS__ )
#define GUI_Error( ... )   PlLogMessage( guiLogLevels[ GUI_LOGLEVEL_ERROR ], __VA_ARGS__ )
#define GUI_Debug( ... )   PlLogMessage( guiLogLevels[ GUI_LOGLEVEL_DEBUG ], __VA_ARGS__ )

/****************************************
 ****************************************/

enum
{
	GUI_MOUSE_CURSOR_DEFAULT,
	GUI_MOUSE_CURSOR_DENY,
	GUI_MOUSE_CURSOR_MOVE,
	GUI_MOUSE_CURSOR_SIZER_LR,
	GUI_MOUSE_CURSOR_SIZER_TB,

	GUI_MAX_CURSOR_STATES
};

#define GUI_PANEL_BORDER_SIZE 2 /* pixel size all the way around */

enum
{
	GUI_FRAME_BACKGROUND,
	GUI_FRAME_FOREGROUND,
	GUI_FRAME_TOP,
	GUI_FRAME_BOTTOM,
	GUI_FRAME_LEFT,
	GUI_FRAME_RIGHT,

	GUI_MAX_FRAME_ELEMENTS
};

enum
{
	GUI_COLOUR_INSET_BACKGROUND,
	GUI_COLOUR_OUTSET_BACKGROUND,
	GUI_COLOUR_INSET_BORDER_TOP,
	GUI_COLOUR_INSET_BORDER_BOTTOM,
	GUI_COLOUR_OUTSET_BORDER_TOP,
	GUI_COLOUR_OUTSET_BORDER_BOTTOM,

	GUI_MAX_DEFAULT_COLOURS
};

enum
{
	GUI_BORDER_TOP,
	GUI_BORDER_BOTTOM,
	GUI_BORDER_LEFT,
	GUI_BORDER_RIGHT,

	GUI_MAX_BORDER_ELEMENTS
};

typedef struct GUIStyleElement
{
	int tl, tr;
	int ll, lr;
} GUIStyleElement;

typedef struct GUIStyleSheet
{
	PLGTexture *texture;
	PLPath      path;

	PLColourF32 colours[ GUI_MAX_DEFAULT_COLOURS ];

	GUIPanelBorder borderStyle;
	int            borderPadding[ GUI_MAX_BORDER_ELEMENTS ];

	GUIStyleElement frameElements[ GUI_MAX_FRAME_ELEMENTS ];
	GUIStyleElement cursorElements[ GUI_MAX_CURSOR_STATES ];
} GUIStyleSheet;

PLGTexture *GUI_CacheTexture( const char *path );

bool GUI_Font_Initialize( void );

void     GUI_Draw_Initialize( void );
void     GUI_Draw_Shutdown( void );
PLGMesh *GUI_Draw_GetBatchQueueMesh( PLGTexture *texture );
void     GUI_Draw_FilledRectangle( PLGMesh *mesh, int x, int y, int w, int h, int z, const PLColour *colour );
void     GUI_Draw_Quad( PLGMesh *mesh, GUIVector2 tl, GUIVector2 tr, GUIVector2 ll, GUIVector2 lr, int z, const PLColourF32 *colour );
