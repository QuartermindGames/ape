// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "../game_private.h"

#include "component_collision.h"

static void *create_collision() { return PL_NEW( GameCollisionComponent ); }
static void  destroy_collision( void *data )
{
	GameCollisionComponent *collision = data;
	PL_DELETE( collision );
}

static AcmBranch *serialize_collision( void *ptr, AcmBranch *root )
{
	GameCollisionComponent *self = ptr;
	acm_push_ui32( root, "groups", self->groups );
	acm_push_ui32( root, "type", self->type );
	return root;
}

static void *deserialize_collision( void *ptr, AcmBranch *root )
{
	GameCollisionComponent *self = ptr;
	self->groups                 = acm_get_uint( root, "groups", 0 );
	self->type                   = acm_get_uint( root, "type", 0 );
	return self;
}

ApeEntityComponentDefinition game_collisionComponent_ = {
        .name            = "collision",
        .createFunction  = create_collision,
        .destroyFunction = destroy_collision,

        .serializeFunction   = serialize_collision,
        .deserializeFunction = deserialize_collision,
};

/////////////////////////////////////////////////////////////////////////////////////
// Public API
