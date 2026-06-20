// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <float.h>

#include "qmmath/public/qm_math_plane.h"

#include <plcore/pl_physics.h>

#include "aux_private.h"
#include "../public/aux_math.h"

// I'm not going to lie, much of this is stolen from various books,
// and I'm absolutely clueless how much of it works... so don't ask

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

bool aux_collision_aabb_intersect_aabb( const PLCollisionAABB *self, const PLCollisionAABB *other, QmMathVector3f *result )
{
	QmMathVector3f maxA = qm_math_vector3f_add( self->maxs, self->origin );
	QmMathVector3f minA = qm_math_vector3f_add( self->mins, self->origin );
	QmMathVector3f maxB = qm_math_vector3f_add( other->maxs, other->origin );
	QmMathVector3f minB = qm_math_vector3f_add( other->mins, other->origin );

	bool hit = ( minA.x <= maxB.x && maxA.x >= minB.x ) &&
	           ( minA.y <= maxB.y && maxA.y >= minB.y ) &&
	           ( minA.z <= maxB.z && maxA.z >= minB.z );
	if ( hit && result != nullptr )
	{
		QmMathVector3f min = qm_math_vector3f( fmaxf( self->mins.x, other->mins.x ),
		                                       fmaxf( self->mins.y, other->mins.y ),
		                                       fmaxf( self->mins.z, other->mins.z ) );
		QmMathVector3f max = qm_math_vector3f( fminf( self->maxs.x, other->maxs.x ),
		                                       fminf( self->maxs.y, other->maxs.y ),
		                                       fminf( self->maxs.z, other->maxs.z ) );

		*result = qm_math_vector3f_scale_float( qm_math_vector3f_add( min, max ), 0.5f );
	}

	return hit;
}

bool aux_collision_sphere_intersect_aabb( const PLCollisionSphere *self, const PLCollisionAABB *other, QmMathVector3f *result )
{
	QmMathVector3f aabbMin = qm_math_vector3f_add( other->mins, other->origin );
	QmMathVector3f aabbMax = qm_math_vector3f_add( other->maxs, other->origin );

	QmMathVector3f closestPoint;
	closestPoint.x = fmaxf( aabbMin.x, fminf( self->origin.x, aabbMax.x ) );
	closestPoint.y = fmaxf( aabbMin.y, fminf( self->origin.y, aabbMax.y ) );
	closestPoint.z = fmaxf( aabbMin.z, fminf( self->origin.z, aabbMax.z ) );

	QmMathVector3f diff       = qm_math_vector3f_sub( closestPoint, self->origin );
	float          distanceSq = qm_math_vector3f_dot_product( diff, diff );
	bool           hit        = distanceSq <= ( self->radius * self->radius );
	if ( hit && result != nullptr )
	{
		*result = closestPoint;
	}

	return hit;
}

bool aux_collision_sphere_intersect_polygon( const PLCollisionSphere *self, const QmMathVector3f *normal, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f *result )
{
	static constexpr unsigned int MAX_EDGES = 16;
	assert( numVertices < MAX_EDGES );
	if ( numVertices < 3 )
	{
		return false;
	}

	// Plane equation: Ax + By + Cz + D = 0
	float D = -qm_math_vector3f_dot_product( *normal, vertices[ 0 ] );

	float distanceToPlane = qm_math_vector3f_dot_product( *normal, self->origin ) + D;

	// only test against the front of the plane, not the back
	// todo: make this optional
	if ( distanceToPlane < 0.0f )
	{
		return false;
	}

	if ( fabsf( distanceToPlane ) > self->radius )
	{
		return false;
	}

	QmMathVector3f projectedPoint = qm_math_vector3f_sub( self->origin, qm_math_vector3f_scale_float( *normal, distanceToPlane ) );

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

	float          minDistanceSq = self->radius * self->radius;
	QmMathVector3f closestPoint  = projectedPoint;

	bool hit = false;
	for ( int i = 0; i < numVertices; i++ )
	{
		QmMathVector3f v0 = vertices[ i ];
		QmMathVector3f v1 = vertices[ ( i + 1 ) % numVertices ];

		QmMathVector3f edge         = qm_math_vector3f_sub( v1, v0 );
		QmMathVector3f toPoint      = qm_math_vector3f_sub( projectedPoint, v0 );
		float          edgeLengthSq = qm_math_vector3f_dot_product( edge, edge );
		if ( edgeLengthSq < QM_MATH_EPSILON )
		{
			continue;
		}

		float t = qm_math_vector3f_dot_product( toPoint, edge ) / edgeLengthSq;
		t       = fmaxf( 0.0f, fminf( 1.0f, t ) );

		QmMathVector3f closestOnEdge = qm_math_vector3f_add( v0, qm_math_vector3f_scale_float( edge, t ) );
		QmMathVector3f diff          = qm_math_vector3f_sub( closestOnEdge, self->origin );
		float          distanceSq    = qm_math_vector3f_dot_product( diff, diff );

		if ( distanceSq < minDistanceSq )
		{
			minDistanceSq = distanceSq;
			closestPoint  = closestOnEdge;
			hit           = true;
		}

		QmMathVector3f vertexDiff       = qm_math_vector3f_sub( v0, self->origin );
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

float aux_collision_cylinder_get_top( const ComCollisionCylinder *self )
{
	return self->origin.y + self->height;
}

bool aux_collision_cylinder_intersect_cylinder( const ComCollisionCylinder *self, const ComCollisionCylinder *other )
{
	if ( other->origin.y > aux_collision_cylinder_get_top( self ) || self->origin.y > aux_collision_cylinder_get_top( other ) )
	{
		return false;
	}

	// the nice thing about a cylinder is we just now need to check the radius against the other radius,
	// and not worry about that silly 3D nonsense (oh we do, god we do... just not here thankfully)

	QmMathVector2f selfPos  = QM_MATH_VECTOR2F( self->origin.x, self->origin.z );
	QmMathVector2f otherPos = QM_MATH_VECTOR2F( other->origin.x, other->origin.z );

	float distance = qm_math_vector2f_distance( selfPos, otherPos );
	return distance <= self->radius + other->radius;
}

bool aux_collision_cylinder_intersect_aabb( const ComCollisionCylinder *self, const PLCollisionAABB *other, QmMathVector3f *result )
{
	QmMathVector3f aabbMin = qm_math_vector3f_add( other->mins, other->origin );
	QmMathVector3f aabbMax = qm_math_vector3f_add( other->maxs, other->origin );

	if ( aux_collision_cylinder_get_top( self ) < aabbMin.y || self->origin.y > aabbMax.y )
	{
		return false;
	}

	// okay, and now all we basically gotta do is intersect a circle with a square in 2D

	QmMathVector2f selfPos = QM_MATH_VECTOR2F( self->origin.x, self->origin.z );

	QmMathVector2f closestPoint;
	closestPoint.x = fmaxf( aabbMin.x, fminf( self->origin.x, aabbMax.x ) );
	closestPoint.y = fmaxf( aabbMin.z, fminf( self->origin.z, aabbMax.z ) );

	float distance = qm_math_vector2f_distance( selfPos, closestPoint );

	bool hit = distance <= self->radius;
	if ( hit && result != nullptr )
	{
		*result = QM_MATH_VECTOR3F( closestPoint.x, other->origin.y, closestPoint.y );
	}

	return hit;
}

bool aux_collision_cylinder_intersect_point( const ComCollisionCylinder *self, const QmMathVector3f *point )
{
	float top = aux_collision_cylinder_get_top( self );
	if ( point->y < QM_OS_MIN( self->origin.y, top ) || point->y > QM_OS_MAX( self->origin.y, top ) )
	{
		return false;
	}

	QmMathVector2f x = qm_math_vector2f( self->origin.x, self->origin.z );
	QmMathVector2f y = qm_math_vector2f( point->x, point->z );
	if ( qm_math_vector2f_distance( x, y ) <= self->radius )
	{
		return true;
	}

	return false;
}

bool aux_collision_cylinder_intersect_polygon( const ComCollisionCylinder *self, const QmMathVector3f *normal, const QmMathVector3f *vertices, unsigned int numVertices )
{
	if ( numVertices < 3 )
	{
		return false;
	}

	// compute the maximum and minimum y of the plane
	QmMathVector2f vbounds = compute_polygon_vertical_bounds( vertices, numVertices );

	// check if the cylinder is higher or lower than the plane
	if ( aux_collision_cylinder_get_top( self ) < vbounds.x || self->origin.y > vbounds.y )
	{
		return false;
	}

	return true;
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
	if ( a_squared < QM_MATH_EPSILON )
	{
		if ( result != nullptr )
		{
			*result = P;
		}

		return true;
	}

	QmMathVector3f dir  = qm_math_vector3f_normalize( difference );
	QmMathVector3f perp = qm_math_vector3f_cross_product( dir, QM_MATH_VECTOR3F( 0.0f, 1.0f, 0.0f ) );
	if ( qm_math_vector3f_length( perp ) < QM_MATH_EPSILON )
	{
		perp = qm_math_vector3f_cross_product( dir, QM_MATH_VECTOR3F( 0.0f, 0.0f, 1.0f ) );
	}

	perp = qm_math_vector3f_normalize( perp );

	if ( result != nullptr )
	{
		*result = qm_math_vector3f_add( P, qm_math_vector3f_scale_float( perp, sqrtf( a_squared ) ) );
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Ray Casting
/////////////////////////////////////////////////////////////////////////////////////

static bool ray_range_test( const PLCollisionRay *ray, const QmMathVector3f point )
{
	assert( ray->range >= 0.0f );

	// we need to account for the range of the ray
	// this check is a botch to support cases from before we supported range!
	if ( ray->range > 0.0f )
	{
		float pointRange = qm_math_vector3f_distance( ray->origin, point );
		if ( pointRange >= ray->range )
		{
			return false;
		}
	}

	return true;
}

bool com_collision_ray_intersect_aabb( const PLCollisionRay *ray, const PLCollisionAABB *aabb, QmMathVector3f *result )
{
	QmMathVector3f intersection;
	if ( PlIsRayIntersectingAabb( aabb, ray, &intersection ) )
	{
		if ( !ray_range_test( ray, intersection ) )
		{
			return false;
		}

		if ( result != nullptr )
		{
			*result = intersection;
		}

		return true;
	}

	return false;
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

	QmMathVector3f intersection;
	intersection.x = ray->origin.x + t * ray->direction.x;
	intersection.y = ray->origin.y + t * ray->direction.y;
	intersection.z = ray->origin.z + t * ray->direction.z;

	if ( !ray_range_test( ray, intersection ) )
	{
		return false;
	}

	if ( result != nullptr )
	{
		*result = intersection;
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

		if ( !ray_range_test( ray, intersection ) )
		{
			continue;
		}

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

bool com_collision_point_intersect_recti32( const QmMathVector2f *point, const AuxMathRectI32 *rect )
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
