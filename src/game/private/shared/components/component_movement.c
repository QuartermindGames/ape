// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Handler for general entity movement.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "component_movement.h"

#include "component_collision.h"
#include "../physics/physics.h"

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

static void ground_check( GameMovementComponent *self, GameCollisionComponent *collision, ApeEntity *entity )
{
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( entity ) );
	if ( room == nullptr )
	{
		return;
	}

	PLVector3 pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );

	//TODO: use collision component state, rather than our own crap

	static constexpr float SPHERE_SIZE = 4.0f;

	// check if we're still grounded
	ApeCollisionIntersection result = {};
	if ( game_physics_get_ground( room, &pos, &result ) )
	{
		self->isGrounded = result.distance <= SPHERE_SIZE;
		if ( self->isGrounded )
		{
			self->velocity.y = 0.0f;
			if ( result.face != nullptr )
			{
				self->contactNormal = result.face->normal;
			}
		}
	}

	ape_draw_debug_sphere( result.intersection, self->isGrounded ? PL_COLOUR_GREEN : PL_COLOUR_RED, SPHERE_SIZE );
}

void game_component_movement_tick_( GameMovementComponent *self, ApeEntity *entity, double delta )
{
	GameCollisionComponent *collision = ape_entity_get_component( entity, "collision" );

	PLVector3 pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );

	//game_debug_( "dir: %s (%p)\n", PlPrintVector3( &self->direction, PL_VAR_F32 ), self );
	//game_debug_( "vel: %s\n", PlPrintVector3( &self->velocity, PL_VAR_F32 ) );

	// check if there's a direction we're trying to move
	self->direction = PlNormalizeVector3( self->direction );
	PLVector3 accl  = PlScaleVector3F( qm_math_vector3f( self->direction.x, 0.0f, self->direction.z ), 16.0f );
	self->velocity  = PlAddVector3( self->velocity, PlScaleVector3F( accl, delta ) );

	// apply gravity (TODO: this should be hooked up with a var!)
	if ( !self->isGrounded )
	{
		self->velocity.y += -16.0f * delta;
		self->direction.y = 0.0f;
	}
	else
	{
		self->velocity.y += self->direction.y * 1024.0f * delta;
	}

	PLVector3 disp = PlScaleVector3F( self->velocity, delta );
	pos            = PlAddVector3( pos, disp );

	if ( collision != nullptr )
	{
		static constexpr float SPHERE_SIZE = 4.0f;
		ape_draw_debug_sphere( pos, PL_COLOURU8( 255, 255, 0, 255 ), SPHERE_SIZE );

		ground_check( self, collision, entity );

		ApeCollisionCollider collider = {};
		collider.type                 = collision->type;
		collider.ptr                  = &collision->collider;

		ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( entity ) );
		if ( room != nullptr )
		{
			unsigned int              numHits;
			ApeCollisionIntersection *hits;
			if ( ( hits = ape_room_intersect( room, &collider, &numHits ) ) != nullptr )
			{
				for ( unsigned int i = 0; i < numHits; ++i )
				{
					if ( hits[ i ].face != nullptr )
					{
						PLCollisionPlane plane = {};
						plane.origin           = hits[ i ].face->bounds.absOrigin;
						plane.normal           = hits[ i ].face->normal;
						ape_draw_debug_plane( &plane, PL_COLOUR_RED, 32.0f );

						if ( hits[ i ].depth > 0.0f )
						{
							PLVector3 collisionDirection = PlNormalizeVector3( PlSubtractVector3( hits[ i ].origin, hits[ i ].intersection ) );
							pos                          = PlAddVector3( pos, PlScaleVector3F( collisionDirection, hits[ i ].depth ) );
						}
					}

					ape_draw_debug_axis( hits[ i ].intersection, pl_vecOrigin3, SPHERE_SIZE );
				}

				PL_DELETE( hits );
			}
		}
	}

	ape_world_node_set_position( APE_WORLD_NODE( entity ), &pos );

	self->direction = ( PLVector3 ) {};
}

ApeEntityComponentDefinition game_movementComponent_ = {
        .name                = "movement",
        .createFunction      = create_movement,
        .destroyFunction     = destroy_movement,
        .serializeFunction   = serialize_movement,
        .deserializeFunction = deserialize_movement,
};
