// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_physics.h>

#include "common_private.h"

bool com_collision_aabb_intersect_aabb( const PLCollisionAABB *a, const PLCollisionAABB *b, PLVector3 *result )
{
	PLVector3 maxA = PlAddVector3( a->maxs, a->origin );
	PLVector3 minA = PlAddVector3( a->mins, a->origin );
	PLVector3 maxB = PlAddVector3( b->maxs, b->origin );
	PLVector3 minB = PlAddVector3( b->mins, b->origin );

	bool hit = ( minA.x <= maxB.x && maxA.x >= minB.x ) &&
	           ( minA.y <= maxB.y && maxA.y >= minB.y ) &&
	           ( minA.z <= maxB.z && maxA.z >= minB.z );
	if ( hit && result != nullptr )
	{
		PLVector3 min = PL_VECTOR3( fmaxf( a->mins.x, b->mins.x ),
		                            fmaxf( a->mins.y, b->mins.y ),
		                            fmaxf( a->mins.z, b->mins.z ) );
		PLVector3 max = PL_VECTOR3( fminf( a->maxs.x, b->maxs.x ),
		                            fminf( a->maxs.y, b->maxs.y ),
		                            fminf( a->maxs.z, b->maxs.z ) );

		*result = PlScaleVector3F( PlAddVector3( min, max ), 0.5f );
	}

	return hit;
}

/////////////////////////////////////////////////////////////////////////////////////
// Ray Casting
/////////////////////////////////////////////////////////////////////////////////////

bool com_collision_ray_intersect_aabb( const PLCollisionRay *ray, const PLCollisionAABB *aabb, PLVector3 *result )
{
	return PlIsRayIntersectingAabb( aabb, ray, result );
}

bool com_collision_ray_intersect_plane( const PLCollisionRay *ray, const PLCollisionPlane *plane, PLVector3 *result )
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

bool com_collision_ray_intersect_polygon( const PLCollisionRay *ray, const PLVector3 *vertices, uint numVertices, PLVector3 *result )
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
