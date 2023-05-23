/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plmodel/plm.h>

#include <yin/node.h>

#include "core_private.h"
#include "../actor.h"
#include "game_interface.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_particle.h"

#include "core_model.h"

#define MODEL_SCALE 10.0f

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
} ASGActor;

typedef struct AsteroidManager
{
	ASGActor     base;
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
	ASGActor *sgActor = PlMAlloc( sizeof( ASGActor ), true );
	Act_SetUserData( self, sgActor );

	return sgActor;
}

static void SGActor_Generic_UpdateParticleEmitter( Actor *self, ASGActor *sgSelf )
{
	PLVector3 forward, left;
	PlAnglesAxes( PLVector3( 0, self->angles.y, 0 ), &left, NULL, &forward );

	if ( sgSelf->particleEmitter != NULL )
	{
		PLVector3 cpos                                 = PlSubtractVector3( self->position, PlScaleVector3F( forward, 20.0f ) );
		sgSelf->particleEmitter->transform.translation = cpos;
		PS_TickEmitter( sgSelf->particleEmitter );
	}

	if ( sgSelf->emitLeft != NULL )
	{
		PLVector3 lpos                          = PlAddVector3( PlSubtractVector3( self->position, PlScaleVector3F( forward, 20.0f ) ), PlScaleVector3F( left, 32.0f ) );
		sgSelf->emitLeft->transform.translation = lpos;
		PS_TickEmitter( sgSelf->emitLeft );
	}

	if ( sgSelf->emitRight != NULL )
	{
		PLVector3 rpos                           = PlSubtractVector3( PlSubtractVector3( self->position, PlScaleVector3F( forward, 20.0f ) ), PlScaleVector3F( left, 32.0f ) );
		sgSelf->emitRight->transform.translation = rpos;
		PS_TickEmitter( sgSelf->emitRight );
	}
}

static void SGActor_Generic_Collide( Actor *self, Actor *other, void *userData )
{
#if 0
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

#	if 0
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
#	endif
			}

			Act_DestroyActor( other );
			return;
		}
	}
	else if ( oldHealth > 0 && other->health <= 0 )
	{
		A_EmitSound( gameEndSound, 100 );
	}

	Monster_Collide( self, other, 2.0f ); //2.0f + sg->scale );
#endif
}

static void SGActor_Generic_Draw( Actor *self, void *userData )
{
	OgeCamera *camera = ogeGetActiveCamera();
	if ( camera == NULL )
		return;

	ASGActor *sgActor = userData;
	if ( sgActor->model != NULL )
	{
		PlMatrixMode( PL_MODELVIEW_MATRIX );
		PlPushMatrix();

		PlLoadIdentityMatrix();

		PlScaleMatrix( PLVector3( MODEL_SCALE + sgActor->scale, MODEL_SCALE + sgActor->scale, MODEL_SCALE + sgActor->scale ) );

		float x = PL_DEG2RAD( self->angles.x - 90.0f );
		PlRotateMatrix( x, 1.0f, 0.0f, 0.0f );
		float y = PL_DEG2RAD( self->angles.y );
		PlRotateMatrix( y, 0.0f, 1.0f, 0.0f );
		float z = PL_DEG2RAD( self->angles.z );
		PlRotateMatrix( z, 0.0f, 0.0f, 1.0f );

		PlTranslateMatrix( Act_GetPosition( self ) );

#if 0
		PLMatrix4 m = PlMultiplyMatrix4( camera->internal->internal.proj, camera->internal->internal.view );
		PLVector2 sv = PlConvertWorldToScreen( &self->position, &m );
		sv.y = (camera->internal->viewport.h / 2.0f) * ( sv.y + ( 1.0f * 2.0f ) );
		printf( "%s\n", PlPrintVector2( &sv, pl_float_var ) );
#endif

		for ( unsigned int i = 0; i < sgActor->model->numMeshes; ++i )
		{
			MDLUserData *modelData = sgActor->model->userData;
			ogeMaterial_DrawMesh( modelData->materials[ i ], sgActor->model->meshes[ i ], NULL, 0 );
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

	PlFree( sgActor );
}

static void SGActor_Generic_SetModel( Actor *self, const char *path )
{
	ASGActor *sgActor = Act_GetUserData( self );
	sgActor->model    = PlmLoadModel( path );
	if ( sgActor->model == NULL )
	{
		PRINT_WARNING( "Failed to load model, \"%s\", for actor!\n", path );
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
	ship->isSolid  = true;

	Act_SetBounds( self, SHIP_BOUNDS_MINS, SHIP_BOUNDS_MAXS );

	SGActor_Generic_SetModel( self, "models/player_ship.node" );

	self->health       = 100;
	self->movementType = ACTOR_MOVEMENT_PHYSICS;

	ship->particleEmitter                  = PS_SpawnEmitter();
	ship->particleEmitter->emissionRate    = 0;
	ship->particleEmitter->emissionVar     = 0;
	ship->particleEmitter->speed           = 2;
	ship->particleEmitter->speedVar        = 5;
	ship->particleEmitter->particleLife    = 2;
	ship->particleEmitter->particleLifeVar = 1;
	ship->particleEmitter->maxParticles    = SHIP_MAX_PARTICLES;
	ship->particleEmitter->startColour     = PL_COLOURF32( 1.0f, 0.5f, 0.5f, 1.0f );
	//ship->particleEmitter->startColourVar			= PlColourF32( 0.02f, 0.05f, 0.1f, 0.0f );
	ship->particleEmitter->endColour                = PL_COLOURF32( 1.0f, 0.2f, 0.2f, 0.0f );
	ship->particleEmitter->forceVar                 = PLVector3( 0.0f, 0.05f, 0.0f );
	ship->particleEmitter->transform.translation    = Act_GetPosition( self );
	ship->particleEmitter->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );
	ship->particleEmitter->material                 = YnCore_Material_Cache( "materials/effects/particle.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false );

	ship->emitLeft                           = PS_SpawnEmitter();
	ship->emitLeft->emissionRate             = 4;
	ship->emitLeft->emissionVar              = 0;
	ship->emitLeft->speed                    = 2;
	ship->emitLeft->speedVar                 = 5;
	ship->emitLeft->particleLife             = 2;
	ship->emitLeft->particleLifeVar          = 1;
	ship->emitLeft->maxParticles             = SHIP_MAX_PARTICLES;
	ship->emitLeft->startColour              = PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.0f );
	ship->emitLeft->endColour                = PL_COLOURF32( 0.2f, 0.2f, 0.2f, 0.0f );
	ship->emitLeft->forceVar                 = PLVector3( 0.0f, 0.05f, 0.0f );
	ship->emitLeft->transform.translation    = Act_GetPosition( self );
	ship->emitLeft->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );
	ship->emitLeft->material                 = YnCore_Material_Cache( "materials/effects/particle.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false );

	ship->emitRight                           = PS_SpawnEmitter();
	ship->emitRight->emissionRate             = 4;
	ship->emitRight->emissionVar              = 0;
	ship->emitRight->speed                    = 2;
	ship->emitRight->speedVar                 = 5;
	ship->emitRight->particleLife             = 2;
	ship->emitRight->particleLifeVar          = 1;
	ship->emitRight->maxParticles             = SHIP_MAX_PARTICLES;
	ship->emitRight->startColour              = PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.0f );
	ship->emitRight->endColour                = PL_COLOURF32( 0.2f, 0.2f, 0.2f, 0.0f );
	ship->emitRight->forceVar                 = PLVector3( 0.0f, 0.05f, 0.0f );
	ship->emitRight->transform.translation    = Act_GetPosition( self );
	ship->emitRight->transformVar.translation = PLVector3( 10.0f, 10.0f, 10.0f );
	ship->emitRight->material                 = YnCore_Material_Cache( "materials/effects/particle.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false );

	OgeCamera *camera      = ogeGetActiveCamera();
	camera->mode        = OGE_CAMERA_MODE_TOP;
	camera->parentActor = self;
}

static void Ship_Destroy( Actor *self, void *userData )
{
	ASGActor *sg = userData;
	PlmDestroyModel( sg->model );

	SGActor_Generic_Destroy( self, userData );
}

#define TURN_SPEED 5.0f
#define MAX_SPEED  4.0f

static void Ship_Tick( Actor *self, void *userData )
{
	SGActor_Generic_Wrap( self );
	SGActor_Generic_UpdateParticleEmitter( self, userData );

	ASGActor *sg = userData;
	if ( PlVector3Length( self->velocity ) <= 1.0f )
	{
		sg->emitLeft->maxParticles        = 0;
		sg->emitRight->maxParticles       = 0;
		sg->particleEmitter->maxParticles = 0;
	}
	else
	{
		sg->emitLeft->maxParticles        = SHIP_MAX_PARTICLES;
		sg->emitRight->maxParticles       = SHIP_MAX_PARTICLES;
		sg->particleEmitter->maxParticles = SHIP_MAX_PARTICLES;
	}

	if ( self->health <= 0 )
	{
		return;
	}

	if ( YnCore_ShellInterface_GetKeyState( KEY_LEFT ) ||
	     YnCore_ShellInterface_GetKeyState( 'a' ) )
		self->angles.y += TURN_SPEED;
	else if ( YnCore_ShellInterface_GetKeyState( KEY_RIGHT ) ||
	          YnCore_ShellInterface_GetKeyState( 'd' ) )
		self->angles.y -= TURN_SPEED;

	static const float incAmount = 0.0015f;

	if ( YnCore_ShellInterface_GetKeyState( KEY_UP ) ||
	     YnCore_ShellInterface_GetKeyState( 'w' ) )
		sg->forwardVelocity += incAmount;
	else if ( YnCore_ShellInterface_GetKeyState( KEY_DOWN ) ||
	          YnCore_ShellInterface_GetKeyState( 's' ) )
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

	if ( YnCore_ShellInterface_GetKeyState( KEY_LEFT_CTRL ) && ( sg->fireDelay < ogeGetNumTicks() ) )
	{
		Actor *projectile    = Act_SpawnActorById( "point.sg.projectile", NULL );
		projectile->position = self->position;

		projectile->velocity = PlScaleVector3F( self->forward, 32.0f );
		projectile->angles   = self->angles;
		projectile->angles.y += -90.0f;

		projectile->parent = self;

		sg->fireDelay = ogeGetNumTicks() + 25;
	}

	static unsigned int survivalScoreTimer = 0;
	if ( self->health > 0 && survivalScoreTimer < ogeGetNumTicks() )
	{
		self->score++;
		survivalScoreTimer = ogeGetNumTicks() + 145;
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
        .id          = "point.sg.ship",
        .Spawn       = Ship_Spawn,
        .Tick        = Ship_Tick,
        .Draw        = SGActor_Generic_Draw,
        .Collide     = Ship_Collide,
        .Destroy     = Ship_Destroy,
        .Serialize   = NULL,
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
#if 0
	ASGActor *sg = Act_GetUserData( self );
	self->position.y = sg->spawnPosition.y + ( ( 1.0f + sinf( ( Engine_GetNumTicks() + sg->variance.y ) / 100.0f ) ) / 10.0f ) * sg->variance.y;
	self->position.z = sg->spawnPosition.z + ( ( 1.0f + cosf( ( Engine_GetNumTicks() + sg->variance.z ) / 100.0f ) ) / 10.0f ) * sg->variance.z;

	self->angles.x = ( ( ( 1.0f + cosf( ( Engine_GetNumTicks() + sg->variance.x ) / 100.0f ) ) / 10.0f ) * ( sg->variance.x * 2.0f ) );
	//self->angles.y -= ( ( 1.0f + sinf( Engine_GetNumTicks() / 100.0f ) ) / 10.0f ) * 16.0f;
	//self->angles.z -= ( ( 1.0f + cosf( Engine_GetNumTicks() / 100.0f ) ) / 10.0f ) * 16.0f;
#else
	self->angles.y += 0.095f;
#endif
}

static void Prop_Deserialize( Actor *self, NdBranch *nodeTree )
{
	const char *modelPath = ndGetStringByName( nodeTree, "modelPath", NULL );
	if ( modelPath != NULL )
	{
		SGActor_Generic_SetModel( self, modelPath );
	}

	ASGActor *sg      = Act_GetUserData( self );
	sg->spawnPosition = self->position;
	sg->spawnAngles   = self->oldPosition;

	sg->variance.x = ( float ) ( rand() % 5 );
	sg->variance.y = ( float ) ( rand() % 5 );
	sg->variance.z = ( float ) ( rand() % 5 );
}

const ActorSetup sg_actorPropSetup = {
        .id          = "point.sg.prop",
        .Spawn       = Prop_Spawn,
        .Tick        = Prop_Tick,
        .Draw        = SGActor_Generic_Draw,
        .Collide     = NULL,
        .Destroy     = SGActor_Generic_Destroy,
        .Serialize   = NULL,
        .Deserialize = Prop_Deserialize,
};
