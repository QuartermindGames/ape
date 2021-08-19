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
#include "client/renderer/renderer.h"
#include "client/renderer/particle.h"

typedef struct ASGActor
{
	PLMModel		 *model;
	ASoundReference impactSound;
	PSEmitter	  *particleEmitter;

	PLVector3 variance;
} ASGActor;

#define SG_BOUNDS 1024 /* bounds before an object is removed */
static bool SGActor_Generic_InsideBounds( Actor *self )
{
	const PLVector3 pos = Act_GetPosition( self );
	return !( pos.x > SG_BOUNDS || pos.x < -SG_BOUNDS || pos.z > SG_BOUNDS || pos.z < -SG_BOUNDS );
}

static ASGActor *SGActor_Generic_Spawn( Actor *self )
{
	ASGActor *sgActor = globalSystem.MAlloc( sizeof( ASGActor ), true );
	Act_SetUserData( self, sgActor );

	return sgActor;
}

static void SGActor_Generic_UpdateParticleEmitter( Actor *self, ASGActor *sgSelf )
{
	if ( sgSelf->particleEmitter == NULL )
		return;

	/* make sure the emitter follows us */
	sgSelf->particleEmitter->transform.translation = Act_GetPosition( self );

	PS_TickEmitter( sgSelf->particleEmitter );
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

static void SGActor_Generic_Draw( Actor *self, void *userData )
{
	Camera *camera = R_GetGlobalCamera();
	if ( camera == NULL )
		return;

	ASGActor *sgActor = userData;

	if ( sgActor->particleEmitter != NULL )
		PS_Draw( sgActor->particleEmitter, camera );

	if ( sgActor->model != NULL ) {} /* todo */
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

	//asteroid->model = PlmLoadModel( ( rand() % 2 == 0 ) ? "models/asteroid_00.node" : "models/asteroid_01.node" );
	//if ( asteroid->model != NULL )
//		Act_SetBounds( self, asteroid->model->bounds.mins, asteroid->model->bounds.maxs );

	asteroid->impactSound = A_CacheSound( "sounds/sg/exxplosion0.wav" );

	asteroid->particleEmitter							= PS_SpawnEmitter();
	asteroid->particleEmitter->emissionRate				= 0;
	asteroid->particleEmitter->emissionVar				= 0;
	asteroid->particleEmitter->speed					= 2;
	asteroid->particleEmitter->speedVar					= 5;
	asteroid->particleEmitter->particleLife				= 2;
	asteroid->particleEmitter->particleLifeVar			= 4;
	asteroid->particleEmitter->maxParticles				= 2048;
	asteroid->particleEmitter->startColour				= PlColourF32( 1.0f, 1.0f, 0.0f, 1.0f );
	//asteroid->particleEmitter->startColourVar			= PlColourF32( 0.02f, 0.05f, 0.1f, 0.0f );
	asteroid->particleEmitter->endColour				= PlColourF32( 1.0f, 0.0f, 1.0f, 0.0f );
	asteroid->particleEmitter->forceVar					= PLVector3( 0.0f, 0.05f, 0.0f );
	asteroid->particleEmitter->transform.translation	= Act_GetPosition( self );
	asteroid->particleEmitter->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );
}

static void Asteroid_Tick( Actor *self, void *userData )
{
	ASGActor *asteroid = userData;

	if ( !SGActor_Generic_InsideBounds( self ) )
	{
		Act_DestroyActor( self );
		return;
	}

	// Make the asteroid spin based on it's given velocity
	PLVector3 spinAngles = PlAddVector3( Act_GetAngles( self ), Act_GetVelocity( self ) );
	Act_SetAngles( self, &spinAngles );

	//asteroid->variance.x			 = 0.0f;
	asteroid->variance.y = 0.05f;
	//asteroid->variance.z			 = 0.0f;
	asteroid->particleEmitter->force = asteroid->variance;

	PLVector3 pos = Act_GetPosition( self );
	//pos.x = cosf( Engine_GetNumTicks() / 64.0f ) * 50.0f;
	pos.y = cosf( Engine_GetNumTicks() / 64.0f ) * 50.0f;
	pos.z = sinf( Engine_GetNumTicks() / 64.0f ) * 50.0f;
	Act_SetPosition( self, &pos );

	SGActor_Generic_UpdateParticleEmitter( self, userData );

	static bool playSoundCheck = false;
	if ( !playSoundCheck )
	{
		A_EmitSound( &asteroid->impactSound, &pos, &pl_vecOrigin3 );
		playSoundCheck = true;
	}
}

const ActorSetup actorAsteroidSetup = {
		.id			 = "point.sg.asteroid",
		.Spawn		 = Asteroid_Spawn,
		.Tick		 = Asteroid_Tick,
		.Draw		 = SGActor_Generic_Draw,
		.Collide	 = SGActor_Generic_Collide,
		.Destroy	 = SGActor_Generic_Destroy,
		.Serialize	 = NULL,
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
		.id			 = "point.sg.projectile",
		.Spawn		 = Projectile_Spawn,
		.Tick		 = Projectile_Tick,
		.Draw		 = NULL,
		.Collide	 = SGActor_Generic_Collide,
		.Destroy	 = SGActor_Generic_Destroy,
		.Serialize	 = NULL,
		.Deserialize = NULL,
};
