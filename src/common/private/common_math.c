// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Common math methods.
// Author:  Mark E. Sowden

#include <plcore/pl_physics.h>

#include "common_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Plane
/////////////////////////////////////////////////////////////////////////////////////

ComMathPlane *com_math_plane_setup( ComMathPlane *self, const PLVector3 *p0, const PLVector3 *p1, const PLVector3 *p2 )
{
	PLVector3 s0 = PlSubtractVector3( *p1, *p0 );
	PLVector3 s1 = PlSubtractVector3( *p2, *p0 );

	self->normal = PlVector3CrossProduct( s0, s1 );
	self->normal = PlNormalizeVector3( self->normal );

	self->distance = -PlVector3DotProduct( self->normal, *p0 );

	return self;
}

float com_math_plane_distance( const ComMathPlane *self, const PLVector3 *pos )
{
	return PlVector3DotProduct( self->normal, *pos ) + self->distance;
}

void com_math_plane_basis_vectors( const ComMathPlane *self, PLVector3 *tangentDst, PLVector3 *bitangentDst )
{
	if ( fabsf( self->normal.x ) > fabsf( self->normal.y ) )
	{
		*tangentDst = PL_VECTOR3( self->normal.z, 0.0f, -self->normal.x );
	}
	else
	{
		*tangentDst = PL_VECTOR3( 0.0f, self->normal.z, -self->normal.y );
	}
	*tangentDst = PlNormalizeVector3( *tangentDst );

	*bitangentDst = PlVector3CrossProduct( self->normal, *tangentDst );
	*bitangentDst = PlNormalizeVector3( *bitangentDst );
}

ComMathPlaneProjection com_math_plane_compute_projection( const ComMathPlane *self )
{
	float nx = fabsf( self->normal.x );
	float ny = fabsf( self->normal.y );
	float nz = fabsf( self->normal.z );

	if ( ny > nx && ny < nz )
	{
		return COM_MATH_PLANE_PROJECTION_XZ;
	}
	if ( nz > nx && nz > ny )
	{
		return COM_MATH_PLANE_PROJECTION_XY;
	}

	return COM_MATH_PLANE_PROJECTION_YZ;
}

PLVector3 com_math_plane_project_point( const ComMathPlane *self, const PLVector3 *point )
{
	float dist = PlVector3DotProduct( self->normal, *point ) + self->distance;
	return PlAddVector3( *point, PlScaleVector3F( self->normal, -dist ) );
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

bool com_math_is_polygon_convex( const PLVector2 *vertices, unsigned int numVertices )
{
	if ( numVertices < 4 )
	{
		return true;
	}

	bool sign = false;
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		PLVector2 a;
		a.x = vertices[ ( i + 2 ) % numVertices ].x - vertices[ ( i + 1 ) % numVertices ].x;
		a.y = vertices[ ( i + 2 ) % numVertices ].y - vertices[ ( i + 1 ) % numVertices ].y;

		PLVector2 b;
		b.x = vertices[ i ].x - vertices[ ( i + 1 ) % numVertices ].x;
		b.y = vertices[ i ].y - vertices[ ( i + 1 ) % numVertices ].y;

		float cp = a.x * b.y - a.y * b.x;
		if ( i == 0 )
		{
			sign = cp > 0.0f;
		}
		else if ( sign != ( cp > 0 ) )
		{
			return false;
		}
	}

	return true;
}

PLVector3 com_math_compute_face_normal( const PLVector3 *vertices, unsigned int numVertices )
{
	PLVector3 normal = {};
	for ( unsigned int i = 0; i < numVertices; i += 3 )
	{
		PLVector3 a = vertices[ i ];
		PLVector3 b = vertices[ i + 1 ];
		PLVector3 c = vertices[ i + 2 ];

		PLVector3 x = PL_VECTOR3( c.x - b.x, c.y - b.y, c.z - b.z );
		PLVector3 y = PL_VECTOR3( a.x - b.x, a.y - b.y, a.z - b.z );
		PLVector3 n = PlNormalizeVector3( PlVector3CrossProduct( x, y ) );

		normal = PlAddVector3( normal, n );
	}

	return PlNormalizeVector3( normal );
}

PLVector3 com_math_pitch_yaw_to_position( float pitch, float yaw )
{
	PLVector3 position = { 1.0f, pitch, 0.0f };
	PLMatrix4 matrix   = PlMatrix4Identity();
	PLMatrix4 m2;
	m2         = PlTranslateMatrix4( position );
	matrix     = PlMultiplyMatrix4( &m2, &matrix );
	m2         = PlRotateMatrix4( PL_DEG2RAD( yaw ), &( PLVector3 ) { 0.0f, 1.0f, 0.0f } );
	matrix     = PlMultiplyMatrix4( &m2, &matrix );
	position.x = matrix.m[ 0 ];
	position.z = matrix.m[ 8 ];
	return position;
}
