// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Default poly brush class.
// Author:  Mark E. Sowden

#include "world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct ApePolyBrush
{
} ApePolyBrush;

#define SELF( X ) APE_SELF_CAST( ApePolyBrush, X )

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeBrushClass ape_polyBrushClass = {
        .name = "polyBrushClass",
        .editorName = "Poly Brush",
        .editorDescription = "Basic brush used for building polygonal geometry.",
};
