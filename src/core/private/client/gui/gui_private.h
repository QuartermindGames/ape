// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "yin/gui_public.h"

#include <plcore/pl_linkedlist.h>
#include <plcore/pl_console.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_texture.h>

typedef struct GuiState
{
	GUIVector2 mousePos, mouseOldPos;
	PLVector2 mouseWheel, mouseOldWheel;

	unsigned int numBatches, lastNumBatches;    // number of batches this frame
	unsigned int numTriangles, lastNumTriangles;// number of triangles drawn this frame
} GuiState;
extern GuiState guiState;

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

extern int gui_LogLevels_[ GUI_MAX_LOG_LEVELS ];

#define GUI_PRINT( ... )   PlLogMessage( gui_LogLevels_[ GUI_LOGLEVEL_DEFAULT ], __VA_ARGS__ )
#define GUI_WARNING( ... ) PlLogMessage( gui_LogLevels_[ GUI_LOGLEVEL_WARNING ], __VA_ARGS__ )
#define GUI_ERROR( ... )   PlLogMessage( gui_LogLevels_[ GUI_LOGLEVEL_ERROR ], __VA_ARGS__ )
#define GUI_DEBUG( ... )   PlLogMessage( guiLogLevels[ GUI_LOGLEVEL_DEBUG ], __VA_ARGS__ )

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

typedef struct GuiStyleElement
{
	int tl, tr;
	int ll, lr;
} GuiStyleElement;

typedef struct GuiStyleSheet
{
	PLGTexture *texture;
	PLPath path;

	PLColourF32 colours[ GUI_MAX_DEFAULT_COLOURS ];

	GuiPanelBorder borderStyle;
	int borderPadding[ GUI_MAX_BORDER_ELEMENTS ];

	GuiStyleElement frameElements[ GUI_MAX_FRAME_ELEMENTS ];
	GuiStyleElement cursorElements[ GUI_MAX_CURSOR_STATES ];
} GuiStyleSheet;

PLGTexture *guiCacheTexture( const char *path );

bool guiInitializeFonts_( void );

void guiInitializeDraw_( void );
void guiShutdownDraw_( void );
PLGMesh *guiGetBatchQueueMesh( PLGTexture *texture );
void guiDrawFilledRectangle( PLGMesh *mesh, int x, int y, int w, int h, int z, const PLColour *colour );
void guiDrawQuad( PLGMesh *mesh, GUIVector2 tl, GUIVector2 tr, GUIVector2 ll, GUIVector2 lr, int z, const PLColourF32 *colour );
