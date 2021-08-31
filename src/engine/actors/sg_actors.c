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

#include "model.h"

#define MAX_ASTEROID_MODELS 2
static PLMModel *asteroidModels[ MAX_ASTEROID_MODELS ] = { NULL, NULL };
static PLMModel *projectileModel = NULL;

ASound *impactSound = NULL;
ASound *thrustSound = NULL;
ASound *fireSound = NULL;
ASound *gameEndSound = NULL;
ASound *gameStartSound = NULL;

void SG_PrecacheData( void )
{
	asteroidModels[ 0 ] = PlmLoadModel( "models/asteroid_00.node" );
	asteroidModels[ 1 ] = PlmLoadModel( "models/asteroid_01.node" );

	projectileModel = PlmLoadModel( "models/projectile.node" );

	impactSound = A_CacheSound( "sounds/sg/explosion0.wav" );
	thrustSound = A_CacheSound( "sounds/testing/ping.wav" );
	fireSound = A_CacheSound( "sounds/sg/soundwah.wav" );
	gameEndSound = A_CacheSound( "sounds/sg/gameend.wav" );
	gameStartSound = A_CacheSound( "sounds/sg/gameon.wav" );
}

void SG_DestroyCachedData( void )
{
	for ( unsigned int i = 0; i < MAX_ASTEROID_MODELS; ++i )
	{
		PlmDestroyModel( asteroidModels[ i ] );
	}

	PlmDestroyModel( projectileModel );
}

typedef struct ASGActor
{
	PLMModel *model;

	float forwardVelocity;
	float scale;

	PSEmitter *particleEmitter;
	PSEmitter *emitLeft, *emitRight;

	PLVector3 variance;

	unsigned int fireDelay;

	bool isSolid;
} ASGActor;

typedef struct AsteroidManager
{
	ASGActor base;
	unsigned int numAsteroids;
} AsteroidManager;
static AsteroidManager *asteroidManager = NULL;

#define MAX_ASTEROIDS 200

#define SG_BOUNDS 2048 /* bounds before an object is removed */
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
	PLVector3 forward, left;
	PlAnglesAxes( PLVector3( 0, self->angles.y, 0 ), &left, NULL, &forward );

	if ( sgSelf->particleEmitter != NULL )
	{
		PLVector3 cpos = PlSubtractVector3( self->position, PlScaleVector3F( forward, 20.0f ) );
		sgSelf->particleEmitter->transform.translation = cpos;
		PS_TickEmitter( sgSelf->particleEmitter );
	}

	if ( sgSelf->emitLeft != NULL )
	{
		PLVector3 lpos = PlAddVector3( PlSubtractVector3( self->position, PlScaleVector3F( forward, 20.0f ) ), PlScaleVector3F( left, 32.0f ) );
		sgSelf->emitLeft->transform.translation = lpos;
		PS_TickEmitter( sgSelf->emitLeft );
	}

	if ( sgSelf->emitRight != NULL )
	{
		PLVector3 rpos = PlSubtractVector3( PlSubtractVector3( self->position, PlScaleVector3F( forward, 20.0f ) ), PlScaleVector3F( left, 32.0f ) );
		sgSelf->emitRight->transform.translation = rpos;
		PS_TickEmitter( sgSelf->emitRight );
	}
}

static void SGActor_Generic_Collide( Actor *self, Actor *other, void *userData )
{
	/* todo
	 *  - emit explosion effect
	 */
	ASGActor *sgActor = self->userData;

	if ( ( self->type == ACTOR_SG_PROJECTILE && other->type == ACTOR_SG_SHIP ) || !sgActor->isSolid )
		return;

	// If we hit a player, there's a slim chance we'll just bounce off
	//if ( /*( Act_GetType( self ) == ACTOR_SG_ASTEROID && Act_GetType( other ) == ACTOR_SG_SHIP ) &&*/ ( rand() % 10 == 0 ) )
	//other->velocity = PlInverseVector3( self->velocity );

	other->health -= 2;
	if ( other->type != ACTOR_SG_SHIP )
	{
		if ( other->health <= 0 )
		{
			//A_EmitSound( impactSound, 10 );
			Act_DestroyActor( other );
			return;
		}

		Monster_Collide( self, other, 0.0f );
	}
}

static void SGActor_Generic_Draw( Actor *self, void *userData )
{
	Camera *camera = R_GetGlobalCamera();
	if ( camera == NULL )
		return;

	ASGActor *sgActor = userData;
	if ( sgActor->model != NULL )
	{
		PlMatrixMode( PL_MODELVIEW_MATRIX );
		PlPushMatrix();

		PlLoadIdentityMatrix();

		PlScaleMatrix( PLVector3( 10.0f + sgActor->scale, 10.0f + sgActor->scale, 10.0f + sgActor->scale ) );

		float x = PlDegreesToRadians( self->angles.x - 90.0f );
		PlRotateMatrix( x, 1.0f, 0.0f, 0.0f );
		float y = PlDegreesToRadians( self->angles.y );
		PlRotateMatrix( y, 0.0f, 1.0f, 0.0f );
		float z = PlDegreesToRadians( self->angles.z );
		PlRotateMatrix( z, 0.0f, 0.0f, 1.0f );

		PlTranslateMatrix( Act_GetPosition( self ) );

		for ( unsigned int i = 0; i < sgActor->model->numMeshes; ++i )
		{
			MDLUserData *modelData = sgActor->model->userData;
			RM_DrawMesh( modelData->materials[ i ], sgActor->model->meshes[ i ] );
		}

		PlPopMatrix();
	}

	if ( sgActor->particleEmitter != NULL )
	{
		PS_Draw( sgActor->particleEmitter, camera );
	}
	if ( sgActor->emitRight != NULL )
	{
		PS_Draw( sgActor->emitRight, camera );
	}
	if ( sgActor->emitLeft != NULL )
	{
		PS_Draw( sgActor->emitLeft, camera );
	}
}

static void SGActor_Generic_Destroy( Actor *self, void *userData )
{
	ASGActor *sgActor = userData;

	PS_DestroyEmitter( sgActor->particleEmitter );
	PS_DestroyEmitter( sgActor->emitLeft );
	PS_DestroyEmitter( sgActor->emitRight );

	if ( asteroidManager != NULL && self->type == ACTOR_SG_ASTEROID )
		asteroidManager->numAsteroids--;

	globalSystem.Free( sgActor );
}

static void SGActor_Generic_SetModel( Actor *self, const char *path )
{
	ASGActor *sgActor = Act_GetUserData( self );
	sgActor->model = PlmLoadModel( path );
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
	ship->isSolid = true;

	Act_SetBounds( self, SHIP_BOUNDS_MINS, SHIP_BOUNDS_MAXS );

	SGActor_Generic_SetModel( self, "models/player_ship.node" );

	self->health = 100;
	self->movementType = ACTOR_MOVEMENT_PHYSICS;

	ship->particleEmitter = PS_SpawnEmitter();
	ship->particleEmitter->emissionRate = 0;
	ship->particleEmitter->emissionVar = 0;
	ship->particleEmitter->speed = 2;
	ship->particleEmitter->speedVar = 5;
	ship->particleEmitter->particleLife = 2;
	ship->particleEmitter->particleLifeVar = 1;
	ship->particleEmitter->maxParticles = 100;
	ship->particleEmitter->startColour = PlColourF32( 1.0f, 0.5f, 0.5f, 1.0f );
	//ship->particleEmitter->startColourVar			= PlColourF32( 0.02f, 0.05f, 0.1f, 0.0f );
	ship->particleEmitter->endColour = PlColourF32( 1.0f, 0.2f, 0.2f, 0.0f );
	ship->particleEmitter->forceVar = PLVector3( 0.0f, 0.05f, 0.0f );
	ship->particleEmitter->transform.translation = Act_GetPosition( self );
	ship->particleEmitter->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );
	ship->particleEmitter->material = RM_CacheMaterial( "materials/effects/particle.mat", CACHE_GROUP_WORLD, true );

	ship->emitLeft = PS_SpawnEmitter();
	ship->emitLeft->emissionRate = 4;
	ship->emitLeft->emissionVar = 0;
	ship->emitLeft->speed = 2;
	ship->emitLeft->speedVar = 5;
	ship->emitLeft->particleLife = 2;
	ship->emitLeft->particleLifeVar = 1;
	ship->emitLeft->maxParticles = 100;
	ship->emitLeft->startColour = PlColourF32( 1.0f, 1.0f, 1.0f, 1.0f );
	ship->emitLeft->endColour = PlColourF32( 0.2f, 0.2f, 0.2f, 0.0f );
	ship->emitLeft->forceVar = PLVector3( 0.0f, 0.05f, 0.0f );
	ship->emitLeft->transform.translation = Act_GetPosition( self );
	ship->emitLeft->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );
	ship->emitLeft->material = RM_CacheMaterial( "materials/effects/particle.mat", CACHE_GROUP_WORLD, true );

	ship->emitRight = PS_SpawnEmitter();
	ship->emitRight->emissionRate = 4;
	ship->emitRight->emissionVar = 0;
	ship->emitRight->speed = 2;
	ship->emitRight->speedVar = 5;
	ship->emitRight->particleLife = 2;
	ship->emitRight->particleLifeVar = 1;
	ship->emitRight->maxParticles = 100;
	ship->emitRight->startColour = PlColourF32( 1.0f, 1.0f, 1.0f, 1.0f );
	ship->emitRight->endColour = PlColourF32( 0.2f, 0.2f, 0.2f, 0.0f );
	ship->emitRight->forceVar = PLVector3( 0.0f, 0.05f, 0.0f );
	ship->emitRight->transform.translation = Act_GetPosition( self );
	ship->emitRight->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );
	ship->emitRight->material = RM_CacheMaterial( "materials/effects/particle.mat", CACHE_GROUP_WORLD, true );

	Camera *camera = R_GetGlobalCamera();
	camera->followMode = CAMERA_MODE_TOPDOWN;
	camera->parentActor = self;
}

#define TURN_SPEED	 5.0f
#define MAX_SPEED	 4.0f
#define MAX_VELOCITY PLAYER_RUN_SPEED
#define MIN_VELOCITY 0.5f

static void Ship_Tick( Actor *self, void *userData )
{
	if ( globalSystem.GetButtonState( INPUT_LEFT ) || globalSystem.GetKeyState( 'a' ) )
		self->angles.y += TURN_SPEED;
	else if ( globalSystem.GetButtonState( INPUT_RIGHT ) || globalSystem.GetKeyState( 'd' ) )
		self->angles.y -= TURN_SPEED;

	static const float incAmount = 0.0015f;

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
	sgActor->forwardVelocity = PlClamp( -MAX_SPEED, sgActor->forwardVelocity, MAX_SPEED );

	self->velocity = PlAddVector3( self->velocity, PlScaleVector3F( Act_GetForward( self ), sgActor->forwardVelocity ) );

	SGActor_Generic_UpdateParticleEmitter( self, userData );
	SGActor_Generic_Wrap( self );

	if ( globalSystem.GetKeyState( KEY_LEFT_CTRL ) && ( sgActor->fireDelay < Engine_GetNumTicks() ) )
	{
		Actor *projectile = Act_SpawnActorById( "point.sg.projectile", NULL );
		projectile->position = self->position;

		projectile->velocity = PlScaleVector3F( self->forward, 32.0f );
		projectile->angles = self->angles;
		projectile->angles.y += -90.0f;

		A_EmitSound( fireSound, 50 );

		sgActor->fireDelay = Engine_GetNumTicks() + 15;
	}

	static unsigned int scoreDelay = 0;
	if ( self->health > 0 && scoreDelay < Engine_GetNumTicks() )
	{
		self->score++;
		scoreDelay = Engine_GetNumTicks() + 35;
	}
}

static void Ship_Collide( Actor *self, Actor *other, void *userData )
{
	//SGActor_Generic_Collide( self, other, userData );

	ASGActor *sgActor = self->userData;
	if ( other->type == ACTOR_SG_PROJECTILE || !sgActor->isSolid )
		return;

	if ( sgActor->forwardVelocity > 0.0f )
	{
		sgActor->forwardVelocity /= 2.0f;
	}

	Monster_Collide( self, other, 100.0f );
}

const ActorSetup sg_actorShip = {
		.id = "point.sg.ship",
		.Spawn = Ship_Spawn,
		.Tick = Ship_Tick,
		.Draw = SGActor_Generic_Draw,
		.Collide = Ship_Collide,
		.Destroy = SGActor_Generic_Destroy,
		.Serialize = NULL,
		.Deserialize = NULL,
};

/****************************************
 * point.sg.asteroid
 ****************************************/

static void Asteroid_Spawn( Actor *self )
{
	ASGActor *asteroid = SGActor_Generic_Spawn( self );

	asteroid->isSolid = true;
	asteroid->model = ( rand() % MAX_ASTEROID_MODELS == 0 ) ? asteroidModels[ 0 ] : asteroidModels[ 1 ];
	asteroid->scale = PlGenerateRandomFloat( 30.0f );

	self->bounds.mins = PlSubtractVector3F( self->bounds.mins, asteroid->scale );
	self->bounds.maxs = PlAddVector3F( self->bounds.maxs, asteroid->scale );

	self->health = 25;

	if ( asteroidManager != NULL )
		asteroidManager->numAsteroids++;
}

static void Asteroid_Tick( Actor *self, void *userData )
{
	// Make the asteroid spin based on it's given velocity
	PLVector3 spinAngles = PlAddVector3( Act_GetAngles( self ), Act_GetVelocity( self ) );
	Act_SetAngles( self, &spinAngles );

	SGActor_Generic_UpdateParticleEmitter( self, userData );
	SGActor_Generic_Wrap( self );
}

const ActorSetup sg_actorAsteroidSetup = {
		.id = "point.sg.asteroid",
		.Spawn = Asteroid_Spawn,
		.Tick = Asteroid_Tick,
		.Draw = SGActor_Generic_Draw,
		.Collide = SGActor_Generic_Collide,
		.Destroy = SGActor_Generic_Destroy,
		.Serialize = NULL,
		.Deserialize = NULL,
};

/****************************************
 * point.sg.asteroidmanager
 ****************************************/

static void AManager_Spawn( Actor *self )
{
	assert( asteroidManager == NULL );

	asteroidManager = globalSystem.MAlloc( sizeof( AsteroidManager ), true );
	self->userData = asteroidManager;

	asteroidManager->base.isSolid = false;
}

static void AManager_Destroy( Actor *self, void *userData )
{
	assert( asteroidManager != NULL );

	globalSystem.Free( asteroidManager );
	asteroidManager = NULL;
}

static void AManager_Tick( Actor *self, void *userData )
{
	if ( asteroidManager->numAsteroids >= MAX_ASTEROIDS )
		return;

	Actor *asteroid = Act_SpawnActor( ACTOR_SG_ASTEROID, NULL );
	asteroid->position = PLVector3( -SG_BOUNDS + ( rand() % ( SG_BOUNDS * 2 ) ), 0.0f, -SG_BOUNDS + ( rand() % ( SG_BOUNDS * 2 ) ) );
	asteroid->bounds.origin = asteroid->position;

	if ( Act_IsVisible( asteroid ) || ( Act_CheckCollisions( asteroid ) != NULL ) )
	{
		Act_DestroyActor( asteroid );
		return;
	}

	asteroid->velocity = PLVector3(
			PlGenerateRandomFloat( 2.0f ) - PlGenerateRandomFloat( 2.0f ),
			0.0f,
			PlGenerateRandomFloat( 2.0f ) - PlGenerateRandomFloat( 2.0f ) );
}

const ActorSetup sg_actorAsteroidManagerSetup = {
		.id = "point.sg.asteroidmanager",
		.Spawn = AManager_Spawn,
		.Tick = AManager_Tick,
		.Draw = NULL,
		.Collide = NULL,
		.Destroy = AManager_Destroy,
		.Serialize = NULL,
		.Deserialize = NULL,
};

/****************************************
 * point.sg.prop
 ****************************************/

static void Prop_Spawn( Actor *self )
{
	SGActor_Generic_Spawn( self );

	self->angles.y = 180.0f;
}

static void Prop_Tick( Actor *self, void *userData )
{
	//self->position.y = ( ( 1.0f + sinf( Engine_GetNumTicks() / 100.0f ) ) / 10.0f ) * 24.0f;
	//self->position.z = ( ( 1.0f + cosf( Engine_GetNumTicks() / 100.0f ) ) / 10.0f ) * 16.0f;

	self->angles.x = ( ( ( 1.0f + cosf( Engine_GetNumTicks() / 100.0f ) ) / 10.0f ) * 16.0f );
	//self->angles.y -= ( ( 1.0f + sinf( Engine_GetNumTicks() / 100.0f ) ) / 10.0f ) * 16.0f;
	//self->angles.z -= ( ( 1.0f + cosf( Engine_GetNumTicks() / 100.0f ) ) / 10.0f ) * 16.0f;
}

static void Prop_Deserialize( Actor *self, NLNode *nodeTree )
{
	ASGActor *sg = self->userData;

	const char *modelPath = NL_GetStrByName( nodeTree, "modelPath", NULL );
	if ( modelPath != NULL )
		SGActor_Generic_SetModel( self, modelPath );
}

const ActorSetup sg_actorPropSetup = {
		.id = "point.sg.prop",
		.Spawn = Prop_Spawn,
		.Tick = Prop_Tick,
		.Draw = SGActor_Generic_Draw,
		.Collide = NULL,
		.Destroy = SGActor_Generic_Destroy,
		.Serialize = NULL,
		.Deserialize = Prop_Deserialize,
};

/****************************************
 * point.sg.projectile
 ****************************************/

static void Projectile_Spawn( Actor *self )
{
	ASGActor *projectile = SGActor_Generic_Spawn( self );
	projectile->isSolid = true;

	projectile->model = projectileModel;

	Act_SetBounds( self, PLVector3( -3.0f, -3.0f, -3.0f ), PLVector3( 3.0f, 3.0f, 3.0f ) );

	//	projectile->impactSound = A_CacheSound( "sounds/sg/projectile_impact.wav" );
}

static void Projectile_Tick( Actor *self, void *userData )
{
	// todo: rotate projectile relative to velocity, so it's always facing the right way

	if ( !SGActor_Generic_InsideBounds( self ) )
	{
		//	Act_DestroyActor( self );
		//	return;
	}

	SGActor_Generic_UpdateParticleEmitter( self, userData );
}

const ActorSetup sg_actorProjectileSetup = {
		.id = "point.sg.projectile",
		.Spawn = Projectile_Spawn,
		.Tick = Projectile_Tick,
		.Draw = SGActor_Generic_Draw,
		.Collide = SGActor_Generic_Collide,
		.Destroy = SGActor_Generic_Destroy,
		.Serialize = NULL,
		.Deserialize = NULL,
};
