// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "aux.h"

#define AUX_VEC2_ARGS( VECTOR ) ( VECTOR ).x, ( VECTOR ).y
#define AUX_VEC3_ARGS( VECTOR ) ( VECTOR ).x, ( VECTOR ).y, ( VECTOR ).z
#define AUX_VEC4_ARGS( VECTOR ) ( VECTOR ).x, ( VECTOR ).y, ( VECTOR ).z, ( VECTOR ).w

#define AUX_VEC2_ARGS_I( VECTOR ) ( int ) ( VECTOR ).x, ( int ) ( VECTOR ).y
#define AUX_VEC3_ARGS_I( VECTOR ) ( int ) ( VECTOR ).x, ( int ) ( VECTOR ).y, ( int ) ( VECTOR ).z
#define AUX_VEC4_ARGS_I( VECTOR ) ( int ) ( VECTOR ).x, ( int ) ( VECTOR ).y, ( int ) ( VECTOR ).z, ( int ) ( VECTOR ).w

typedef struct AuxMathRectI32
{
	int x, y, w, h;
} AuxMathRectI32;

static inline bool aux_math_vector_check_epsilon( const QmMathVector3f *va, const QmMathVector3f *vb )
{
	return fabsf( va->x - vb->x ) <= QM_MATH_EPSILON &&
	       fabsf( va->y - vb->y ) <= QM_MATH_EPSILON &&
	       fabsf( va->z - vb->z ) <= QM_MATH_EPSILON;
}

static inline QmMathVector3f *aux_math_normalize_angles( const QmMathVector3f *a, QmMathVector3f *b )
{
	b->x = fmodf( a->x, 360.0f );
	b->y = fmodf( a->y, 360.0f );
	b->z = fmodf( a->z, 360.0f );
	return b;
}

// Linearly interpolate euler angles in degrees, accounting for the wrap-around
static inline QmMathVector3f *aux_math_interpolate_angles( const QmMathVector3f *a, const QmMathVector3f *b, float t, QmMathVector3f *c )
{
	QmMathVector3f delta = qm_math_vector3f_sub( *b, *a );
	aux_math_normalize_angles( &delta, &delta );

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

	aux_math_normalize_angles( c, c );

	return c;
}

static inline QmMathColour3f16 *aux_math_interpolate_colour_3f16( const QmMathColour3f16 *a, const QmMathColour3f16 *b, float t, QmMathColour3f16 *c )
{
	QmMathColour3f16 delta = QM_MATH_COLOUR3F16( b->r - a->r,
	                                             b->g - a->g,
	                                             b->b - a->b );

	c->r = QM_MATH_CLAMP( 0.0f16, a->r + delta.r * ( _Float16 ) t, 1.0f16 );
	c->g = QM_MATH_CLAMP( 0.0f16, a->g + delta.g * ( _Float16 ) t, 1.0f16 );
	c->b = QM_MATH_CLAMP( 0.0f16, a->b + delta.b * ( _Float16 ) t, 1.0f16 );

	return c;
}
