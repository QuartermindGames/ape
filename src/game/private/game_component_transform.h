// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

typedef struct ECTransform
{
	PLVector3 translation;
	PLVector3 scale;
	PLVector3 angles;
	int sectorNum;
} ECTransform;
#define ECTRANSFORM( SELF ) ENTITY_COMPONENT_CAST( ( SELF ), ECTransform )
