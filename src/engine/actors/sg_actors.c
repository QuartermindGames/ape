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
#include "game_interface.h"

#include "client/audio/audio.h"
#include "client/renderer/renderer.h"
#include "client/renderer/particle.h"

#include "model.h"

#define MODEL_SCALE 10.0f

#define MAX_ASTEROID_MODELS 2
static PLMModel *asteroidModels[ MAX_ASTEROID_MODELS ] = { NULL, NULL };

static void Asteroid_SetScale( Actor *self, float scale );

static PLMModel *projectileModel = NULL;

static ASound *impactSound = NULL;
static ASound *thrustSound = NULL;
static ASound *fireSound = NULL;
static ASound *gameEndSound = NULL;
static ASound *gameStartSound = NULL;

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

	PLVector3 spawnPosition;
	PLVector3 spawnAngles;

	PLVector3 variance;

	unsigned int fireDelay;

	bool isSolid;
	bool shouldDraw;
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
	ASGActor *sg = globalSystem.MAlloc( sizeof( ASGActor ), true );
	Act_SetUserData( self, sg );

	sg->shouldDraw = true;

	return sg;
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
	ASGActor *sg = self->userData;
	if ( sg == NULL )
	{
		return;
	}

	if ( ( self->type == ACTOR_SG_PROJECTILE && other->type == ACTOR_SG_SHIP ) || !sg->isSolid )
		return;

	int damageAmount;
	switch ( Game_GetDifficultyMode() )
	{
		default:
		case GAME_DIFFICULTY_NORMAL:
			damageAmount = 8;
			break;
		case GAME_DIFFICULTY_EASY:
			damageAmount = 2;
			break;
		case GAME_DIFFICULTY_HARD:
			damageAmount = 15;
			break;
	}

	int oldHealth = other->health;
	if ( other->health > 0 )
	{
		if ( other->type == ACTOR_SG_SHIP )
		{
			other->health -= damageAmount;
			A_EmitSound( impactSound, 45 );
		}
		else if ( self->type == ACTOR_SG_PROJECTILE && other->type == ACTOR_SG_ASTEROID )
		{
			other->health -= 10;
			A_EmitSound( impactSound, 35 );
		}
	}

	if ( other->type != ACTOR_SG_SHIP )
	{
		if ( other->health <= 0 )
		{
			/* special logic for asteroids shoved in here,
			 * so they break up if smashed into a *bigger*
			 * asteroid */
			if ( other->type == ACTOR_SG_ASTEROID )
			{
				if ( self->type == ACTOR_SG_PROJECTILE )
				{
					self->parent->score += 10;
				}

#if 0
				if ( sg->scale > 1.0f )
				{
					for ( unsigned int i = 0; i < 3; ++i )
					{
						Actor *asteroid = Act_SpawnActor( ACTOR_SG_ASTEROID, NULL );

						Asteroid_SetScale( asteroid, sg->scale / 3.0f );

						asteroid->position = other->position;
						asteroid->collisionVolume.origin = asteroid->position;
					}
				}
#endif
			}

			Act_DestroyActor( other );
			return;
		}
	}
	else if ( oldHealth > 0 && other->health <= 0 )
	{
		A_EmitSound( gameEndSound, 100 );
	}

	Monster_Collide( self, other, 2.0f );//2.0f + sg->scale );
}

static void SGActor_Generic_Draw( Actor *self, void *userData )
{
	Camera *camera = R_GetGlobalCamera();
	if ( camera == NULL )
		return;

	ASGActor *sg = userData;
	if ( sg->model != NULL && sg->shouldDraw )
	{
		PlMatrixMode( PL_MODELVIEW_MATRIX );
		PlPushMatrix();

		PlLoadIdentityMatrix();

		PlScaleMatrix( PLVector3( MODEL_SCALE + sg->scale, MODEL_SCALE + sg->scale, MODEL_SCALE + sg->scale ) );

		float x = PlDegreesToRadians( self->angles.x - 90.0f );
		PlRotateMatrix( x, 1.0f, 0.0f, 0.0f );
		float y = PlDegreesToRadians( self->angles.y );
		PlRotateMatrix( y, 0.0f, 1.0f, 0.0f );
		float z = PlDegreesToRadians( self->angles.z );
		PlRotateMatrix( z, 0.0f, 0.0f, 1.0f );

		PlTranslateMatrix( Act_GetPosition( self ) );

		for ( unsigned int i = 0; i < sg->model->numMeshes; ++i )
		{
			MDLUserData *modelData = sg->model->userData;
			RM_DrawMesh( modelData->materials[ i ], sg->model->meshes[ i ] );
		}

		PlPopMatrix();
	}

	if ( sg->particleEmitter != NULL )
	{
		PS_Draw( sg->particleEmitter, camera );
	}
	if ( sg->emitRight != NULL )
	{
		PS_Draw( sg->emitRight, camera );
	}
	if ( sg->emitLeft != NULL )
	{
		PS_Draw( sg->emitLeft, camera );
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

	sgActor->model->bounds.mins = PlSubtractVector3F( sgActor->model->bounds.mins, MODEL_SCALE );
	sgActor->model->bounds.maxs = PlAddVector3F( sgActor->model->bounds.maxs, MODEL_SCALE );

	Act_SetBounds( self, sgActor->model->bounds.mins, sgActor->model->bounds.maxs );
	Act_SetVisibilityVolume( self, &sgActor->model->bounds.mins, &sgActor->model->bounds.maxs );
}

/****************************************
 * point.sg.ship
 ****************************************/

#define SHIP_BOUNDS_MAXS PLVector3( 16.0f, 90.0f, 16.0f )
#define SHIP_BOUNDS_MINS PLVector3( -16.0f, 0.0f, -16.0f )

#define SHIP_MAX_PARTICLES 100

static void Ship_Spawn( Actor *self )
{
	ASGActor *ship = SGActor_Generic_Spawn( self );
	ship->isSolid = true;

	Act_SetBounds( self, SHIP_BOUNDS_MINS, SHIP_BOUNDS_MAXS );

	SGActor_Generic_SetModel( self, "models/player_ship.node" );

	self->health = 100;
	self->movementType = ACTOR_MOVEMENT_PHYSICS;

	ship->particleEmitter = PS_SpawnEmitter( PS_DRAW_SPRITE );
	ship->particleEmitter->emissionRate = 0;
	ship->particleEmitter->emissionVar = 0;
	ship->particleEmitter->speed = 2;
	ship->particleEmitter->speedVar = 5;
	ship->particleEmitter->particleLife = 2;
	ship->particleEmitter->particleLifeVar = 1;
	ship->particleEmitter->maxParticles = SHIP_MAX_PARTICLES;
	ship->particleEmitter->startColour = PlColourF32( 1.0f, 0.5f, 0.5f, 1.0f );
	ship->particleEmitter->endColour = PlColourF32( 1.0f, 0.2f, 0.2f, 0.0f );
	ship->particleEmitter->forceVar = PLVector3( 0.0f, 0.05f, 0.0f );
	ship->particleEmitter->transform.translation = Act_GetPosition( self );
	ship->particleEmitter->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );
	ship->particleEmitter->material = RM_CacheMaterial( "materials/effects/particle.mat", CACHE_GROUP_WORLD, true );

	ship->emitLeft = PS_SpawnEmitter( PS_DRAW_TRAIL );
	ship->emitLeft->emissionRate = 4;
	ship->emitLeft->emissionVar = 0;
	ship->emitLeft->speed = 2;
	ship->emitLeft->speedVar = 5;
	ship->emitLeft->particleLife = 2;
	ship->emitLeft->particleLifeVar = 1;
	ship->emitLeft->maxParticles = SHIP_MAX_PARTICLES;
	ship->emitLeft->startColour = PlColourF32( 1.0f, 1.0f, 1.0f, 1.0f );
	ship->emitLeft->endColour = PlColourF32( 0.2f, 0.2f, 0.2f, 0.0f );
	ship->emitLeft->forceVar = PLVector3( 0.0f, 0.05f, 0.0f );
	ship->emitLeft->transform.translation = Act_GetPosition( self );
	ship->emitLeft->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );
	ship->emitLeft->material = RM_CacheMaterial( "materials/effects/particle.mat", CACHE_GROUP_WORLD, true );

	ship->emitRight = PS_SpawnEmitter( PS_DRAW_TRAIL );
	ship->emitRight->emissionRate = 4;
	ship->emitRight->emissionVar = 0;
	ship->emitRight->speed = 2;
	ship->emitRight->speedVar = 5;
	ship->emitRight->particleLife = 2;
	ship->emitRight->particleLifeVar = 1;
	ship->emitRight->maxParticles = SHIP_MAX_PARTICLES;
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

static void Ship_Destroy( Actor *self, void *userData )
{
	ASGActor *sg = userData;
	PlmDestroyModel( sg->model );

	SGActor_Generic_Destroy( self, userData );
}

#define TURN_SPEED	5.0f
#define MAX_SPEED	4.0f
#define ACCEL_SPEED 0.0055f

static void Ship_Tick( Actor *self, void *userData )
{
	SGActor_Generic_Wrap( self );
	SGActor_Generic_UpdateParticleEmitter( self, userData );

	ASGActor *sg = userData;
	if ( PlVector3Length( self->velocity ) <= 1.0f )
	{
		sg->emitLeft->maxParticles = 0;
		sg->emitRight->maxParticles = 0;
		sg->particleEmitter->maxParticles = 0;
	}
	else
	{
		sg->emitLeft->maxParticles = SHIP_MAX_PARTICLES;
		sg->emitRight->maxParticles = SHIP_MAX_PARTICLES;
		sg->particleEmitter->maxParticles = SHIP_MAX_PARTICLES;
	}

	if ( self->health <= 0 )
	{
		sg->shouldDraw = false;
		return;
	}

	if ( globalSystem.GetKeyState( KEY_LEFT ) ||
		 globalSystem.GetKeyState( 'a' ) )
		self->angles.y += TURN_SPEED;
	else if ( globalSystem.GetKeyState( KEY_RIGHT ) ||
			  globalSystem.GetKeyState( 'd' ) )
		self->angles.y -= TURN_SPEED;

	static const float incAmount = ACCEL_SPEED;

	if ( globalSystem.GetKeyState( KEY_UP ) ||
		 globalSystem.GetKeyState( 'w' ) )
		sg->forwardVelocity += incAmount;
	else if ( globalSystem.GetKeyState( KEY_DOWN ) ||
			  globalSystem.GetKeyState( 's' ) )
		sg->forwardVelocity -= incAmount;
	else if ( sg->forwardVelocity != 0.0f )
	{
		sg->forwardVelocity = sg->forwardVelocity > 0 ? sg->forwardVelocity - incAmount : sg->forwardVelocity + incAmount;
		if ( sg->forwardVelocity < 0.1f && sg->forwardVelocity > -0.1f )
			sg->forwardVelocity = 0.0f;
	}

	/* clamp the velocity as necessary */
	sg->forwardVelocity = PlClamp( -MAX_SPEED, sg->forwardVelocity, MAX_SPEED );

	self->velocity = PlAddVector3( self->velocity, PlScaleVector3F( Act_GetForward( self ), sg->forwardVelocity ) );

	if ( globalSystem.GetKeyState( KEY_LEFT_CTRL ) && ( sg->fireDelay < Engine_GetNumTicks() ) )
	{
		Actor *projectile = Act_SpawnActorById( "point.sg.projectile", NULL );
		projectile->position = self->position;

		projectile->velocity = PlScaleVector3F( self->forward, 32.0f );
		projectile->angles = self->angles;
		projectile->angles.y += -90.0f;

		projectile->parent = self;

		A_EmitSound( fireSound, 50 );

		sg->fireDelay = Engine_GetNumTicks() + 25;
	}

	static unsigned int survivalScoreTimer = 0;
	if ( self->health > 0 && survivalScoreTimer < Engine_GetNumTicks() )
	{
		self->score++;
		survivalScoreTimer = Engine_GetNumTicks() + 145;
	}
}

static void Ship_Collide( Actor *self, Actor *other, void *userData )
{
	if ( self->health <= 0 )
	{
		return;
	}

	ASGActor *sg = self->userData;
	if ( other->type == ACTOR_SG_PROJECTILE || !sg->isSolid )
	{
		return;
	}

	if ( sg->forwardVelocity > 0.0f )
	{
		sg->forwardVelocity /= 2.0f;
	}

	Monster_Collide( self, other, 20.0f );
}

const ActorSetup sg_actorShip = {
		.id = "point.sg.ship",
		.Spawn = Ship_Spawn,
		.Tick = Ship_Tick,
		.Draw = SGActor_Generic_Draw,
		.Collide = Ship_Collide,
		.Destroy = Ship_Destroy,
		.Serialize = NULL,
		.Deserialize = NULL,
};

/****************************************
 * point.sg.asteroid
 ****************************************/

static void Asteroid_SetScale( Actor *self, float scale )
{
	ASGActor *sg = Act_GetUserData( self );

	sg->scale = scale;
	//scale += MODEL_SCALE;

	//sg->model->bounds.mins = PlSubtractVector3F( sg->model->bounds.mins, scale );
	//sg->model->bounds.maxs = PlAddVector3F( sg->model->bounds.maxs, scale );

	self->collisionVolume.mins = PlSubtractVector3F( sg->model->bounds.mins, scale + MODEL_SCALE );
	self->collisionVolume.maxs = PlAddVector3F( sg->model->bounds.maxs, scale + MODEL_SCALE );
	self->visibilityVolume.mins = PlSubtractVector3F( sg->model->bounds.mins, scale + MODEL_SCALE * 4.0f );
	self->visibilityVolume.maxs = PlAddVector3F( sg->model->bounds.maxs, scale + MODEL_SCALE * 4.0f );
}

static void Asteroid_Spawn( Actor *self )
{
	ASGActor *sg = SGActor_Generic_Spawn( self );

	sg->isSolid = true;
	sg->model = ( rand() % MAX_ASTEROID_MODELS == 0 ) ? asteroidModels[ 0 ] : asteroidModels[ 1 ];

	self->health = rand() % 15 + 1;

	Asteroid_SetScale( self, self->health * 2 );

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

	float x, y;
	GameDifficulty difficulty = Game_GetDifficultyMode();
	if ( difficulty == GAME_DIFFICULTY_HARD )
	{
		x = -5.0f;
		y = 5.0f;
	}
	else
	{
		x = -3.0f;
		y = 3.0f;
	}

	self->velocity = PlClampVector3( &self->velocity, x, y );
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
	asteroid->collisionVolume.origin = asteroid->position;

	if ( Act_IsVisible( asteroid ) || ( Act_CheckCollisions( asteroid ) != NULL ) )
	{
		Act_DestroyActor( asteroid );
		return;
	}

	asteroid->velocity = PLVector3(
			PlGenerateRandomFloat( 4.0f ) - PlGenerateRandomFloat( 4.0f ),
			0.0f,
			PlGenerateRandomFloat( 4.0f ) - PlGenerateRandomFloat( 4.0f ) );
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
	ASGActor *sg = Act_GetUserData( self );
	self->position.y = sg->spawnPosition.y + ( ( 1.0f + sinf( ( Engine_GetNumTicks() + sg->variance.y ) / 100.0f ) ) / 10.0f ) * sg->variance.y;
	self->position.z = sg->spawnPosition.z + ( ( 1.0f + cosf( ( Engine_GetNumTicks() + sg->variance.z ) / 100.0f ) ) / 10.0f ) * sg->variance.z;

	self->angles.x = ( ( ( 1.0f + cosf( ( Engine_GetNumTicks() + sg->variance.x ) / 100.0f ) ) / 10.0f ) * ( sg->variance.x * 2.0f ) );
	//self->angles.y -= ( ( 1.0f + sinf( Engine_GetNumTicks() / 100.0f ) ) / 10.0f ) * 16.0f;
	//self->angles.z -= ( ( 1.0f + cosf( Engine_GetNumTicks() / 100.0f ) ) / 10.0f ) * 16.0f;
}

static void Prop_Deserialize( Actor *self, NLNode *nodeTree )
{
	const char *modelPath = NL_GetStrByName( nodeTree, "modelPath", NULL );
	if ( modelPath != NULL )
	{
		SGActor_Generic_SetModel( self, modelPath );
	}

	ASGActor *sg = Act_GetUserData( self );
	sg->spawnPosition = self->position;
	sg->spawnAngles = self->oldPosition;

	sg->variance.x = rand() % 5;
	sg->variance.y = rand() % 5;
	sg->variance.z = rand() % 5;
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
}

static void Projectile_Tick( Actor *self, void *userData )
{
	if ( !SGActor_Generic_InsideBounds( self ) )
	{
		Act_DestroyActor( self );
		return;
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
