// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <float.h>

#include <plcore/pl_physics.h>

#include "common_private.h"
#include "common/common_math.h"

// I'm not going to lie, much of this is stolen from various books,
// and I'm absolutely clueless how much of it works... so don't ask

static constexpr float EPSILON = 1e-6f;

static PLVector3 closest_point_on_line_segment( const PLVector3 *a, const PLVector3 *b, const PLVector3 *point )
{
	PLVector3 ab = PlSubtractVector3( *b, *a );
	float     t  = PlVector3DotProduct( PlSubtractVector3( *point, *a ), ab ) / PlVector3DotProduct( ab, ab );
	return PlScaleVector3( PlAddVector3F( *a, fminf( fmaxf( t, 0.0f ), 1.0f ) ), ab );
}

static PLVector2 compute_polygon_vertical_bounds( const PLVector3 *vertices, unsigned int numVertices )
{
	// compute the maximum and minimum y of the plane
	float maxY = vertices[ 0 ].y, minY = vertices[ 0 ].y;
	for ( unsigned int i = 1; i < numVertices; ++i )
	{
		if ( vertices[ i ].y > maxY )
		{
			maxY = vertices[ i ].y;
		}
		if ( vertices[ i ].y < minY )
		{
			minY = vertices[ i ].y;
		}
	}

	return ( PLVector2 ) { minY, maxY };
}

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
		PLVector3 min = qm_math_vector3f( fmaxf( a->mins.x, b->mins.x ),
		                                  fmaxf( a->mins.y, b->mins.y ),
		                                  fmaxf( a->mins.z, b->mins.z ) );
		PLVector3 max = qm_math_vector3f( fminf( a->maxs.x, b->maxs.x ),
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

bool com_collision_sphere_intersect_polygon( const PLCollisionSphere *sphere, const PLVector3 *normal, const PLVector3 *vertices, unsigned int numVertices, PLVector3 *result )
{
	static constexpr unsigned int MAX_EDGES = 16;
	assert( numVertices < MAX_EDGES );
	if ( numVertices < 3 )
	{
		return false;
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
	com_math_plane_basis_vectors( &( ComMathPlane ) { .normal = *normal }, &tangent, &bitangent );

	PLVector2 poly2D[ MAX_EDGES ];
	for ( unsigned int i = 0; i < numVertices; ++i )
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
		if ( edgeLengthSq < EPSILON )
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
// Cylinder
/////////////////////////////////////////////////////////////////////////////////////

float com_collision_cylinder_get_top( const ComCollisionCylinder *cylinder )
{
	return cylinder->origin.y + cylinder->height;
}

bool com_collision_cylinder_intersect_point( const ComCollisionCylinder *cylinder, const PLVector3 *point )
{
	float top = com_collision_cylinder_get_top( cylinder );
	if ( point->y < PL_MIN( cylinder->origin.y, top ) )
	{
		return false;
	}
	if ( point->y > PL_MAX( cylinder->origin.y, top ) )
	{
		return false;
	}

	PLVector2 x = ( PLVector2 ) { cylinder->origin.x, cylinder->origin.z };
	PLVector2 y = ( PLVector2 ) { point->x, point->z };
	if ( PlVector2Distance( &x, &y ) <= cylinder->radius )
	{
		return true;
	}

	return false;
}

bool com_collision_cylinder_intersect_polygon( const ComCollisionCylinder *cylinder, const PLVector3 *vertices, unsigned int numVertices, const PLVector3 *normal )
{
	if ( numVertices < 3 )
	{
		return false;
	}

	// compute the maximum and minimum y of the plane
	PLVector2 vbounds = compute_polygon_vertical_bounds( vertices, numVertices );

	// check if the cylinder is higher or lower than the plane
	float top = com_collision_cylinder_get_top( cylinder );
	if ( top < vbounds.x || cylinder->origin.y > vbounds.y )
	{
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Capsule
/////////////////////////////////////////////////////////////////////////////////////

bool com_collision_capsule_intersect_polygon( const ComCollisionCapsule *capsule, const PLVector3 *normal, const PLVector3 *vertices, unsigned int numVertices, PLVector3 *result )
{
	static constexpr unsigned int MAX_EDGES = 16;
	assert( numVertices < MAX_EDGES );
	if ( numVertices < 3 )
	{
		return false;
	}

	float D = -PlVector3DotProduct( *normal, vertices[ 0 ] );

	PLVector3 a = capsule->origin;
	PLVector3 b = capsule->end;

	float da = PlVector3DotProduct( *normal, a ) + D;
	float db = PlVector3DotProduct( *normal, b ) + D;

	PLVector3 closestPoint;
	float     distanceToPlane;

	float denom = db - da;
	if ( fabsf( denom ) < EPSILON )
	{
		if ( fabsf( da ) < fabsf( db ) )
		{
			closestPoint    = a;
			distanceToPlane = da;
		}
		else
		{
			closestPoint    = b;
			distanceToPlane = db;
		}
	}
	else
	{
		float t = -da / denom;
		if ( t < 0.0f )
		{
			closestPoint    = a;
			distanceToPlane = da;
		}
		else if ( t > 1.0f )
		{
			closestPoint    = b;
			distanceToPlane = db;
		}
		else
		{
			PLVector3 segment = PlSubtractVector3( b, a );
			closestPoint      = PlAddVector3( a, PlScaleVector3F( segment, t ) );
			distanceToPlane   = da + t * denom;
		}
	}

	if ( fabsf( distanceToPlane ) > capsule->radius )
	{
		return false;
	}

	PLVector3 projectedPoint = PlSubtractVector3( closestPoint, PlScaleVector3F( *normal, distanceToPlane ) );

	PLVector3 tangent, bitangent;
	com_math_plane_basis_vectors( &( ComMathPlane ) { .normal = *normal }, &tangent, &bitangent );

	PLVector2 poly2D[ MAX_EDGES ];
	for ( unsigned int i = 0; i < numVertices; ++i )
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

	float     minDistanceSq = capsule->radius * capsule->radius;
	PLVector3 returnHit     = projectedPoint;

	bool hit = false;
	for ( int i = 0; i < numVertices; i++ )
	{
		PLVector3 v0 = vertices[ i ];
		PLVector3 v1 = vertices[ ( i + 1 ) % numVertices ];

		PLVector3 edge         = PlSubtractVector3( v1, v0 );
		PLVector3 toPoint      = PlSubtractVector3( projectedPoint, v0 );
		float     edgeLengthSq = PlVector3DotProduct( edge, edge );
		if ( edgeLengthSq < EPSILON )
		{
			continue;
		}

		float t = PlVector3DotProduct( toPoint, edge ) / edgeLengthSq;
		t       = fmaxf( 0.0f, fminf( 1.0f, t ) );

		PLVector3 closestOnEdge = PlAddVector3( v0, PlScaleVector3F( edge, t ) );
		PLVector3 diff          = PlSubtractVector3( closestOnEdge, closestPoint );
		float     distanceSq    = PlVector3DotProduct( diff, diff );

		if ( distanceSq < minDistanceSq )
		{
			minDistanceSq = distanceSq;
			returnHit     = closestOnEdge;
			hit           = true;
		}

		PLVector3 vertexDiff       = PlSubtractVector3( v0, closestPoint );
		float     vertexDistanceSq = PlVector3DotProduct( vertexDiff, vertexDiff );
		if ( vertexDistanceSq < minDistanceSq )
		{
			minDistanceSq = vertexDistanceSq;
			returnHit     = v0;
			hit           = true;
		}
	}

	if ( hit )
	{
		if ( result != nullptr )
		{
			*result = returnHit;
		}
		return true;
	}

	return false;
}

bool com_collision_aabb_intersect_polygon( const PLCollisionAABB *aabb, const PLVector3 *normal,
                                           const PLVector3 *vertices, unsigned int numVertices, PLVector3 *result )
{
	static constexpr unsigned int MAX_EDGES = 16;
	assert( numVertices < MAX_EDGES );
	if ( numVertices < 3 )
	{
		return false;
	}

	PLVector3 aabbMin     = PlAddVector3( aabb->origin, aabb->mins );
	PLVector3 aabbMax     = PlAddVector3( aabb->origin, aabb->maxs );
	PLVector3 aabbCenter  = PlScaleVector3F( PlAddVector3( aabbMin, aabbMax ), 0.5f );
	PLVector3 halfExtents = PlScaleVector3F( PlSubtractVector3( aabbMax, aabbMin ), 0.5f );

	float D = -PlVector3DotProduct( *normal, vertices[ 0 ] );

	float distance = PlVector3DotProduct( *normal, aabbCenter ) + D;
	float radius   = halfExtents.x * fabsf( normal->x ) +
	               halfExtents.y * fabsf( normal->y ) +
	               halfExtents.z * fabsf( normal->z );

	if ( distance > radius || distance < -radius )
	{
		return false;
	}

	PLVector3 tangent, bitangent;
	com_math_plane_basis_vectors( &( ComMathPlane ) { .normal = *normal }, &tangent, &bitangent );

	float minT = PlVector3DotProduct( aabbMin, tangent );
	float maxT = PlVector3DotProduct( aabbMax, tangent );
	float minB = PlVector3DotProduct( aabbMin, bitangent );
	float maxB = PlVector3DotProduct( aabbMax, bitangent );

	PLVector2 poly2D[ MAX_EDGES ];
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		poly2D[ i ].x = PlVector3DotProduct( vertices[ i ], tangent );
		poly2D[ i ].y = PlVector3DotProduct( vertices[ i ], bitangent );
	}

	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		if ( poly2D[ i ].x >= minT && poly2D[ i ].x <= maxT &&
		     poly2D[ i ].y >= minB && poly2D[ i ].y <= maxB )
		{
			if ( result ) *result = vertices[ i ];
			return true;
		}
	}

	PLVector2 aabbPoints[ 4 ] = {
	        {minT, minB},
	        {maxT, minB},
	        {maxT, maxB},
	        {minT, maxB}
    };

	for ( int i = 0; i < 4; ++i )
	{
		bool inside = false;
		for ( int j = 0, k = numVertices - 1; j < numVertices; k = j++ )
		{
			PLVector2 vi = poly2D[ j ];
			PLVector2 vk = poly2D[ k ];

			if ( ( ( vi.y > aabbPoints[ i ].y ) != ( vk.y > aabbPoints[ i ].y ) ) &&
			     ( aabbPoints[ i ].x < ( vk.x - vi.x ) * ( aabbPoints[ i ].y - vi.y ) / ( vk.y - vi.y ) + vi.x ) )
			{
				inside = !inside;
			}
		}
		if ( inside )
		{
			if ( result )
			{
				*result = PlAddVector3( aabbCenter,
				                        PlAddVector3( PlScaleVector3F( tangent, aabbPoints[ i ].x - aabbCenter.x ),
				                                      PlScaleVector3F( bitangent, aabbPoints[ i ].y - aabbCenter.y ) ) );
			}
			return true;
		}
	}

	return false;
}

bool com_collision_sphere_intersect_sphere( const PLCollisionSphere *sphere, const PLCollisionSphere *sphere2, PLVector3 *result )
{
	PLVector3 difference = PlSubtractVector3( sphere2->origin, sphere->origin );
	float     distance   = PlVector3Length( difference );
	float     r1         = sphere->radius;
	float     r2         = sphere2->radius;
	float     sumRadius  = r1 + r2;
	float     diffRadius = fabsf( r1 - r2 );

	if ( distance > sumRadius || distance < diffRadius )
	{
		return false;
	}

	float     h = ( distance * distance + r1 * r1 - r2 * r2 ) / ( 2 * distance );
	PLVector3 P = PlAddVector3( sphere->origin, PlScaleVector3F( difference, h / distance ) );

	float a_squared = r1 * r1 - h * h;
	if ( a_squared < EPSILON )
	{
		*result = P;
		return true;
	}
	float a = sqrtf( a_squared );

	PLVector3 dir = PlNormalizeVector3( difference );

	PLVector3 perp = PlVector3CrossProduct( dir, ( PLVector3 ) { 0.0f, 1.0f, 0.0f } );
	if ( PlVector3Length( perp ) < EPSILON )
	{
		perp = PlVector3CrossProduct( dir, ( PLVector3 ) { 0.0f, 0.0f, 1.0f } );
	}
	perp = PlNormalizeVector3( perp );

	*result = PlAddVector3( P, PlScaleVector3F( perp, a ) );
	return true;
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

bool com_collision_ray_intersect_polygon( const PLCollisionRay *ray, const PLVector3 *vertices, unsigned int numVertices, PLVector3 *result )
{
	if ( numVertices < 3 )
	{
		return false;
	}

	for ( unsigned int i = 0; i < numVertices - 1; ++i )
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

/////////////////////////////////////////////////////////////////////////////////////
// 2D Collision
/////////////////////////////////////////////////////////////////////////////////////

bool com_collision_point_intersect_recti32( const PLVector2 *point, const ComMathRectI32 *rect )
{
	if ( point->x <= rect->x )
	{
		return false;
	}
	if ( point->x >= rect->x + rect->w )
	{
		return false;
	}

	if ( point->y <= rect->y )
	{
		return false;
	}
	if ( point->y >= rect->y + rect->h )
	{
		return false;
	}

	return true;
}
