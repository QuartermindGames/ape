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
	PLMModel *model;

	ASoundReference impactSound;
	ASoundReference thrustSound;

	float forwardVelocity;

	PSEmitter *particleEmitter;

	PLVector3 variance;

	unsigned int fireDelay;
} ASGActor;

#define SG_BOUNDS 1024 /* bounds before an object is removed */
static bool SGActor_Generic_InsideBounds( Actor *self )
{
	const PLVector3 pos = Act_GetPosition( self );
	return !( pos.x > SG_BOUNDS || pos.x < -SG_BOUNDS || pos.z > SG_BOUNDS || pos.z < -SG_BOUNDS );
}

static void SGActor_Generic_Wrap( Actor *self )
{
	PLVector3 pos = Act_GetPosition( self );

	if ( pos.x > SG_BOUNDS ) pos.x = -SG_BOUNDS;
	else if ( pos.x < -SG_BOUNDS )
		pos.x = SG_BOUNDS;

	if ( pos.y > SG_BOUNDS ) pos.y = -SG_BOUNDS;
	else if ( pos.y < -SG_BOUNDS )
		pos.y = SG_BOUNDS;

	if ( pos.z > SG_BOUNDS ) pos.z = -SG_BOUNDS;
	else if ( pos.z < -SG_BOUNDS )
		pos.z = SG_BOUNDS;

	Act_SetPosition( self, &pos );
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
	if ( ( Act_GetType( self ) == ACTOR_SG_ASTEROID && Act_GetType( other ) == ACTOR_SG_SHIP ) && ( rand() % 10 == 0 ) )
	{
		v = PlInverseVector3( Act_GetVelocity( other ) );
		Act_SetVelocity( self, &v );
		return;
	}

	//if ( Act_GetType( other ) != ACTOR_SG_SHIP )
	//	Act_DestroyActor( self );
}

static void SGActor_Generic_Draw( Actor *self, void *userData )
{
	Camera *camera = R_GetGlobalCamera();
	if ( camera == NULL )
		return;

	ASGActor *sgActor = userData;

	if ( sgActor->particleEmitter != NULL )
		PS_Draw( sgActor->particleEmitter, camera );

	if ( sgActor->model != NULL )
	{
		PlMatrixMode( PL_MODELVIEW_MATRIX );
		PlPushMatrix();

		PlLoadIdentityMatrix();
		PlTranslateMatrix( Act_GetPosition( self ) );

		for ( unsigned int i = 0; i < sgActor->model->numMeshes; ++i )
			RM_DrawMesh( RM_GetFallbackMaterial(), sgActor->model->meshes[ i ] );

		PlPopMatrix();
	} /* todo */
}

static void SGActor_Generic_Destroy( Actor *self, void *userData )
{
	ASGActor *sgActor = userData;
	A_ReleaseSound( &sgActor->impactSound );
}

static void SGActor_Generic_SetModel( Actor *self, const char *path )
{
	ASGActor *sgActor = Act_GetUserData( self );
	sgActor->model	  = PlmLoadModel( path );
	if ( sgActor->model == NULL )
	{
		PrintWarn( "Failed to load model, \"%s\", for actor!\n", path );
		return;
	}

	Act_SetBounds( self, sgActor->model->bounds.mins, sgActor->model->bounds.maxs );
}

/****************************************
 * point.sg.ship
 ****************************************/

#define SHIP_BOUNDS_MAXS PLVector3( 16.0f, 90.0f, 16.0f )
#define SHIP_BOUNDS_MINS PLVector3( -16.0f, 0.0f, -16.0f )

static void Ship_Spawn( Actor *self )
{
	ASGActor *ship = SGActor_Generic_Spawn( self );

	Act_SetBounds( self, SHIP_BOUNDS_MINS, SHIP_BOUNDS_MAXS );

	SGActor_Generic_SetModel( self, "models/player_ship.node" );

	self->movementType = ACTOR_MOVEMENT_PHYSICS;

	ship->particleEmitter							= PS_SpawnEmitter();
	ship->particleEmitter->emissionRate				= 0;
	ship->particleEmitter->emissionVar				= 0;
	ship->particleEmitter->speed					= 2;
	ship->particleEmitter->speedVar					= 5;
	ship->particleEmitter->particleLife				= 2;
	ship->particleEmitter->particleLifeVar			= 4;
	ship->particleEmitter->maxParticles				= 512;
	ship->particleEmitter->startColour				= PlColourF32( 1.0f, 1.0f, 0.0f, 1.0f );
	//ship->particleEmitter->startColourVar			= PlColourF32( 0.02f, 0.05f, 0.1f, 0.0f );
	ship->particleEmitter->endColour				= PlColourF32( 1.0f, 0.0f, 1.0f, 0.0f );
	ship->particleEmitter->forceVar					= PLVector3( 0.0f, 0.05f, 0.0f );
	ship->particleEmitter->transform.translation	= Act_GetPosition( self );
	ship->particleEmitter->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );

	Camera *camera		= R_GetGlobalCamera();
	camera->followMode	= CAMERA_MODE_TOPDOWN;
	camera->parentActor = self;
}

#define TURN_SPEED	 5.0f
#define MAX_SPEED	 4.0f
#define MAX_VELOCITY PLAYER_RUN_SPEED
#define MIN_VELOCITY 0.5f

static void Ship_Tick( Actor *self, void *userData )
{
	PLVector3 pos		  = Act_GetPosition( self );
	PLVector3 curVelocity = Act_GetVelocity( self );

	float nAngle = Act_GetAngle( self );
	if ( globalSystem.GetButtonState( INPUT_LEFT ) || globalSystem.GetKeyState( 'a' ) )
		nAngle += TURN_SPEED;
	else if ( globalSystem.GetButtonState( INPUT_RIGHT ) || globalSystem.GetKeyState( 'd' ) )
		nAngle -= TURN_SPEED;

	Act_SetAngle( self, nAngle );

	static const float incAmount = 0.05f;

	ASGActor *sgActor = userData;
	if ( globalSystem.GetButtonState( INPUT_UP ) || globalSystem.GetKeyState( 'w' ) )
		sgActor->forwardVelocity += incAmount;
	else if ( globalSystem.GetButtonState( INPUT_DOWN ) || globalSystem.GetKeyState( 's' ) )
		sgActor->forwardVelocity -= incAmount;
	else if ( sgActor->forwardVelocity != 0.0f )
	{
		sgActor->forwardVelocity = sgActor->forwardVelocity > 0 ? sgActor->forwardVelocity - incAmount : sgActor->forwardVelocity + incAmount;
		if ( sgActor->forwardVelocity < 0.1f && sgActor->forwardVelocity > -0.1f )
			sgActor->forwardVelocity = 0.0f;
	}

	/* clamp the velocity as necessary */
	float maxVelocity		 = MAX_SPEED;
	sgActor->forwardVelocity = PlClamp( -maxVelocity, sgActor->forwardVelocity, maxVelocity );

	curVelocity = PlAddVector3( curVelocity, PlScaleVector3F( Act_GetForward( self ), sgActor->forwardVelocity ) );

	Act_SetVelocity( self, &curVelocity );
	Act_SetPosition( self, &pos );

	SGActor_Generic_UpdateParticleEmitter( self, userData );
	SGActor_Generic_Wrap( self );

	/* update position, as we might've wrapped */
	pos = Act_GetPosition( self );

	if ( globalSystem.GetKeyState( KEY_LEFT_CTRL ) && ( sgActor->fireDelay < Engine_GetNumTicks() ) )
	{
		Actor *projectile = Act_SpawnActorById( "point.sg.projectile", NULL );
		Act_SetPosition( projectile, &pos );

		PLVector3 f = Act_GetForward( self );
		PLVector3 v = PlScaleVector3F( f, 32.0f );
		Act_SetVelocity( projectile, &v );

		sgActor->fireDelay = Engine_GetNumTicks() + 15;
	}
}

const ActorSetup sg_actorShip = {
		.id			 = "point.sg.ship",
		.Spawn		 = Ship_Spawn,
		.Tick		 = Ship_Tick,
		.Draw		 = SGActor_Generic_Draw,
		.Collide	 = SGActor_Generic_Collide,
		.Destroy	 = SGActor_Generic_Destroy,
		.Serialize	 = NULL,
		.Deserialize = NULL,
};

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

#if 0
	if ( !SGActor_Generic_InsideBounds( self ) )
	{
		Act_DestroyActor( self );
		return;
	}
#endif

	// Make the asteroid spin based on it's given velocity
	PLVector3 spinAngles = PlAddVector3( Act_GetAngles( self ), Act_GetVelocity( self ) );
	Act_SetAngles( self, &spinAngles );

	//asteroid->variance.x			 = 0.0f;
	asteroid->variance.y			 = 0.05f;
	//asteroid->variance.z			 = 0.0f;
	asteroid->particleEmitter->force = asteroid->variance;

	PLVector3 pos = Act_GetPosition( self );
	//pos.x = cosf( Engine_GetNumTicks() / 64.0f ) * 50.0f;
	pos.y		  = cosf( Engine_GetNumTicks() / 64.0f ) * 50.0f;
	pos.z		  = sinf( Engine_GetNumTicks() / 64.0f ) * 50.0f;
	Act_SetPosition( self, &pos );

	SGActor_Generic_UpdateParticleEmitter( self, userData );
	SGActor_Generic_Wrap( self );

	static bool playSoundCheck = false;
	if ( !playSoundCheck )
	{
		A_EmitSound( &asteroid->impactSound, &pos, &pl_vecOrigin3 );
		playSoundCheck = true;
	}
}

const ActorSetup sg_actorAsteroidSetup = {
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

	Act_SetBounds( self, PLVector3( -3.0f, -3.0f, -3.0f ), PLVector3( 3.0f, 3.0f, 3.0f ) );

	SGActor_Generic_SetModel( self, "models/asteroid_00.node" );

	projectile->impactSound = A_CacheSound( "sounds/sg/projectile_impact.wav" );
}

static void Projectile_Tick( Actor *self, void *userData )
{
	// todo: rotate projectile relative to velocity, so it's always facing the right way

	if ( !SGActor_Generic_InsideBounds( self ) )
	{
		Act_DestroyActor( self );
		return;
	}
}

const ActorSetup sg_actorProjectileSetup = {
		.id			 = "point.sg.projectile",
		.Spawn		 = Projectile_Spawn,
		.Tick		 = Projectile_Tick,
		.Draw		 = NULL,
		.Collide	 = SGActor_Generic_Collide,
		.Destroy	 = SGActor_Generic_Destroy,
		.Serialize	 = NULL,
		.Deserialize = NULL,
};
