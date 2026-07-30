// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "../game_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Surfaces
// Basically, the things that you walk on and bump into.
// Materials define this via the surfaceType var.

/**
 * Fetches the surfaces config and loads everything in.
 */
void game_physics_surface_initialize();

/**
 * Shuts down the surface system and cleans up all surfaces.
 */
void game_physics_surface_shutdown();

/**
 * Returns the key specified by the specific surface entry.
 */
const char *game_physics_surface_get_key( uint8_t index );

/**
 * Returns the number of available surfaces.
 */
uint8_t game_physics_surface_get_num();

/////////////////////////////////////////////////////////////////////////////////////

static inline bool game_physics_get_ground( ApeRoom *room, const QmMathVector3f *position, ApeCollisionIntersection *result )
{
	PLCollisionRay ray = {};
	ray.origin         = *position;
	ray.direction      = qm_math_vector3f( 0.0f, -1.0f, 0.0f );

	ape_room_ray_intersect( room, &ray, result );

	return result->face != nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////
// Physics Rope

#define GAME_PHYSICS_ROPE_MAX_PARTICLES 256

/**
 * Particles represent the individual
 * points of the rope that are actually
 * simulated.
 */
typedef struct GamePhysicsRopeParticle
{
	QmMathVector3f position;
	QmMathVector3f oldPosition;
	QmMathVector3f velocity;

	bool fixed;
} GamePhysicsRopeParticle;

typedef struct GamePhysicsRope
{
	GamePhysicsRopeParticle particles[ GAME_PHYSICS_ROPE_MAX_PARTICLES ];
	unsigned int            numParticles;

	float length;
	float windCoefficient;

	bool isCollidable;
} GamePhysicsRope;

/**
 * Returns the average length of each segment of the rope.
 *
 * @param self	Pointer to instance.
 * @return 		Average length of each segment.
 */
float game_physics_rope_get_average_segment_length( const GamePhysicsRope *self );

/**
 * Calculates and returns the current length of the rope.
 *
 * @param self	Pointer to instance.
 * @return		Length of the rope.
 */
float game_physics_rope_get_length( const GamePhysicsRope *self );

/**
 * Attach the rope, either from the start or end, to a given point.
 *
 * @param self 		Pointer to instance.
 * @param position 	Position to fix to.
 * @param start 	Whether to attach the start or end.
 */
void game_physics_rope_attach( GamePhysicsRope *self, const QmMathVector3f *position, bool start );

/**
 * Dettach the rope, either from the start or end.
 *
 * @param self 	Pointer to instance.
 * @param start Whether to dettach the start or end.
 */
void game_physics_rope_dettach( GamePhysicsRope *self, bool start );

/**
 * Set the number of particles the rope will have.
 * If either end is attached, this will be retained with the new start or end.
 *
 * @param self 	Pointer to instance.
 * @param num 	Number of particles.
 */
void game_physics_rope_set_num_particles( GamePhysicsRope *self, unsigned int num );

/**
 * Simulate the rope. Should be called once per tick.
 *
 * @param self 		Pointer to instance.
 * @param room		Specify the room, for collisions. If left null, collisions are disabled.
 * @param delta 	Time delta of the frame.
 * @param windVelocity
 */
void game_physics_rope_tick( GamePhysicsRope *self, ApeRoom *room, double delta, const QmMathVector3f *windVelocity );

/**
 * Sets up the initial rope state. Should be called before simulation.
 *
 * @param self 			Pointer to instance.
 * @param numParticles 	Number of particles for the rope.
 * @param length 		Length of the rope / slack.
 * @param initPosition	Initial position of the rope. All particles will init at this position.
 */
void game_physics_rope_setup( GamePhysicsRope *self, unsigned int numParticles, float length, const QmMathVector3f *initPosition );

/**
 * Provides a visual of the rope using the debug drawing API.
 *
 * @param self	Pointer to instance.
 */
void game_physics_rope_debug_draw( const GamePhysicsRope *self );

/**
 * Get the position of one of the specific particle of the rope.
 *
 * @param self 		Pointer to instance.
 * @param particle 	Index of the particle.
 * @return 			Position of the specified segment, or NaN on error.
 */
QmMathVector3f game_physics_rope_get_particle_position( const GamePhysicsRope *self, unsigned int particle );

/**
 * Return the start position of the rope.
 *
 * @param self 	Pointer to instance.
 * @return 		Position.
 */
QmMathVector3f game_physics_rope_get_start_position( const GamePhysicsRope *self );

/**
 * Return the end position of the rope.
 *
 * @param self 	Pointer to instance.
 * @return 		Position.
 */
QmMathVector3f game_physics_rope_get_end_position( const GamePhysicsRope *self );

/////////////////////////////////////////////////////////////////////////////////////
