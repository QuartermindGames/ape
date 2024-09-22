// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "common_private.h"

bool com_math_is_polygon_convex( const PLVector2 *vertices, uint numVertices )
{
	if ( numVertices < 4 )
	{
		return true;
	}

	bool sign = false;
	for ( uint i = 0; i < numVertices; ++i )
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

		PLVector3 x = PLVector3( c.x - b.x, c.y - b.y, c.z - b.z );
		PLVector3 y = PLVector3( a.x - b.x, a.y - b.y, a.z - b.z );
		PLVector3 n = PlNormalizeVector3( PlVector3CrossProduct( x, y ) );

		normal = PlAddVector3( normal, n );
	}

	return PlNormalizeVector3( normal );
}
