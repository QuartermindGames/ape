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

static void move_and_collide( GameMovementComponent *self, ApeEntity *entity, double delta )
{
}

static void ground_check( GameMovementComponent *self, ApeEntity *entity )
{
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( entity ) );
	if ( room == nullptr )
	{
		return;
	}

	PLVector3 pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );

	static constexpr float SPHERE_SIZE = 4.0f;

	// check if we're still grounded
	PLCollisionRay ray = {};
	ray.origin         = pos;
	ray.direction      = PL_VECTOR3( 0.0f, -1.0f, 0.0f );

	ApeCollisionIntersection result;
	ape_room_ray_intersect( room, &ray, &result );
	self->isGrounded = result.distance <= SPHERE_SIZE;
}

void game_component_movement_tick_( GameMovementComponent *self, ApeEntity *entity, double delta )
{
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( entity ) );
	if ( room == nullptr )
	{
		return;
	}

	//TODO: take all the desired directions etc., and apply them w/ collision checks

	PLVector3 pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );

	game_debug_( "dir: %s\n", PlPrintVector3( &self->direction, PL_VAR_F32 ) );
	game_debug_( "vel: %s\n", PlPrintVector3( &self->velocity, PL_VAR_F32 ) );

	// check if there's a direction we're trying to move
	self->direction = PlNormalizeVector3( self->direction );
	self->velocity  = PlAddVector3( self->velocity, PlScaleVector3F( self->direction, 1.0f ) );

	// apply gravity (TODO: this should be hooked up with a var!)
	if ( !self->isGrounded )
	{
		self->velocity.y += -0.5f * delta;
	}

	ape_draw_debug_sphere( pos, PL_COLOURU8( 255, 255, 0, 255 ), 16.0f );

	pos = PlAddVector3( pos, PlAddVector3F( self->velocity, delta ) );

	static constexpr float SPHERE_SIZE = 4.0f;

	ground_check( self, entity );

	// check if we're still grounded
	PLCollisionRay ray = {};
	ray.origin         = pos;
	ray.direction      = PL_VECTOR3( 0.0f, -1.0f, 0.0f );

	ApeCollisionIntersection result;
	ape_room_ray_intersect( room, &ray, &result );
	self->isGrounded = result.distance <= SPHERE_SIZE;

	PLCollisionSphere sphere = {};
	sphere.origin            = pos;
	sphere.radius            = SPHERE_SIZE;

	ApeCollisionCollider collider = {};
	collider.type                 = APE_COLLISION_TYPE_SPHERE;
	collider.sphere               = &sphere;

	unsigned int              numHits;
	ApeCollisionIntersection *hits;
	if ( ( hits = ape_room_intersect( room, &collider, &numHits ) ) != nullptr )
	{
#if 0
		
		for ( unsigned int i = 0; i < numHits; ++i )
		{
			if ( hits[ i ].face != nullptr )
			{
				PLCollisionPlane plane = {};
				plane.origin           = hits[ i ].face->bounds.absOrigin;
				plane.normal           = hits[ i ].face->normal;
				ape_draw_debug_plane( &plane, PL_COLOUR_RED, 32.0f );

				float penetrationDepth = sphere.radius - hits[ i ].distance;
				if ( penetrationDepth > 0.0f )
				{
					PLVector3 collisionDirection = PlNormalizeVector3( PlSubtractVector3( sphere.origin, hits[ i ].intersection ) );
					pos                          = PlAddVector3( pos, PlScaleVector3F( collisionDirection, penetrationDepth ) );
				}
			}

			ape_draw_debug_axis( hits[ i ].intersection, pl_vecOrigin3, 2.0f );
		}

#else

		// now determine which was the closest hit;
		ApeCollisionIntersection *hit = &hits[ 0 ];
		for ( unsigned int i = 1; i < numHits; i++ )
		{
			if ( hits[ i ].face == nullptr )
			{
				continue;
			}

			if ( hits[ i ].distance < hit->distance )
			{
				hit = &hits[ i ];
			}
		}

		if ( hit->face != nullptr )
		{
			PLCollisionPlane plane = {};
			plane.origin           = hit->face->bounds.absOrigin;
			plane.normal           = hit->face->normal;
			ape_draw_debug_plane( &plane, PL_COLOUR_RED, 32.0f );
		}

		float penetrationDepth = sphere.radius - hit->distance;
		if ( penetrationDepth > 0.0f )
		{
			PLVector3 collisionDirection = PlNormalizeVector3( PlSubtractVector3( sphere.origin, hit->intersection ) );
			pos                          = PlAddVector3( pos, PlScaleVector3F( collisionDirection, penetrationDepth ) );
		}

#endif

		PL_DELETE( hits );
	}

	ape_draw_debug_sphere( result.intersection, self->isGrounded ? PL_COLOUR_GREEN : PL_COLOUR_RED, 8.0f );

	ape_world_node_set_position( APE_WORLD_NODE( entity ), &pos );
}

ApeEntityComponentDefinition game_movementComponent_ = {
        .name                = "movement",
        .createFunction      = create_movement,
        .destroyFunction     = destroy_movement,
        .serializeFunction   = serialize_movement,
        .deserializeFunction = deserialize_movement,
};
