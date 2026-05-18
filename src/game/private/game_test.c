// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Test framework for game-specific items.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_random.h"

#include "game_private.h"

void game_test_cylinder_aabb_collision_( const QmMathVector3f *pos )
{
	ComCollisionCylinder cylinder = {};
	cylinder.height               = 64.0f;
	cylinder.origin               = qm_math_vector3f( 16.0f, 16.0f, 32.0f );
	cylinder.radius               = 16.0f;

	PLCollisionAABB bounds = {};
	bounds.maxs            = QM_MATH_VECTOR3F( 16.0f, 16.0f, 16.0f );
	bounds.mins            = QM_MATH_VECTOR3F( -16.0f, -16.0f, -16.0f );
	bounds.origin          = *pos;

	bounds.origin.y -= 16.0f;

	QmMathVector3f result;

	QmMathColour4ub colour;
	if ( aux_collision_cylinder_intersect_aabb( &cylinder, &bounds, &result ) )
	{
		colour = QM_MATH_COLOUR4UB( 0, 255, 0, 255 );
		ape_draw_debug_arrow( cylinder.origin, result, colour, 2.0f );
	}
	else
	{
		colour = QM_MATH_COLOUR4UB( 255, 0, 0, 255 );
	}

	ape_draw_debug_cylinder( &cylinder, &colour, 16 );
	ape_draw_debug_aabb( &bounds, colour );
}

void game_test_cylinder_point_collision_( const QmMathVector3f *pos )
{
	ComCollisionCylinder cylinder = {};
	cylinder.height               = 64.0f;
	cylinder.origin               = qm_math_vector3f( 16.0f, 16.0f, 32.0f );
	cylinder.radius               = 16.0f;

	QmMathColour4ub colour;
	if ( aux_collision_cylinder_intersect_point( &cylinder, pos ) )
	{
		colour = QM_MATH_COLOUR4UB( 0, 255, 0, 255 );
	}
	else
	{
		colour = QM_MATH_COLOUR4UB( 255, 0, 0, 255 );
	}

	ape_draw_debug_cylinder( &cylinder, &colour, 16 );
}

void game_test_cylinder_cylinder_collision_( const QmMathVector3f *pos )
{
	ComCollisionCylinder me = {};
	me.height               = 64.0f;
	me.origin               = *pos;
	me.radius               = 16.0f;

	ComCollisionCylinder them = {};
	them.height               = 64.0f;
	them.origin               = qm_math_vector3f( 16.0f, 16.0f, 32.0f );
	them.radius               = 16.0f;

	QmMathColour4ub colour;
	if ( aux_collision_cylinder_intersect_cylinder( &me, &them ) )
	{
		colour = QM_MATH_COLOUR4UB( 0, 255, 0, 255 );
	}
	else
	{
		colour = QM_MATH_COLOUR4UB( 255, 0, 0, 255 );
	}

	ape_draw_debug_cylinder( &me, &colour, 16 );
	ape_draw_debug_cylinder( &them, &PL_COLOUR_AQUA, 16 );
}

void game_test_cylinder_polygon_collision_( const QmMathVector3f *pos )
{
	QmMathVector3f point = *pos;
	point.y -= 32.0f;

	ComCollisionCylinder cylinder = {};
	cylinder.height               = 64.0f;
	cylinder.origin               = point;
	cylinder.radius               = 16.0f;

	static constexpr QmMathVector3f vertices[] = {
	        QM_MATH_VECTOR3F( 0.0f, 16.0f, 0.0f ),
	        QM_MATH_VECTOR3F( 32.0f, 16.0f, 0.0f ),
	        QM_MATH_VECTOR3F( 32.0f, 16.0f, 32.0f ),
	        QM_MATH_VECTOR3F( 0.0f, 16.0f, 32.0f ),
	};
	static constexpr unsigned int numVertices = QM_OS_ARRAY_ELEMENTS( vertices );

	QmMathColour4ub colour;
	if ( aux_collision_cylinder_intersect_polygon( &cylinder, &QM_MATH_VECTOR3F( 0.0f, 1.0f, 0.0f ), vertices, numVertices ) )
	{
		colour = QM_MATH_COLOUR4UB( 0, 255, 0, 255 );
	}
	else
	{
		colour = QM_MATH_COLOUR4UB( 255, 0, 0, 255 );
	}

	ape_draw_debug_polygon( vertices, numVertices, colour );
	ape_draw_debug_cylinder( &cylinder, &colour, 16 );
}

bool game_test_fire_decal_( ApeRoom *room, const QmMathVector3f *pos, const QmMathVector3f *dir )
{
	static ApeMaterial *material = nullptr;
	if ( material == nullptr )
	{
		material = ape_material_cache( "materials/decals/decal_sheet_default.mat.n", APE_CACHE_GROUP_WORLD, false );
		if ( material == nullptr )
		{
			return false;
		}
	}

	unsigned int seed  = qm_os_random_seed_initialize();
	float        angle = qm_os_random_uniform_float( &seed, 360.0f );
	float        scale = 1.0f + qm_os_random_float( &seed, 2.0f );

	return ape_room_create_projected_decal( room, material, pos, dir, angle, scale );
}
