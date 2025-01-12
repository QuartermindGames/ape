// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_math.h>

typedef enum ComMathVectorType
{
	COM_MATH_VECTOR_FACING_ANY,
	COM_MATH_VECTOR_FACING_X,
	COM_MATH_VECTOR_FACING_Y,
	COM_MATH_VECTOR_FACING_Z,
} ComMathVectorType;

static inline ComMathVectorType com_math_vector_classify( const PLVector3 *vector )
{
	if ( fabsf( vector->x ) > 0.9f )
	{
		return COM_MATH_VECTOR_FACING_X;
	}
	else if ( fabsf( vector->y ) > 0.9f )
	{
		return COM_MATH_VECTOR_FACING_Y;
	}
	else if ( fabsf( vector->z ) > 0.9f )
	{
		return COM_MATH_VECTOR_FACING_Z;
	}
	else
	{
		return COM_MATH_VECTOR_FACING_ANY;
	}
}

static inline bool com_math_vector_check_epsilon( const PLVector3 *va, const PLVector3 *vb )
{
	return fabsf( va->x - vb->x ) <= PL_EPSILON &&
	       fabsf( va->y - vb->y ) <= PL_EPSILON &&
	       fabsf( va->z - vb->z ) <= PL_EPSILON;
}
