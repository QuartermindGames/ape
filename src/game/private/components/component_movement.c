// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Handler for general entity movement.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "component_movement.h"

#include "component_collision.h"
#include "../physics/physics.h"

static void *create_movement()
{
	return QM_OS_MEMORY_NEW( GameMovementComponent );
}

static void destroy_movement( void *data )
{
	GameMovementComponent *movement = data;
	qm_os_memory_free( movement );
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
	movement->velocity              = com_acm_get_vector3( root, "velocity", &QM_MATH_VECTOR3F_ZERO );
	return movement;
}

static void ground_check( GameMovementComponent *self, GameCollisionComponent *collision, ApeEntity *entity )
{
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( entity ) );
	if ( room == nullptr )
	{
		return;
	}

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );

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

void game_component_movement_tick_( GameMovementComponent *self, GameCollisionComponent *collision, ApeEntity *entity, double delta )
{
	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );

	//game_debug_( "dir: %s (%p)\n", PlPrintVector3( &self->direction, PL_VAR_F32 ), self );
	//game_debug_( "vel: %s\n", PlPrintVector3( &self->velocity, PL_VAR_F32 ) );

	// check if there's a direction we're trying to move
	self->direction     = qm_math_vector3f_normalize( self->direction );
	QmMathVector3f accl = qm_math_vector3f_scale_float( qm_math_vector3f( self->direction.x, 0.0f, self->direction.z ), 16.0f );
	self->velocity      = qm_math_vector3f_add( self->velocity, qm_math_vector3f_scale_float( accl, delta ) );

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

	QmMathVector3f disp = qm_math_vector3f_scale_float( self->velocity, delta );
	pos                 = qm_math_vector3f_add( pos, disp );

	if ( collision != nullptr )
	{
		static constexpr float SPHERE_SIZE = 4.0f;
		//ape_draw_debug_sphere( pos, QM_MATH_COLOUR4UB( 255, 255, 0, 255 ), SPHERE_SIZE );

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
							QmMathVector3f collisionDirection = qm_math_vector3f_normalize( qm_math_vector3f_sub( hits[ i ].origin, hits[ i ].intersection ) );
							pos                               = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( collisionDirection, hits[ i ].depth ) );
						}
					}

					//ape_draw_debug_axis( hits[ i ].intersection, pl_vecOrigin3, SPHERE_SIZE );
				}

				qm_os_memory_free( hits );
			}
		}
	}

	ape_world_node_set_position( APE_WORLD_NODE( entity ), &pos );

	self->direction = ( QmMathVector3f ) {};
}

ApeEntityComponentDefinition game_movementComponent_ = {
        .name                = "movement",
        .createFunction      = create_movement,
        .destroyFunction     = destroy_movement,
        .serializeFunction   = serialize_movement,
        .deserializeFunction = deserialize_movement,
};
