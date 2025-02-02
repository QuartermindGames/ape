// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_physics.h>

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

		PLVector3 x = PL_VECTOR3( c.x - b.x, c.y - b.y, c.z - b.z );
		PLVector3 y = PL_VECTOR3( a.x - b.x, a.y - b.y, a.z - b.z );
		PLVector3 n = PlNormalizeVector3( PlVector3CrossProduct( x, y ) );

		normal = PlAddVector3( normal, n );
	}

	return PlNormalizeVector3( normal );
}

/////////////////////////////////////////////////////////////////////////////////////
// Ray Casting
/////////////////////////////////////////////////////////////////////////////////////

bool com_math_ray_intersect_aabb( const PLCollisionRay *ray, const PLCollisionAABB *aabb, PLVector3 *result )
{
	return PlIsRayIntersectingAabb( aabb, ray, result );
}

bool com_math_ray_intersect_plane( const PLCollisionRay *ray, const PLCollisionPlane *plane, PLVector3 *result )
{
	float denom = PlVector3DotProduct( plane->normal, ray->direction );
	if ( denom >= 0.0f || fabsf( denom ) < 1e-6f )
	{
		return false;
	}

	float d = PlVector3DotProduct( plane->normal, plane->origin );
	float t = ( d - PlVector3DotProduct( plane->normal, ray->origin ) ) / denom;
	if ( t < 0.0f )
	{
		return false;
	}

	if ( result != nullptr )
	{
		result->x = ray->origin.x + t * ray->direction.x;
		result->y = ray->origin.y + t * ray->direction.y;
		result->z = ray->origin.z + t * ray->direction.z;
	}

	return true;
}

bool com_math_ray_intersect_polygon( const PLCollisionRay *ray, const PLVector3 *vertices, uint numVertices, PLVector3 *result )
{
	if ( numVertices < 3 )
	{
		return false;
	}

	for ( uint i = 0; i < numVertices - 1; ++i )
	{
		const PLVector3 *v0 = &vertices[ 0 ];
		const PLVector3 *v1 = &vertices[ i ];
		const PLVector3 *v2 = &vertices[ i + 1 ];

		PLVector3 edge1  = PlSubtractVector3( *v1, *v0 );
		PLVector3 edge2  = PlSubtractVector3( *v2, *v0 );
		PLVector3 normal = PlVector3CrossProduct( edge1, edge2 );

		float denom = PlVector3DotProduct( normal, ray->direction );
		if ( denom >= 0.0f || fabsf( denom ) < 1e-6 )
		{
			continue;
		}

		float d = PlVector3DotProduct( normal, *v0 );
		float t = ( d - PlVector3DotProduct( normal, ray->origin ) ) / denom;
		if ( t < 0.0f )
		{
			continue;
		}

		PLVector3 intersection = {
		        ray->origin.x + t * ray->direction.x,
		        ray->origin.y + t * ray->direction.y,
		        ray->origin.z + t * ray->direction.z,
		};

		PLVector3 point = PlSubtractVector3( intersection, *v0 );

		float area = PlVector3DotProduct( normal, normal );

		PLVector3 c = PlVector3CrossProduct( edge1, point );
		float     a = PlVector3DotProduct( normal, c ) / area;
		if ( a < 0.0f || a > 1.0f )
		{
			continue;
		}

		PLVector3 c2 = PlVector3CrossProduct( point, edge2 );
		float     b  = PlVector3DotProduct( normal, c2 ) / area;
		if ( b < 0.0f || b > 1.0f )
		{
			continue;
		}

		float g = 1.0f - a - b;
		if ( g < 0.0f || g > 1.0f )
		{
			continue;
		}

		if ( result != nullptr )
		{
			*result = intersection;
		}
		return true;
	}

	return false;
}
