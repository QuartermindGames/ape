// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Common math methods.
// Author:  Mark E. Sowden

#include <plcore/pl_physics.h>

#include "common_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Plane
/////////////////////////////////////////////////////////////////////////////////////

ComMathPlane *com_math_plane_setup( ComMathPlane *self, const QmMathVector3f *p0, const QmMathVector3f *p1, const QmMathVector3f *p2 )
{
	QmMathVector3f s0 = qm_math_vector3f_sub( *p1, *p0 );
	QmMathVector3f s1 = qm_math_vector3f_sub( *p2, *p0 );

	self->normal = qm_math_vector3f_cross_product( s0, s1 );
	self->normal = qm_math_vector3f_normalize( self->normal );

	self->distance = -qm_math_vector3f_dot_product( self->normal, *p0 );

	return self;
}

float com_math_plane_distance( const ComMathPlane *self, const QmMathVector3f *pos )
{
	return qm_math_vector3f_dot_product( self->normal, *pos ) + self->distance;
}

void com_math_plane_basis_vectors( const ComMathPlane *self, QmMathVector3f *tangentDst, QmMathVector3f *bitangentDst )
{
	if ( fabsf( self->normal.x ) > fabsf( self->normal.y ) )
	{
		*tangentDst = qm_math_vector3f( self->normal.z, 0.0f, -self->normal.x );
	}
	else
	{
		*tangentDst = qm_math_vector3f( 0.0f, self->normal.z, -self->normal.y );
	}
	*tangentDst = qm_math_vector3f_normalize( *tangentDst );

	*bitangentDst = qm_math_vector3f_cross_product( self->normal, *tangentDst );
	*bitangentDst = qm_math_vector3f_normalize( *bitangentDst );
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

QmMathVector3f com_math_plane_project_point( const ComMathPlane *self, const QmMathVector3f *point )
{
	float dist = com_math_plane_distance( self, point );
	return qm_math_vector3f_add( *point, qm_math_vector3f_scale_float( self->normal, -dist ) );
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

bool com_math_is_polygon_convex( const QmMathVector2f *vertices, const unsigned int numVertices )
{
	if ( numVertices < 4 )
	{
		return true;
	}

	bool sign = false;
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		QmMathVector2f a;
		a.x = vertices[ ( i + 2 ) % numVertices ].x - vertices[ ( i + 1 ) % numVertices ].x;
		a.y = vertices[ ( i + 2 ) % numVertices ].y - vertices[ ( i + 1 ) % numVertices ].y;

		QmMathVector2f b;
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

QmMathVector3f com_math_compute_face_normal( const QmMathVector3f *vertices, unsigned int numVertices )
{
	QmMathVector3f normal = {};
	for ( unsigned int i = 0; i < numVertices; i += 3 )
	{
		QmMathVector3f a = vertices[ i ];
		QmMathVector3f b = vertices[ i + 1 ];
		QmMathVector3f c = vertices[ i + 2 ];

		QmMathVector3f x = qm_math_vector3f( c.x - b.x, c.y - b.y, c.z - b.z );
		QmMathVector3f y = qm_math_vector3f( a.x - b.x, a.y - b.y, a.z - b.z );
		QmMathVector3f n = qm_math_vector3f_normalize( qm_math_vector3f_cross_product( x, y ) );

		normal = qm_math_vector3f_add( normal, n );
	}

	return qm_math_vector3f_normalize( normal );
}

QmMathVector3f com_math_pitch_yaw_to_position( float pitch, float yaw )
{
	QmMathVector3f position = { 1.0f, pitch, 0.0f };
	PLMatrix4      matrix   = PlMatrix4Identity();
	PLMatrix4      m2;
	m2         = PlTranslateMatrix4( position );
	matrix     = PlMultiplyMatrix4( &m2, &matrix );
	m2         = PlRotateMatrix4( PL_DEG2RAD( yaw ), &( QmMathVector3f ) { 0.0f, 1.0f, 0.0f } );
	matrix     = PlMultiplyMatrix4( &m2, &matrix );
	position.x = matrix.m[ 0 ];
	position.z = matrix.m[ 8 ];
	return position;
}
