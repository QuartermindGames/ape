// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include <plcore/pl_math.h>

/**
 * Standard transform structure.
 */
typedef struct ApeSceneTransform
{
	QmMathVector3f translation;
	QmMathVector3f scale;
	PLQuaternion rotation;
} ApeSceneTransform;
#define apeInitializeTransform( TRANSFORM ) memset( ( TRANSFORM ), 0, sizeof( SGTransform ) )
