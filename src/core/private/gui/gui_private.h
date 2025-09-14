// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../ape_private.h"

#include "ape/ape_public_gui.h"

#include <plgraphics/plg.h>

typedef struct ApeGUIState
{
	QmMathVector2i mousePos, mouseOldPos;
	QmMathVector2f mouseWheel, mouseOldWheel;

	unsigned int numBatches, lastNumBatches;    // number of batches this frame
	unsigned int numTriangles, lastNumTriangles;// number of triangles drawn this frame
} ApeGUIState;
extern ApeGUIState ape_guiState_;

void     ape_gui_draw_shutdown_( void );
