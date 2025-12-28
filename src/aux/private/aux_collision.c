// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <float.h>

#include "qmmath/public/qm_math_plane.h"

#include <plcore/pl_physics.h>

#include "aux_private.h"
#include "../public/aux_math.h"

// I'm not going to lie, much of this is stolen from various books,
// and I'm absolutely clueless how much of it works... so don't ask

static constexpr float EPSILON = 1e-6f;

static QmMathVector3f closest_point_on_line_segment( const QmMathVector3f *a, const QmMathVector3f *b, const QmMathVector3f *point )
{
	QmMathVector3f ab = qm_math_vector3f_sub( *b, *a );
	float          t  = qm_math_vector3f_dot_product( qm_math_vector3f_sub( *point, *a ), ab ) / qm_math_vector3f_dot_product( ab, ab );
	return qm_math_vector3f_scale( qm_math_vector3f_add_float( *a, fminf( fmaxf( t, 0.0f ), 1.0f ) ), ab );
}

static QmMathVector2f compute_polygon_vertical_bounds( const QmMathVector3f *vertices, const unsigned int numVertices )
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

	return qm_math_vector2f( minY, maxY );
}

bool com_collision_aabb_intersect_aabb( const PLCollisionAABB *a, const PLCollisionAABB *b, QmMathVector3f *result )
{
	QmMathVector3f maxA = qm_math_vector3f_add( a->maxs, a->origin );
	QmMathVector3f minA = qm_math_vector3f_add( a->mins, a->origin );
	QmMathVector3f maxB = qm_math_vector3f_add( b->maxs, b->origin );
	QmMathVector3f minB = qm_math_vector3f_add( b->mins, b->origin );

	bool hit = ( minA.x <= maxB.x && maxA.x >= minB.x ) &&
	           ( minA.y <= maxB.y && maxA.y >= minB.y ) &&
	           ( minA.z <= maxB.z && maxA.z >= minB.z );
	if ( hit && result != nullptr )
	{
		QmMathVector3f min = qm_math_vector3f( fmaxf( a->mins.x, b->mins.x ),
		                                       fmaxf( a->mins.y, b->mins.y ),
		                                       fmaxf( a->mins.z, b->mins.z ) );
		QmMathVector3f max = qm_math_vector3f( fminf( a->maxs.x, b->maxs.x ),
		                                       fminf( a->maxs.y, b->maxs.y ),
		                                       fminf( a->maxs.z, b->maxs.z ) );

		*result = qm_math_vector3f_scale_float( qm_math_vector3f_add( min, max ), 0.5f );
	}

	return hit;
}

bool com_collision_sphere_intersect_aabb( const PLCollisionSphere *sphere, const PLCollisionAABB *aabb, QmMathVector3f *result )
{
	QmMathVector3f aabbMin = qm_math_vector3f_add( aabb->mins, aabb->origin );
	QmMathVector3f aabbMax = qm_math_vector3f_add( aabb->maxs, aabb->origin );

	QmMathVector3f closestPoint;
	closestPoint.x = fmaxf( aabbMin.x, fminf( sphere->origin.x, aabbMax.x ) );
	closestPoint.y = fmaxf( aabbMin.y, fminf( sphere->origin.y, aabbMax.y ) );
	closestPoint.z = fmaxf( aabbMin.z, fminf( sphere->origin.z, aabbMax.z ) );

	QmMathVector3f diff       = qm_math_vector3f_sub( closestPoint, sphere->origin );
	float          distanceSq = qm_math_vector3f_dot_product( diff, diff );
	bool           hit        = distanceSq <= ( sphere->radius * sphere->radius );
	if ( hit && result != nullptr )
	{
		*result = closestPoint;
	}

	return hit;
}

bool com_collision_sphere_intersect_polygon( const PLCollisionSphere *sphere, const QmMathVector3f *normal, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f *result )
{
	static constexpr unsigned int MAX_EDGES = 16;
	assert( numVertices < MAX_EDGES );
	if ( numVertices < 3 )
	{
		return false;
	}

	// Plane equation: Ax + By + Cz + D = 0
	float D = -qm_math_vector3f_dot_product( *normal, vertices[ 0 ] );

	float distanceToPlane = qm_math_vector3f_dot_product( *normal, sphere->origin ) + D;

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

	QmMathVector3f projectedPoint = qm_math_vector3f_sub( sphere->origin, qm_math_vector3f_scale_float( *normal, distanceToPlane ) );

	QmMathVector3f tangent, bitangent;
	qm_math_plane_basis_vectors( &( QmMathPlane ) { .normal = *normal }, &tangent, &bitangent );

	QmMathVector2f poly2D[ MAX_EDGES ];
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		QmMathVector3f delta = qm_math_vector3f_sub( vertices[ i ], projectedPoint );
		poly2D[ i ].x        = qm_math_vector3f_dot_product( delta, tangent );
		poly2D[ i ].y        = qm_math_vector3f_dot_product( delta, bitangent );
	}
	bool inside = false;
	for ( int i = 0, j = numVertices - 1; i < numVertices; j = i++ )
	{
		static constexpr QmMathVector2f point2D = {};

		QmMathVector2f vi = poly2D[ i ];
		QmMathVector2f vj = poly2D[ j ];
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

	float          minDistanceSq = sphere->radius * sphere->radius;
	QmMathVector3f closestPoint  = projectedPoint;

	bool hit = false;
	for ( int i = 0; i < numVertices; i++ )
	{
		QmMathVector3f v0 = vertices[ i ];
		QmMathVector3f v1 = vertices[ ( i + 1 ) % numVertices ];

		QmMathVector3f edge         = qm_math_vector3f_sub( v1, v0 );
		QmMathVector3f toPoint      = qm_math_vector3f_sub( projectedPoint, v0 );
		float          edgeLengthSq = qm_math_vector3f_dot_product( edge, edge );
		if ( edgeLengthSq < EPSILON )
		{
			continue;
		}

		float t = qm_math_vector3f_dot_product( toPoint, edge ) / edgeLengthSq;
		t       = fmaxf( 0.0f, fminf( 1.0f, t ) );

		QmMathVector3f closestOnEdge = qm_math_vector3f_add( v0, qm_math_vector3f_scale_float( edge, t ) );
		QmMathVector3f diff          = qm_math_vector3f_sub( closestOnEdge, sphere->origin );
		float          distanceSq    = qm_math_vector3f_dot_product( diff, diff );

		if ( distanceSq < minDistanceSq )
		{
			minDistanceSq = distanceSq;
			closestPoint  = closestOnEdge;
			hit           = true;
		}

		QmMathVector3f vertexDiff       = qm_math_vector3f_sub( v0, sphere->origin );
		float          vertexDistanceSq = qm_math_vector3f_dot_product( vertexDiff, vertexDiff );
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

bool com_collision_cylinder_intersect_point( const ComCollisionCylinder *cylinder, const QmMathVector3f *point )
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

	QmMathVector2f x = qm_math_vector2f( cylinder->origin.x, cylinder->origin.z );
	QmMathVector2f y = qm_math_vector2f( point->x, point->z );
	if ( qm_math_vector2f_distance( x, y ) <= cylinder->radius )
	{
		return true;
	}

	return false;
}

bool com_collision_cylinder_intersect_polygon( const ComCollisionCylinder *cylinder, const QmMathVector3f *vertices, unsigned int numVertices, const QmMathVector3f *normal )
{
	if ( numVertices < 3 )
	{
		return false;
	}

	// compute the maximum and minimum y of the plane
	QmMathVector2f vbounds = compute_polygon_vertical_bounds( vertices, numVertices );

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

bool com_collision_capsule_intersect_polygon( const ComCollisionCapsule *capsule, const QmMathVector3f *normal, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f *result )
{
	static constexpr unsigned int MAX_EDGES = 16;
	assert( numVertices < MAX_EDGES );
	if ( numVertices < 3 )
	{
		return false;
	}

	float D = -qm_math_vector3f_dot_product( *normal, vertices[ 0 ] );

	QmMathVector3f a = capsule->origin;
	QmMathVector3f b = capsule->end;

	float da = qm_math_vector3f_dot_product( *normal, a ) + D;
	float db = qm_math_vector3f_dot_product( *normal, b ) + D;

	QmMathVector3f closestPoint;
	float          distanceToPlane;

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
			QmMathVector3f segment = qm_math_vector3f_sub( b, a );
			closestPoint           = qm_math_vector3f_add( a, qm_math_vector3f_scale_float( segment, t ) );
			distanceToPlane        = da + t * denom;
		}
	}

	if ( fabsf( distanceToPlane ) > capsule->radius )
	{
		return false;
	}

	QmMathVector3f projectedPoint = qm_math_vector3f_sub( closestPoint, qm_math_vector3f_scale_float( *normal, distanceToPlane ) );

	QmMathVector3f tangent, bitangent;
	qm_math_plane_basis_vectors( &( QmMathPlane ) { .normal = *normal }, &tangent, &bitangent );

	QmMathVector2f poly2D[ MAX_EDGES ];
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		QmMathVector3f delta = qm_math_vector3f_sub( vertices[ i ], projectedPoint );
		poly2D[ i ].x        = qm_math_vector3f_dot_product( delta, tangent );
		poly2D[ i ].y        = qm_math_vector3f_dot_product( delta, bitangent );
	}
	bool inside = false;
	for ( int i = 0, j = numVertices - 1; i < numVertices; j = i++ )
	{
		static constexpr QmMathVector2f point2D = {};

		QmMathVector2f vi = poly2D[ i ];
		QmMathVector2f vj = poly2D[ j ];
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

	float          minDistanceSq = capsule->radius * capsule->radius;
	QmMathVector3f returnHit     = projectedPoint;

	bool hit = false;
	for ( int i = 0; i < numVertices; i++ )
	{
		QmMathVector3f v0 = vertices[ i ];
		QmMathVector3f v1 = vertices[ ( i + 1 ) % numVertices ];

		QmMathVector3f edge         = qm_math_vector3f_sub( v1, v0 );
		QmMathVector3f toPoint      = qm_math_vector3f_sub( projectedPoint, v0 );
		float          edgeLengthSq = qm_math_vector3f_dot_product( edge, edge );
		if ( edgeLengthSq < EPSILON )
		{
			continue;
		}

		float t = qm_math_vector3f_dot_product( toPoint, edge ) / edgeLengthSq;
		t       = fmaxf( 0.0f, fminf( 1.0f, t ) );

		QmMathVector3f closestOnEdge = qm_math_vector3f_add( v0, qm_math_vector3f_scale_float( edge, t ) );
		QmMathVector3f diff          = qm_math_vector3f_sub( closestOnEdge, closestPoint );
		float          distanceSq    = qm_math_vector3f_dot_product( diff, diff );

		if ( distanceSq < minDistanceSq )
		{
			minDistanceSq = distanceSq;
			returnHit     = closestOnEdge;
			hit           = true;
		}

		QmMathVector3f vertexDiff       = qm_math_vector3f_sub( v0, closestPoint );
		float          vertexDistanceSq = qm_math_vector3f_dot_product( vertexDiff, vertexDiff );
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

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

bool com_collision_aabb_intersect_polygon( const PLCollisionAABB *aabb, const QmMathVector3f *normal,
                                           const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f *result )
{
	static constexpr unsigned int MAX_EDGES = 16;
	assert( numVertices < MAX_EDGES );
	if ( numVertices < 3 )
	{
		return false;
	}

	QmMathVector3f aabbMin     = qm_math_vector3f_add( aabb->origin, aabb->mins );
	QmMathVector3f aabbMax     = qm_math_vector3f_add( aabb->origin, aabb->maxs );
	QmMathVector3f aabbCenter  = qm_math_vector3f_scale_float( qm_math_vector3f_add( aabbMin, aabbMax ), 0.5f );
	QmMathVector3f halfExtents = qm_math_vector3f_scale_float( qm_math_vector3f_sub( aabbMax, aabbMin ), 0.5f );

	float D = -qm_math_vector3f_dot_product( *normal, vertices[ 0 ] );

	float distance = qm_math_vector3f_dot_product( *normal, aabbCenter ) + D;
	float radius   = halfExtents.x * fabsf( normal->x ) +
	               halfExtents.y * fabsf( normal->y ) +
	               halfExtents.z * fabsf( normal->z );

	if ( distance > radius || distance < -radius )
	{
		return false;
	}

	QmMathVector3f tangent, bitangent;
	qm_math_plane_basis_vectors( &( QmMathPlane ) { .normal = *normal }, &tangent, &bitangent );

	float minT = qm_math_vector3f_dot_product( aabbMin, tangent );
	float maxT = qm_math_vector3f_dot_product( aabbMax, tangent );
	float minB = qm_math_vector3f_dot_product( aabbMin, bitangent );
	float maxB = qm_math_vector3f_dot_product( aabbMax, bitangent );

	QmMathVector2f poly2D[ MAX_EDGES ];
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		poly2D[ i ].x = qm_math_vector3f_dot_product( vertices[ i ], tangent );
		poly2D[ i ].y = qm_math_vector3f_dot_product( vertices[ i ], bitangent );
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

	QmMathVector2f aabbPoints[ 4 ] = {
	        {.x = minT, .y = minB},
	        {.x = maxT, .y = minB},
	        {.x = maxT, .y = maxB},
	        {.x = minT, .y = maxB}
    };

	for ( int i = 0; i < 4; ++i )
	{
		bool inside = false;
		for ( int j = 0, k = numVertices - 1; j < numVertices; k = j++ )
		{
			QmMathVector2f vi = poly2D[ j ];
			QmMathVector2f vk = poly2D[ k ];

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
				*result = qm_math_vector3f_add( aabbCenter,
				                                qm_math_vector3f_add( qm_math_vector3f_scale_float( tangent, aabbPoints[ i ].x - aabbCenter.x ),
				                                                      qm_math_vector3f_scale_float( bitangent, aabbPoints[ i ].y - aabbCenter.y ) ) );
			}
			return true;
		}
	}

	return false;
}

bool com_collision_sphere_intersect_sphere( const PLCollisionSphere *sphere, const PLCollisionSphere *sphere2, QmMathVector3f *result )
{
	QmMathVector3f difference = qm_math_vector3f_sub( sphere2->origin, sphere->origin );
	float          distance   = qm_math_vector3f_length( difference );
	float          r1         = sphere->radius;
	float          r2         = sphere2->radius;
	float          sumRadius  = r1 + r2;
	float          diffRadius = fabsf( r1 - r2 );

	if ( distance > sumRadius || distance < diffRadius )
	{
		return false;
	}

	float          h = ( distance * distance + r1 * r1 - r2 * r2 ) / ( 2 * distance );
	QmMathVector3f P = qm_math_vector3f_add( sphere->origin, qm_math_vector3f_scale_float( difference, h / distance ) );

	float a_squared = r1 * r1 - h * h;
	if ( a_squared < EPSILON )
	{
		*result = P;
		return true;
	}
	float a = sqrtf( a_squared );

	QmMathVector3f dir = qm_math_vector3f_normalize( difference );

	QmMathVector3f perp = qm_math_vector3f_cross_product( dir, QM_MATH_VECTOR3F( 0.0f, 1.0f, 0.0f ) );
	if ( qm_math_vector3f_length( perp ) < EPSILON )
	{
		perp = qm_math_vector3f_cross_product( dir, QM_MATH_VECTOR3F( 0.0f, 0.0f, 1.0f ) );
	}
	perp = qm_math_vector3f_normalize( perp );

	*result = qm_math_vector3f_add( P, qm_math_vector3f_scale_float( perp, a ) );
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Ray Casting
/////////////////////////////////////////////////////////////////////////////////////

bool com_collision_ray_intersect_aabb( const PLCollisionRay *ray, const PLCollisionAABB *aabb, QmMathVector3f *result )
{
	return PlIsRayIntersectingAabb( aabb, ray, result );
}

bool com_collision_ray_intersect_plane( const PLCollisionRay *ray, const PLCollisionPlane *plane, QmMathVector3f *result )
{
	float denom = qm_math_vector3f_dot_product( plane->normal, ray->direction );
	if ( denom >= 0.0f || fabsf( denom ) < 1e-6f )
	{
		return false;
	}

	float d = qm_math_vector3f_dot_product( plane->normal, plane->origin );
	float t = ( d - qm_math_vector3f_dot_product( plane->normal, ray->origin ) ) / denom;
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

bool com_collision_ray_intersect_polygon( const PLCollisionRay *ray, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f *result )
{
	if ( numVertices < 3 )
	{
		return false;
	}

	for ( unsigned int i = 0; i < numVertices - 1; ++i )
	{
		const QmMathVector3f *v0 = &vertices[ 0 ];
		const QmMathVector3f *v1 = &vertices[ i ];
		const QmMathVector3f *v2 = &vertices[ i + 1 ];

		QmMathVector3f edge1  = qm_math_vector3f_sub( *v1, *v0 );
		QmMathVector3f edge2  = qm_math_vector3f_sub( *v2, *v0 );
		QmMathVector3f normal = qm_math_vector3f_cross_product( edge1, edge2 );

		float denom = qm_math_vector3f_dot_product( normal, ray->direction );
		if ( denom >= 0.0f || fabsf( denom ) < 1e-6 )
		{
			continue;
		}

		float d = qm_math_vector3f_dot_product( normal, *v0 );
		float t = ( d - qm_math_vector3f_dot_product( normal, ray->origin ) ) / denom;
		if ( t < 0.0f )
		{
			continue;
		}

		QmMathVector3f intersection = {
		        .x = ray->origin.x + t * ray->direction.x,
		        .y = ray->origin.y + t * ray->direction.y,
		        .z = ray->origin.z + t * ray->direction.z,
		};

		QmMathVector3f point = qm_math_vector3f_sub( intersection, *v0 );

		float area = qm_math_vector3f_dot_product( normal, normal );

		QmMathVector3f c = qm_math_vector3f_cross_product( edge1, point );
		float          a = qm_math_vector3f_dot_product( normal, c ) / area;
		if ( a < 0.0f || a > 1.0f )
		{
			continue;
		}

		QmMathVector3f c2 = qm_math_vector3f_cross_product( point, edge2 );
		float          b  = qm_math_vector3f_dot_product( normal, c2 ) / area;
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

bool com_collision_point_intersect_recti32( const QmMathVector2f *point, const ComMathRectI32 *rect )
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
