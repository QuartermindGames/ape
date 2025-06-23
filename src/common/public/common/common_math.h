// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "common.h"

static inline bool com_math_vector_check_epsilon( const PLVector3 *va, const PLVector3 *vb )
{
	return fabsf( va->x - vb->x ) <= PL_EPSILON &&
	       fabsf( va->y - vb->y ) <= PL_EPSILON &&
	       fabsf( va->z - vb->z ) <= PL_EPSILON;
}

static inline PLVector3 *com_math_normalize_angles( const PLVector3 *a, PLVector3 *b )
{
	b->x = fmodf( a->x, 360.0f );
	b->y = fmodf( a->y, 360.0f );
	b->z = fmodf( a->z, 360.0f );
	return b;
}

// Linearly interpolate euler angles in degrees, accounting for the wrap-around
static inline PLVector3 *com_math_interpolate_angles( const PLVector3 *a, const PLVector3 *b, float t, PLVector3 *c )
{
	PLVector3 delta = PlSubtractVector3( *b, *a );
	com_math_normalize_angles( &delta, &delta );

	for ( unsigned int i = 0; i < 3; i++ )
	{
		if ( PL_VECTOR3_I( delta, i ) > 180.0f )
		{
			PL_VECTOR3_I( delta, i ) -= 360.0f;
		}
		else if ( PL_VECTOR3_I( delta, i ) < -180.0f )
		{
			PL_VECTOR3_I( delta, i ) += 360.0f;
		}
	}

	c->x = a->x + delta.x * t;
	c->y = a->y + delta.y * t;
	c->z = a->z + delta.z * t;

	com_math_normalize_angles( c, c );

	return c;
}
