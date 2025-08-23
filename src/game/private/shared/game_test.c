// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Test framework for game-specific items.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_random.h"

#include "game_private.h"

void game_test_cylinder_point_collision_( const PLVector3 *pos )
{
	ComCollisionCylinder cylinder = {};
	cylinder.height               = 64.0f;
	cylinder.origin               = qm_math_vector3f( 16.0f, 16.0f, 32.0f );
	cylinder.radius               = 16.0f;

	PLColour colour;
	if ( com_collision_cylinder_intersect_point( &cylinder, pos ) )
	{
		colour = PL_COLOURU8( 0, 255, 0, 255 );
	}
	else
	{
		colour = PL_COLOURU8( 255, 0, 0, 255 );
	}

	ape_draw_debug_cylinder( &cylinder, &colour, 16 );
}

void game_test_cylinder_polygon_collision_( const PLVector3 *pos )
{
	PLVector3 point = *pos;
	point.y -= 32.0f;

	ComCollisionCylinder cylinder = {};
	cylinder.height               = 64.0f;
	cylinder.origin               = point;
	cylinder.radius               = 16.0f;

	static constexpr PLVector3 vertices[] = {
	        QM_MATH_VECTOR3F( 0.0f, 16.0f, 0.0f ),
	        QM_MATH_VECTOR3F( 32.0f, 16.0f, 0.0f ),
	        QM_MATH_VECTOR3F( 32.0f, 16.0f, 32.0f ),
	        QM_MATH_VECTOR3F( 0.0f, 16.0f, 32.0f ),
	};
	static constexpr unsigned int numVertices = PL_ARRAY_ELEMENTS( vertices );

	PLColour colour;
	if ( com_collision_cylinder_intersect_polygon( &cylinder, vertices, numVertices, &QM_MATH_VECTOR3F( 0.0f, 1.0f, 0.0f ) ) )
	{
		colour = PL_COLOURU8( 0, 255, 0, 255 );
	}
	else
	{
		colour = PL_COLOURU8( 255, 0, 0, 255 );
	}

	ape_draw_debug_polygon( vertices, numVertices, colour );
	ape_draw_debug_cylinder( &cylinder, &colour, 16 );
}

bool game_test_fire_decal_( ApeRoom *room, const PLVector3 *pos, const PLVector3 *dir )
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
