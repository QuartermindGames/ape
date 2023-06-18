// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "../ape_private.h"

#include "ape_client_decal.h"

typedef struct ApeDecal
{
	PLVector3 position;
	float width;
	float height;

	PLVectorArray *polygons;
} ApeDecal;
