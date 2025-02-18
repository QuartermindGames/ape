// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Health indicator, used for "living" entities.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "component_health.h"

static void *create_health()
{
	return PL_NEW( GameHealthComponent );
}

static void destroy_health( void *data )
{
	GameHealthComponent *health = data;
	PL_DELETE( health );
}

static AcmBranch *serialize_health( void *ptr, AcmBranch *root )
{
	GameHealthComponent *self = ptr;
	acm_push_ui32( root, "health", self->health );
	acm_push_ui32( root, "maxHealth", self->maxHealth );
	acm_push_ui32( root, "status", self->status );
	return root;
}

static void *deserialize_health( void *ptr, AcmBranch *root )
{
	GameHealthComponent *self = ptr;
	self->health              = acm_get_uint( root, "health", 0 );
	self->maxHealth           = acm_get_uint( root, "maxHealth", 0 );
	self->status              = acm_get_uint( root, "status", GAME_HEALTH_ALIVE );
	return self;
}

ApeEntityComponentDefinition game_healthComponent_ = {
        .name            = "health",
        .createFunction  = create_health,
        .destroyFunction = destroy_health,

        .serializeFunction   = serialize_health,
        .deserializeFunction = deserialize_health,
};
