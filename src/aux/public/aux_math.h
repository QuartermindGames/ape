// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "aux.h"

typedef struct ComMathRectI32
{
	int x, y, w, h;
} ComMathRectI32;

static inline bool com_math_vector_check_epsilon( const QmMathVector3f *va, const QmMathVector3f *vb )
{
	return fabsf( va->x - vb->x ) <= QM_MATH_EPSILON &&
	       fabsf( va->y - vb->y ) <= QM_MATH_EPSILON &&
	       fabsf( va->z - vb->z ) <= QM_MATH_EPSILON;
}

static inline QmMathVector3f *com_math_normalize_angles( const QmMathVector3f *a, QmMathVector3f *b )
{
	b->x = fmodf( a->x, 360.0f );
	b->y = fmodf( a->y, 360.0f );
	b->z = fmodf( a->z, 360.0f );
	return b;
}

// Linearly interpolate euler angles in degrees, accounting for the wrap-around
static inline QmMathVector3f *com_math_interpolate_angles( const QmMathVector3f *a, const QmMathVector3f *b, float t, QmMathVector3f *c )
{
	QmMathVector3f delta = qm_math_vector3f_sub( *b, *a );
	com_math_normalize_angles( &delta, &delta );

	for ( unsigned int i = 0; i < 3; i++ )
	{
		if ( delta.v[ i ] > 180.0f )
		{
			delta.v[ i ] -= 360.0f;
		}
		else if ( delta.v[ i ] < -180.0f )
		{
			delta.v[ i ] += 360.0f;
		}
	}

	c->x = a->x + delta.x * t;
	c->y = a->y + delta.y * t;
	c->z = a->z + delta.z * t;

	com_math_normalize_angles( c, c );

	return c;
}
