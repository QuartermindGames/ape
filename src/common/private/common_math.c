// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Common math methods.
// Author:  Mark E. Sowden

#include <plcore/pl_physics.h>

#include "common_private.h"

QmMathVector3f com_math_pitch_yaw_to_position( const float pitch, const float yaw )
{
	QmMathVector3f position = qm_math_vector3f( 1.0f, pitch, 0.0f );
	PLMatrix4      matrix   = PlMatrix4Identity();
	PLMatrix4      m2;
	m2         = PlTranslateMatrix4( position );
	matrix     = PlMultiplyMatrix4( &m2, &matrix );
	m2         = PlRotateMatrix4( PL_DEG2RAD( yaw ), &QM_MATH_VECTOR3F( 0.0f, 1.0f, 0.0f ) );
	matrix     = PlMultiplyMatrix4( &m2, &matrix );
	position.x = matrix.m[ 0 ];
	position.z = matrix.m[ 8 ];
	return position;
}
