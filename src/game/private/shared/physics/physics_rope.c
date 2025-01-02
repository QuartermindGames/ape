// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Wiggly jiggly rope physics!
// Author:  Mark E. Sowden

#include "physics.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static constexpr uint DEFAULT_NUM_PARTICLES = 8;

/////////////////////////////////////////////////////////////////////////////////////
// Public

float game_physics_rope_get_average_segment_length( const GamePhysicsRope *self )
{
	return self->length / ( float ) ( self->numParticles - 1 );
}

float game_physics_rope_get_length( const GamePhysicsRope *self )
{
	float l = 0.0f;
	for ( unsigned int i = 0; i < ( self->numParticles - 1 ); ++i )
	{
		l += PlVector3Length( PlSubtractVector3( self->particles[ i + 1 ].position, self->particles[ i ].position ) );
	}

	return l;
}

void game_physics_rope_attach( GamePhysicsRope *self, const PLVector3 *position, bool start )
{
	unsigned int slot                = start ? 0 : ( self->numParticles - 1 );
	self->particles[ slot ].fixed    = true;
	self->particles[ slot ].position = *position;
}

void game_physics_rope_dettach( GamePhysicsRope *self, bool start )
{
	unsigned int slot             = start ? 0 : ( self->numParticles - 1 );
	self->particles[ slot ].fixed = false;
}

void game_physics_rope_set_num_particles( GamePhysicsRope *self, uint num )
{
	if ( num == self->numParticles )
	{
		return;
	}
	else if ( num < 2 )
	{
		game_warning_( "Invalid number of particles for rope (%u); must be greater than 2!\n", num );
		num = 2;
	}
	else if ( num >= GAME_PHYSICS_ROPE_MAX_PARTICLES )
	{
		game_warning_( "Invalid number of particles for rope (%u); must be less than %u!\n", num, GAME_PHYSICS_ROPE_MAX_PARTICLES );
		num = ( GAME_PHYSICS_ROPE_MAX_PARTICLES - 1 );
	}

	if ( self->numParticles > 0 )
	{
		// store and dettach the current end, given we'll have a new end
		GamePhysicsRopeParticle end = self->particles[ self->numParticles - 1 ];
		game_physics_rope_dettach( self, false );

		self->numParticles = num;

		// and restore it
		self->particles[ self->numParticles - 1 ] = end;
	}
	else
	{
		self->numParticles = num;
	}
}

void game_physics_rope_tick( GamePhysicsRope *self, float delta )
{
	// add forces
	for ( uint i = 0; i < self->numParticles; ++i )
	{
		if ( self->particles[ i ].fixed )
		{
			continue;
		}

		self->particles[ i ].velocity = PL_VECTOR3( 0.0f, -0.005f, 0.0f );

		PLVector3 temp                   = self->particles[ i ].position;
		self->particles[ i ].position    = PlAddVector3( self->particles[ i ].position,
		                                                 PlAddVector3(
                                                              PlSubtractVector3( self->particles[ i ].position, self->particles[ i ].oldPosition ),
                                                              PlDivideVector3F( self->particles[ i ].velocity, delta ) ) );
		self->particles[ i ].oldPosition = temp;
	}

	// satisfy constraints
	uint numIterations = 2;
	for ( uint i = 0; i < numIterations; ++i )
	{
		for ( uint j = 0; j < self->numParticles - 1; ++j )
		{
			GamePhysicsRopeParticle *a = &self->particles[ j ];
			GamePhysicsRopeParticle *b = &self->particles[ j + 1 ];

			PLVector3 deltaVec = PlSubtractVector3( a->position, b->position );
			float     deltaLen = PlVector3Length( deltaVec );
			float     diff     = ( deltaLen > 0 ) ? ( self->length - deltaLen ) / deltaLen : 0.0f;

			PLVector3 adjust = PlScaleVector3F( deltaVec, 0.5f * diff );
			if ( !a->fixed )
			{
				a->position = PlAddVector3( a->position, adjust );
			}
			if ( !b->fixed )
			{
				b->position = PlSubtractVector3( b->position, adjust );
			}
		}
	}
}

void game_physics_rope_setup( GamePhysicsRope *self, uint numParticles, float length )
{
	game_physics_rope_set_num_particles( self, numParticles );

	self->length = length;
}

void game_physics_rope_debug_draw( GamePhysicsRope *self )
{
	for ( uint i = 0; i < self->numParticles; ++i )
	{
		const PLColour colour = ( self->particles[ i ].fixed ) ? PL_COLOUR_RED : PL_COLOUR_MAGENTA;
		ape_draw_debug_sphere( self->particles[ i ].position, colour, 0.1f );

		if ( i == 0 )
		{
			continue;
		}

		ape_draw_debug_arrow( self->particles[ i - 1 ].position, self->particles[ i ].position, PL_COLOUR_MAGENTA, 1.0f );
	}
}

PLVector3 game_physics_rope_get_particle_position( const GamePhysicsRope *self, uint particle )
{
	if ( particle >= self->numParticles )
	{
		game_warning_( "Invalid particle segment specified for rope (%u >= %u)!\n", particle, self->numParticles );
		return PL_VECTOR3( NAN, NAN, NAN );
	}

	return self->particles[ particle ].position;
}

PLVector3 game_physics_rope_get_start_position( const GamePhysicsRope *self )
{
	return game_physics_rope_get_particle_position( self, 0 );
}

PLVector3 game_physics_rope_get_end_position( const GamePhysicsRope *self )
{
	return game_physics_rope_get_particle_position( self, self->numParticles - 1 );
}
