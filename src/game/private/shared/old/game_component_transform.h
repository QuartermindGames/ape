// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef struct ECTransform
{
	PLVector3 translation;
	PLVector3 scale;
	PLVector3 angles;
	int sectorNum;
} ECTransform;
#define ECTRANSFORM( SELF ) ENTITY_COMPONENT_CAST( ( SELF ), ECTransform )
