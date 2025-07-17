// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Test framework for game-specific items.
// Author:  Mark E. Sowden

#include "game_private.h"

void game_test_cylinder_point_collision_( const PLVector3 *pos )
{
	ComCollisionCylinder cylinder = {};
	cylinder.height               = 64.0f;
	cylinder.origin               = PL_VECTOR3( 16.0f, 16.0f, 32.0f );
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
	        PL_VECTOR3( 0.0f, 16.0f, 0.0f ),
	        PL_VECTOR3( 32.0f, 16.0f, 0.0f ),
	        PL_VECTOR3( 32.0f, 16.0f, 32.0f ),
	        PL_VECTOR3( 0.0f, 16.0f, 32.0f ),
	};
	static constexpr unsigned int numVertices = PL_ARRAY_ELEMENTS( vertices );

	PLColour colour;
	if ( com_collision_cylinder_intersect_polygon( &cylinder, vertices, numVertices, &PL_VECTOR3( 0.0f, 1.0f, 0.0f ) ) )
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
