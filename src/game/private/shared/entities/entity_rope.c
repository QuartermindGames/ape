// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Wiggly jiggly ropes!
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "../physics/physics.h"

typedef struct RopeEntity
{
	ApeWorldNode *startConnection;
	ApeWorldNode *endConnection;

	GamePhysicsRope physics;
} RopeEntity;
#define ROPE_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), RopeEntity )

static void *create_rope( ApeEntity *self, AcmBranch *properties )
{
	return PL_NEW( RopeEntity );
}

static void update_bounds( ApeEntity *self )
{
	RopeEntity *rope = ROPE_ENTITY( self );
	assert( rope != nullptr );

	PLVector3 startPos = game_physics_rope_get_start_position( &rope->physics );
	PLVector3 endPos   = game_physics_rope_get_end_position( &rope->physics );

	self->base.bounds.mins = startPos;
	self->base.bounds.maxs = startPos;
	if ( endPos.x > startPos.x )
	{
		self->base.bounds.maxs.x = endPos.x;
	}
	else
	{
		self->base.bounds.mins.x = endPos.x;
	}
	if ( endPos.y > startPos.y )
	{
		self->base.bounds.maxs.y = endPos.y;
	}
	else
	{
		self->base.bounds.mins.y = endPos.y;
	}
	if ( endPos.z > startPos.z )
	{
		self->base.bounds.maxs.z = endPos.z;
	}
	else
	{
		self->base.bounds.mins.z = endPos.z;
	}
}

static void spawn_rope( ApeEntity *self )
{
	RopeEntity *rope = ROPE_ENTITY( self );
	assert( rope != nullptr );

	game_physics_rope_setup( &rope->physics, 16, 1.0f );

	rope->startConnection = APE_WORLD_NODE( self );
	if ( rope->startConnection != nullptr )
	{
		PLVector3 position = ape_world_node_get_position( rope->startConnection );
		game_physics_rope_attach( &rope->physics, &position, true );
	}
	if ( rope->endConnection != nullptr )
	{
		PLVector3 position = ape_world_node_get_position( rope->endConnection );
		game_physics_rope_attach( &rope->physics, &position, false );
	}

	// simulate it a bit so it can settle
	for ( unsigned int i = 0; i < 512; ++i )
	{
		game_physics_rope_tick( &rope->physics, 1.0f );
	}

	update_bounds( self );
}

static void tick_rope( ApeEntity *self )
{
	RopeEntity *rope = ROPE_ENTITY( self );
	assert( rope != nullptr );

	if ( rope->startConnection != nullptr )
	{
		PLVector3 position = ape_world_node_get_position( rope->startConnection );
		game_physics_rope_attach( &rope->physics, &position, true );
	}
	if ( rope->endConnection != nullptr )
	{
		PLVector3 position = ape_world_node_get_position( rope->endConnection );
		game_physics_rope_attach( &rope->physics, &position, false );
	}

	game_physics_rope_tick( &rope->physics, 1.0f );

	update_bounds( self );
}

static void draw_rope( ApeEntity *self, ApeLight *light, int flags )
{
	RopeEntity *rope = ROPE_ENTITY( self );
	assert( rope != nullptr );

	game_physics_rope_debug_draw( &rope->physics );
}

ApeEntityClassDefinition game_ropeEntityClass = {
        .name           = "rope",
        .description    = "Physics-driven rope handler."
                          "Rope can have a start attachment and end attachment.",
        .createFunction = create_rope,
        .spawnFunction  = spawn_rope,
        .tickFunction   = tick_rope,
        .drawFunction   = draw_rope,
};
