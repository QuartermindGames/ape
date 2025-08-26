// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../ape_private.h"

#include "ape/ape_public_gui.h"

#include <plgraphics/plg.h>

typedef struct ApeGUIState
{
	ApeVector2i mousePos, mouseOldPos;
	QmMathVector2f  mouseWheel, mouseOldWheel;

	unsigned int numBatches, lastNumBatches;    // number of batches this frame
	unsigned int numTriangles, lastNumTriangles;// number of triangles drawn this frame
} ApeGUIState;
extern ApeGUIState ape_guiState_;

void     guiShutdownDraw_( void );
PLGMesh *ape_gui_get_batch_queue_mesh( PLGTexture *texture );
void     ape_gui_draw_filled_rectangle( PLGMesh *mesh, int x, int y, int w, int h, int z, const QmMathColour4ub *colour );
void     ape_gui_draw_quad( PLGMesh *mesh, ApeVector2i tl, ApeVector2i tr, ApeVector2i ll, ApeVector2i lr, int z, const QmMathColour4f *colour );
