// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Code for extending entity functionality beyond what core provides.
// Author:  Mark E. Sowden

#include "game_private.h"
#include "game_entity.h"

#include "physics/physics.h"

void game_entity_place_on_ground( ApeEntity *self )
{
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( self ) );
	if ( room == nullptr )
	{
		game_warning_( "Failed to place entity on ground, no room!\n" );
		return;
	}

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( self ) );

	ApeCollisionIntersection result = {};
	if ( !game_physics_get_ground( room, &pos, &result ) )
	{
		char tmp[ 64 ];
		qm_math_vector3f_print( pos, tmp, sizeof( tmp ) );
		game_warning_( "Failed to place entity on ground at %s!\n", tmp );
		return;
	}

	//TODO: account for collisions here...?
	ape_world_node_set_position( APE_WORLD_NODE( self ), &result.intersection );
}
