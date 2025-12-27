// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "../ape_private.h"

#include "ape/ape_public_gui.h"

typedef struct ApeGUIState
{
	QmMathVector2i mousePos, mouseOldPos;
	QmMathVector2f mouseWheel, mouseOldWheel;

	unsigned int numBatches, lastNumBatches;    // number of batches this frame
	unsigned int numTriangles, lastNumTriangles;// number of triangles drawn this frame
} ApeGUIState;
extern ApeGUIState ape_guiState_;
