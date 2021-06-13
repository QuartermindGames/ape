/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 *
 * Purpose: https://itch.io/jam/quad-cities-indie-arcade-jam
 */

#include <plmodel/plm.h>

#include "yin.h"
#include "actor.h"

#include "client/audio/audio.h"

typedef struct ASGActor
{
	PLMModel *model;

	ASoundReference impactSound;
} ASGActor;

static ASGActor *SGActor_Generic_Spawn( Actor *self )
{
	ASGActor *sgActor = globalSystem.MAlloc( sizeof( ASGActor ), true );
	Act_SetUserData( self, sgActor );

	return sgActor;
}

static void SGActor_Generic_Collide( Actor *self, Actor *other, void *userData )
{
	/* todo
	 *  - emit explosion effect
	 */
	ASGActor *sgActor = userData;

	PLVector3 p = Act_GetPosition( self );
	PLVector3 v = Act_GetVelocity( self );
	A_EmitSound( &sgActor->impactSound, &p, &v );

	// If we hit a player, there's a slim chance we'll just bounce off
	if ( ( Act_GetType( self ) == ACTOR_SG_ASTEROID && Act_GetType( other ) == ACTOR_PLAYER ) && ( rand() % 10 == 0 ) )
	{
		v = PlInverseVector3( Act_GetVelocity( other ) );
		Act_SetVelocity( self, &v );
		return;
	}

	Act_DestroyActor( self );
}

static void SGActor_Generic_Destroy( Actor *self, void *userData )
{
	ASGActor *sgActor = userData;
	A_ReleaseSound( &sgActor->impactSound );
}

/****************************************
 * point.sg.asteroid
 ****************************************/

static void Asteroid_Spawn( Actor *self )
{
	ASGActor *asteroid = SGActor_Generic_Spawn( self );

	asteroid->model = PlmLoadModel( ( rand() % 2 == 0 ) ? "models/asteroid_00.ply" : "models/asteroid_01.ply" );

	Act_SetBounds( self, asteroid->model->bounds.mins, asteroid->model->bounds.maxs );

	asteroid->impactSound = A_CacheSound( "sounds/sg/explosion.wav" );
}

static void Asteroid_Tick( Actor *self, void *userData )
{
	// Make the asteroid spin based on it's given velocity
	PLVector3 spinAngles = PlAddVector3( Act_GetAngles( self ), Act_GetVelocity( self ) );
	Act_SetAngles( self, &spinAngles );
}

const ActorSetup actorAsteroidSetup = {
        .id          = "point.sg.asteroid",
        .Spawn       = Asteroid_Spawn,
        .Tick        = Asteroid_Tick,
        .Draw        = NULL,
        .Collide     = SGActor_Generic_Collide,
        .Destroy     = SGActor_Generic_Destroy,
        .Serialize   = NULL,
        .Deserialize = NULL,
};

/****************************************
 * point.sg.projectile
 ****************************************/

static void Projectile_Spawn( Actor *self )
{
	ASGActor *projectile = SGActor_Generic_Spawn( self );

	projectile->model = PlmLoadModel( "models/projectile.ply" );

	Act_SetBounds( self, PLVector3( 3.0f, 3.0f, 3.0f ), PLVector3( -3.0f, -3.0f, -3.0f ) );

	projectile->impactSound = A_CacheSound( "sounds/sg/projectile_impact.wav" );
}

static void Projectile_Tick( Actor *self, void *userData )
{
	// todo: rotate projectile relative to velocity, so it's always facing the right way
}

const ActorSetup actorProjectileSetup = {
        .id          = "point.sg.projectile",
        .Spawn       = Projectile_Spawn,
        .Tick        = Projectile_Tick,
        .Draw        = NULL,
        .Collide     = SGActor_Generic_Collide,
        .Destroy     = SGActor_Generic_Destroy,
        .Serialize   = NULL,
        .Deserialize = NULL,
};
