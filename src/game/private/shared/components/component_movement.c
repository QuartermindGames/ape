// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Handler for general entity movement.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "component_movement.h"

static void *create_movement()
{
	return PL_NEW( GameMovementComponent );
}

static void destroy_movement( void *data )
{
	GameMovementComponent *movement = data;
	PL_DELETE( movement );
}

static AcmBranch *serialize_movement( void *ptr, AcmBranch *root )
{
	GameMovementComponent *movement = ptr;
	com_acm_push_vector3( root, "velocity", &movement->velocity, false );
	return root;
}

static void *deserialize_movement( void *ptr, AcmBranch *root )
{
	GameMovementComponent *movement = ptr;
	movement->velocity              = com_acm_get_vector3( root, "velocity", &pl_vecOrigin3 );
	return movement;
}

void game_component_movement_handle_( GameMovementComponent *self, ApeEntity *entity, const PLVector3 *dir )
{
}

ApeEntityComponentDefinition game_movementComponent_ = {
        .name                = "movement",
        .createFunction      = create_movement,
        .destroyFunction     = destroy_movement,
        .serializeFunction   = serialize_movement,
        .deserializeFunction = deserialize_movement,
};
