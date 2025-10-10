// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Wiggly jiggly ropes!
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "../physics/physics.h"

#define ROPE_CLASS_NAME "rope"

static constexpr float ROPE_DEFAULT_LENGTH        = 32.0f;
static constexpr int   ROPE_DEFAULT_NUM_PARTICLES = 8;

typedef struct RopeEntity
{
	ApeWorldNode *endConnection;

	ApeStringProperty  endConnectionName[ 64 ];
	ApeIntegerProperty numParticles;
	ApeFloatProperty   length;

	GamePhysicsRope physics;
} RopeEntity;
#define ROPE_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), ROPE_CLASS_NAME, RopeEntity )

static bool showRopeDebug;

static void cache_rope()
{
	PlRegisterConsoleVariable( "game_debug_rope", "Toggle the display of wireframe ropes.", "false", PL_VAR_BOOL, &showRopeDebug, nullptr, false );
}

static void *create_rope( ApeEntity *self, AcmBranch *properties )
{
	RopeEntity *rope   = QM_OS_MEMORY_NEW( RopeEntity );
	rope->numParticles = ROPE_DEFAULT_NUM_PARTICLES;
	rope->length       = ROPE_DEFAULT_LENGTH;

	return rope;
}

static void update_bounds( ApeEntity *self )
{
	RopeEntity *rope = ROPE_ENTITY( self );
	assert( rope != nullptr );

	QmMathVector3f startPos = game_physics_rope_get_start_position( &rope->physics );
	QmMathVector3f endPos   = game_physics_rope_get_end_position( &rope->physics );

	self->base.localBounds.mins = startPos;
	self->base.localBounds.maxs = startPos;
	if ( endPos.x > startPos.x )
	{
		self->base.localBounds.maxs.x = endPos.x;
	}
	else
	{
		self->base.localBounds.mins.x = endPos.x;
	}
	if ( endPos.y > startPos.y )
	{
		self->base.localBounds.maxs.y = endPos.y;
	}
	else
	{
		self->base.localBounds.mins.y = endPos.y;
	}
	if ( endPos.z > startPos.z )
	{
		self->base.localBounds.maxs.z = endPos.z;
	}
	else
	{
		self->base.localBounds.mins.z = endPos.z;
	}
}

static void spawn_rope( ApeEntity *self )
{
	RopeEntity *rope = ROPE_ENTITY( self );
	assert( rope != nullptr );

	QmMathVector3f position = ape_world_node_get_local_position( APE_WORLD_NODE( self ) );
	game_physics_rope_setup( &rope->physics, rope->numParticles, rope->length, &position );
	game_physics_rope_attach( &rope->physics, &position, true );

	if ( rope->endConnection != nullptr )
	{
		position = ape_world_node_get_local_position( rope->endConnection );
		game_physics_rope_attach( &rope->physics, &position, false );
	}

	// simulate it a bit so it can settle
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( self ) );
	for ( unsigned int i = 0; i < 512; ++i )
	{
		game_physics_rope_tick( &rope->physics, room, 1.0f );
	}

	update_bounds( self );
}

static void tick_rope( ApeEntity *self, double delta )
{
	delta = game_get_delta_mod_( delta );

	RopeEntity *rope = ROPE_ENTITY( self );
	assert( rope != nullptr );

	QmMathVector3f position = ape_world_node_get_local_position( APE_WORLD_NODE( self ) );
	game_physics_rope_attach( &rope->physics, &position, true );

	if ( rope->endConnection != nullptr )
	{
		position = ape_world_node_get_local_position( rope->endConnection );
		game_physics_rope_attach( &rope->physics, &position, false );
	}

	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( self ) );
	game_physics_rope_tick( &rope->physics, room, delta );
	if ( showRopeDebug )
	{
		game_physics_rope_debug_draw( &rope->physics );
	}

	update_bounds( self );
}

static void draw_rope( ApeEntity *self, ApeLight *light, int flags )
{
	RopeEntity *rope = ROPE_ENTITY( self );
	assert( rope != nullptr );
}

static ApeEditorProperty properties[] = {
        APE_EDITOR_PROPERTY_STRING( "Next Connection", "Next entity that the rope connects to.", RopeEntity, endConnectionName ),

        APE_EDITOR_PROPERTY_BASIC( "Length", "Length of the rope.", RopeEntity, length, FLOAT ),
        APE_EDITOR_PROPERTY_BASIC( "Particles", "Number of particles making up the rope.", RopeEntity, numParticles, INTEGER ),
};

ApeEntityClassDefinition game_ropeEntityClass_ = {
        .name           = ROPE_CLASS_NAME,
        .description    = "Physics-driven rope handler."
                          "Rope can have a start attachment and end attachment.",
        .cacheFunction  = cache_rope,
        .createFunction = create_rope,
        .spawnFunction  = spawn_rope,
        .tickFunction   = tick_rope,
        .drawFunction   = draw_rope,

        .properties    = properties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( properties ),
};
