// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../game_private.h"

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
	PLVector3 position;
	PLVector3 oldPosition;
	PLVector3 velocity;

	bool fixed;
} GamePhysicsRopeParticle;

typedef struct GamePhysicsRope
{
	GamePhysicsRopeParticle particles[ GAME_PHYSICS_ROPE_MAX_PARTICLES ];
	uint                    numParticles;

	float width;
	float length;
	float mass;
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
void game_physics_rope_attach( GamePhysicsRope *self, const PLVector3 *position, bool start );

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
void game_physics_rope_set_num_particles( GamePhysicsRope *self, uint num );

/**
 * Simulate the rope. Should be called once per tick.
 *
 * @param self 		Pointer to instance.
 * @param delta 	Time delta of the frame.
 */
void game_physics_rope_tick( GamePhysicsRope *self, float delta );

/**
 * Sets up the initial rope state. Should be called before simulation.
 *
 * @param self 			Pointer to instance.
 * @param numParticles 	Number of particles for the rope.
 * @param length 		Length of the rope / slack.
 */
void game_physics_rope_setup( GamePhysicsRope *self, uint numParticles, float length );

/**
 * Provides a visual of the rope using the debug drawing API.
 *
 * @param self	Pointer to instance.
 */
void game_physics_rope_debug_draw( GamePhysicsRope *self );

/**
 * Get the position of one of the specific particle of the rope.
 *
 * @param self 		Pointer to instance.
 * @param particle 	Index of the particle.
 * @return 			Position of the specified segment, or NaN on error.
 */
PLVector3 game_physics_rope_get_particle_position( const GamePhysicsRope *self, uint particle );

/**
 * Return the start position of the rope.
 *
 * @param self 	Pointer to instance.
 * @return 		Position.
 */
PLVector3 game_physics_rope_get_start_position( const GamePhysicsRope *self );

/**
 * Return the end position of the rope.
 *
 * @param self 	Pointer to instance.
 * @return 		Position.
 */
PLVector3 game_physics_rope_get_end_position( const GamePhysicsRope *self );

/////////////////////////////////////////////////////////////////////////////////////
