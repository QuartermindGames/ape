// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_physics.h>

#include "common_private.h"

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
