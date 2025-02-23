// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_physics.h>

#include "common_private.h"

// I'm not going to lie, much of this is stolen from various books,
// and I'm absolutely clueless how much of it works... so don't ask

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

bool com_collision_sphere_intersect_aabb( const PLCollisionSphere *sphere, const PLCollisionAABB *aabb, PLVector3 *result )
{
	PLVector3 aabbMin = PlAddVector3( aabb->mins, aabb->origin );
	PLVector3 aabbMax = PlAddVector3( aabb->maxs, aabb->origin );

	PLVector3 closestPoint;
	closestPoint.x = fmaxf( aabbMin.x, fminf( sphere->origin.x, aabbMax.x ) );
	closestPoint.y = fmaxf( aabbMin.y, fminf( sphere->origin.y, aabbMax.y ) );
	closestPoint.z = fmaxf( aabbMin.z, fminf( sphere->origin.z, aabbMax.z ) );

	PLVector3 diff       = PlSubtractVector3( closestPoint, sphere->origin );
	float     distanceSq = PlVector3DotProduct( diff, diff );
	bool      hit        = distanceSq <= ( sphere->radius * sphere->radius );
	if ( hit && result != nullptr )
	{
		*result = closestPoint;
	}

	return hit;
}

static void compute_plane_basis_vectors( const PLVector3 *normal, PLVector3 *tangent, PLVector3 *bitangent )
{
	if ( fabsf( normal->x ) > fabsf( normal->y ) )
	{
		*tangent = PL_VECTOR3( normal->z, 0.0f, -normal->x );
	}
	else
	{
		*tangent = PL_VECTOR3( 0.0f, normal->z, -normal->y );
	}
	*tangent = PlNormalizeVector3( *tangent );

	*bitangent = PlVector3CrossProduct( *normal, *tangent );
	*bitangent = PlNormalizeVector3( *bitangent );
}

bool com_collision_sphere_intersect_polygon( const PLCollisionSphere *sphere, const PLVector3 *normal, const PLVector3 *vertices, UInt numVertices, PLVector3 *result )
{
	static constexpr unsigned int MAX_EDGES = 16;
	assert( numVertices < MAX_EDGES );

	if ( numVertices < 3 )
	{
		return false;// Not a valid polygon
	}

	// Plane equation: Ax + By + Cz + D = 0
	float D = -PlVector3DotProduct( *normal, vertices[ 0 ] );

	float distanceToPlane = PlVector3DotProduct( *normal, sphere->origin ) + D;

	// only test against the front of the plane, not the back
	// todo: make this optional
	if ( distanceToPlane < 0.0f )
	{
		return false;
	}

	if ( fabsf( distanceToPlane ) > sphere->radius )
	{
		return false;
	}

	PLVector3 projectedPoint = PlSubtractVector3( sphere->origin, PlScaleVector3F( *normal, distanceToPlane ) );

	PLVector3 tangent, bitangent;
	compute_plane_basis_vectors( normal, &tangent, &bitangent );

	PLVector2 poly2D[ MAX_EDGES ];
	for ( UInt i = 0; i < numVertices; ++i )
	{
		PLVector3 delta = PlSubtractVector3( vertices[ i ], projectedPoint );
		poly2D[ i ].x   = PlVector3DotProduct( delta, tangent );
		poly2D[ i ].y   = PlVector3DotProduct( delta, bitangent );
	}
	bool inside = false;
	for ( int i = 0, j = numVertices - 1; i < numVertices; j = i++ )
	{
		static constexpr PLVector2 point2D = {};

		PLVector2 vi = poly2D[ i ];
		PLVector2 vj = poly2D[ j ];
		if ( ( ( vi.y > point2D.y ) != ( vj.y > point2D.y ) ) &&
		     ( point2D.x < ( vj.x - vi.x ) * ( point2D.y - vi.y ) / ( vj.y - vi.y ) + vi.x ) )
		{
			inside = !inside;
		}
	}

	if ( inside )
	{
		if ( result != nullptr )
		{
			*result = projectedPoint;
		}
		return true;
	}

	float     minDistanceSq = sphere->radius * sphere->radius;
	PLVector3 closestPoint  = projectedPoint;

	bool hit = false;
	for ( int i = 0; i < numVertices; i++ )
	{
		PLVector3 v0 = vertices[ i ];
		PLVector3 v1 = vertices[ ( i + 1 ) % numVertices ];

		PLVector3 edge         = PlSubtractVector3( v1, v0 );
		PLVector3 toPoint      = PlSubtractVector3( projectedPoint, v0 );
		float     edgeLengthSq = PlVector3DotProduct( edge, edge );
		if ( edgeLengthSq < 1e-6f )
		{
			continue;
		}

		float t = PlVector3DotProduct( toPoint, edge ) / edgeLengthSq;
		t       = fmaxf( 0.0f, fminf( 1.0f, t ) );

		PLVector3 closestOnEdge = PlAddVector3( v0, PlScaleVector3F( edge, t ) );
		PLVector3 diff          = PlSubtractVector3( closestOnEdge, sphere->origin );
		float     distanceSq    = PlVector3DotProduct( diff, diff );

		if ( distanceSq < minDistanceSq )
		{
			minDistanceSq = distanceSq;
			closestPoint  = closestOnEdge;
			hit           = true;
		}

		PLVector3 vertexDiff       = PlSubtractVector3( v0, sphere->origin );
		float     vertexDistanceSq = PlVector3DotProduct( vertexDiff, vertexDiff );
		if ( vertexDistanceSq < minDistanceSq )
		{
			minDistanceSq = vertexDistanceSq;
			closestPoint  = v0;
			hit           = true;
		}
	}

	if ( hit )
	{
		if ( result != nullptr )
		{
			*result = closestPoint;
		}
		return true;
	}

	return false;
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

bool com_collision_ray_intersect_polygon( const PLCollisionRay *ray, const PLVector3 *vertices, UInt numVertices, PLVector3 *result )
{
	if ( numVertices < 3 )
	{
		return false;
	}

	for ( UInt i = 0; i < numVertices - 1; ++i )
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
