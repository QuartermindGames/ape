// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Handler for general entity movement.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_random.h"

#include "game_private.h"

#include "component_movement.h"
#include "component_camera.h"
#include "component_collision.h"

#include "physics/physics.h"

static constexpr float GAME_MOVEMENT_DEFAULT_ACCELERATION = 1.0f;
static constexpr float GAME_MOVEMENT_DEFAULT_FRICTION     = 16.0f;

static bool printCurrentSurface;

/////////////////////////////////////////////////////////////////////////////////////
// Footstep sound stuff
// This pretty much just exists for the tech demo and will go in the trash once
// there's a proper event system for models in place.

static constexpr unsigned int MAX_FOOTSTEP_SOUNDS_PER_TYPE = 5;
static ApeAudioSample       **footstepSounds;

static void game_component_movement_on_register()
{
	ape_console_var_register( "game.printCurrentSurface", "Print information on the current surface we're standing on.", "false", PL_VAR_BOOL, &printCurrentSurface, nullptr, 0 );

	uint8_t numSurfaces = game_physics_surface_get_num();
	if ( numSurfaces > 0 )
	{
		footstepSounds = APE_MEMORY_NEW_C( ApeAudioSample *, MAX_FOOTSTEP_SOUNDS_PER_TYPE * numSurfaces );
		for ( unsigned int i = 0; i < numSurfaces; ++i )
		{
			const char *key = game_physics_surface_get_key( i );
			for ( unsigned int j = 0; j < MAX_FOOTSTEP_SOUNDS_PER_TYPE; ++j )
			{
				char path[ strlen( key ) + 64 ];
				snprintf( path, sizeof( path ), "sounds/footsteps/footstep_%s_%02u.wav", key, j );
				footstepSounds[ i * MAX_FOOTSTEP_SOUNDS_PER_TYPE + j ] = ape_audio_sample_cache( path );
			}
		}
	}
}

static ApeBrushFace *ground_check( GameMovementComponent *self, GameCollisionComponent *collision, ApeEntity *entity, ApeCollisionIntersection *result );

static void game_component_movement_footstep( GameMovementComponent *self, GameCollisionComponent *collision, ApeEntity *entity, double delta )
{
	// again, this is all just for the tech demo - there are so many issues with this otherwise
	// like, the movement system should have almost nothing to do with the camera here and all
	// of this should be defined by the model instead

	GameCameraComponent *camera = ape_entity_get_component( entity, GAME_CAMERA_COMPONENT_NAME );
	if ( camera == nullptr )
	{
		return;
	}

	ApeCollisionIntersection result;
	ApeBrushFace            *face = ground_check( self, collision, entity, &result );
	if ( face == nullptr || !self->isGrounded )
	{
		return;
	}

	uint8_t surfaceType = ape_material_get_surface_type( face->material );

	//game_print_( "view bob %f\n", camera->viewBob );
	if ( !self->hasPlayedStep && camera->viewBob <= -0.5f )
	{
		unsigned int    seed   = qm_os_random_seed_initialize();
		uint8_t         r      = qm_os_random_int( &seed ) % MAX_FOOTSTEP_SOUNDS_PER_TYPE;
		ApeAudioSample *sample = footstepSounds[ surfaceType * MAX_FOOTSTEP_SOUNDS_PER_TYPE + r ];
		if ( sample != nullptr )
		{
			ape_audio_sample_emit( sample, &result.intersection,
			                       20.f + qm_os_random_float( &seed, 10.f ),
			                       0.5f + qm_os_random_float( &seed, 0.5f ) );
		}

		self->hasPlayedStep = true;
	}
	else if ( self->hasPlayedStep && camera->viewBob >= 0.5f )
	{
		self->hasPlayedStep = false;
	}

	if ( printCurrentSurface )
	{
		const char *key = game_physics_surface_get_key( surfaceType );
		if ( key != nullptr )
		{
			const char *materialPath = ape_material_get_path( face->material );
			game_print_( "You're standing on %s (%s)\n", key, materialPath );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////

static void *create_movement()
{
	GameMovementComponent *movement = QM_OS_MEMORY_NEW( GameMovementComponent );
	movement->acceleration          = GAME_MOVEMENT_DEFAULT_ACCELERATION;

	return movement;
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

static ApeBrushFace *ground_check( GameMovementComponent *self, GameCollisionComponent *collision, ApeEntity *entity, ApeCollisionIntersection *result )
{
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( entity ) );
	if ( room == nullptr )
	{
		return nullptr;
	}

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );

	//TODO: use collision component state, rather than our own crap

	static constexpr float SPHERE_SIZE = 4.0f;

	// check if we're still grounded
	*result = ( ApeCollisionIntersection ) {};
	if ( game_physics_get_ground( room, &pos, result ) )
	{
		self->isGrounded = result->distance <= SPHERE_SIZE;
		if ( self->isGrounded )
		{
			self->velocity.y = 0.0f;

			ApeBrushFace *face = result->face;
			if ( face != nullptr )
			{
				self->contactNormal = face->normal;
			}
		}
	}

	return result->face;
}

void game_component_movement_tick_( GameMovementComponent *self, GameCollisionComponent *collision, ApeEntity *entity, double delta )
{
	// fetch the entity angles, clear the pitch
	QmMathVector3f angles = ape_world_node_get_angles( APE_WORLD_NODE( entity ) );
	angles.x              = 0.0f;

	// now fetch the forward and left axes
	QmMathVector3f forward, left;
	PlAnglesAxes( angles, &left, nullptr, &forward );

	float maxVelocity  = self->shiftModifier ? self->maxRunSpeed : self->maxWalkSpeed;
	float acceleration = self->shiftModifier ? self->acceleration * self->maxRunSpeed : self->acceleration * self->maxWalkSpeed;

	self->forwardVelocity += acceleration * self->directions[ GAME_MOVEMENT_DIRECTION_FB ] * delta;
	self->strafeVelocity += acceleration * self->directions[ GAME_MOVEMENT_DIRECTION_LR ] * delta;

	// clamp the velocities
	self->forwardVelocity = QM_MATH_CLAMP( -maxVelocity, self->forwardVelocity, maxVelocity );
	self->strafeVelocity  = QM_MATH_CLAMP( -maxVelocity, self->strafeVelocity, maxVelocity );

#if 0
	game_debug_( "for: %f (%f)\n", self->forwardVelocity, acceleration );
	game_debug_( "str: %f\n", self->strafeVelocity );
	game_debug_( "vel: %f %f %f\n", AUX_VEC3_ARGS( self->velocity ) );
#endif

	QmMathVector3f moveVelocity = qm_math_vector3f_add( self->velocity,
	                                                    qm_math_vector3f_add(
	                                                            qm_math_vector3f_scale_float( forward, self->forwardVelocity ),
	                                                            qm_math_vector3f_scale_float( left, self->strafeVelocity ) ) );
	moveVelocity                = qm_math_vector3f_clamp( moveVelocity, -maxVelocity, maxVelocity );

	self->velocity = qm_math_vector3f_add( self->velocity, moveVelocity );

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );
	pos                = qm_math_vector3f_add( pos, self->velocity );

	if ( collision != nullptr )
	{
		game_component_movement_footstep( self, collision, entity, delta );
	}

	// now update the entity position
	ape_world_node_set_position( APE_WORLD_NODE( entity ), &pos );

	// apply some friction to the velocity here
	if ( qm_math_vector3f_length( self->velocity ) > 0.01f )
	{
		self->velocity = qm_math_vector3f_scale_float( self->velocity, GAME_MOVEMENT_DEFAULT_FRICTION * delta );

		// urgh what why am I doing this...
		self->forwardVelocity = self->forwardVelocity * GAME_MOVEMENT_DEFAULT_FRICTION * delta;
		self->strafeVelocity  = self->strafeVelocity * GAME_MOVEMENT_DEFAULT_FRICTION * delta;
	}
	else
	{
		// zero it out if it's below 0.0
		self->velocity = ( QmMathVector3f ) {};

		// urgh what why am I doing this...
		self->forwardVelocity = self->forwardVelocity * GAME_MOVEMENT_DEFAULT_FRICTION * delta;
		self->strafeVelocity  = self->strafeVelocity * GAME_MOVEMENT_DEFAULT_FRICTION * delta;
	}

	self->shiftModifier = false;

	// clear the desired input directions, as we've dealt with them now
	QM_OS_ZERO_( self->directions );

	///////////////////////////////////// old shit below!

#if 0
	// check if there's a direction we're trying to move
	self->direction     = qm_math_vector3f_normalize( self->direction );
	QmMathVector3f accl = qm_math_vector3f_scale_float( qm_math_vector3f( self->direction.x, 0.0f, self->direction.z ), self->maxWalkSpeed );
	self->velocity      = qm_math_vector3f_add( self->velocity, qm_math_vector3f_scale_float( accl, delta ) );

	QmMathVector3f disp = qm_math_vector3f_scale_float( self->velocity, delta );
	QmMathVector3f pos  = ape_world_node_get_position( APE_WORLD_NODE( entity ) );
	pos                 = qm_math_vector3f_add( pos, disp );

	if ( collision != nullptr )
	{
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

	// apply gravity (TODO: this should be hooked up with a var!)
	if ( !self->isGrounded )
	{
		QmMathVector3f gravity = {};
		ape_entity_get_gravity( entity, &gravity );

		self->velocity    = qm_math_vector3f_add( self->velocity, qm_math_vector3f_scale_float( gravity, delta ) );
		self->direction.y = 0.0f;
	}
	else
	{
		if ( self->capabilities & GAME_MOVEMENT_CAPABILITY_JUMP )
		{
			self->velocity.y += self->direction.y * 1024.0f * delta;
		}

		// apply friction
	}

	ape_world_node_set_position( APE_WORLD_NODE( entity ), &pos );
#endif
}

ApeEntityComponentDefinition game_movementComponent_ = {
        .name                = GAME_MOVEMENT_COMPONENT_NAME,
        .createFunction      = create_movement,
        .destroyFunction     = destroy_movement,
        .serializeFunction   = serialize_movement,
        .deserializeFunction = deserialize_movement,

        .onRegister = game_component_movement_on_register,
};
