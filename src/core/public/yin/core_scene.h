// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_math.h>

/**
 * Standard transform structure.
 */
typedef struct ApeSceneTransform
{
	PLVector3 translation;
	PLVector3 scale;
	PLQuaternion rotation;
} ApeSceneTransform;
#define apeInitializeTransform( TRANSFORM ) memset( ( TRANSFORM ), 0, sizeof( SGTransform ) )
