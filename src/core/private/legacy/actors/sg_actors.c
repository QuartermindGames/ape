// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "acm/public/acm/acm.h"

#include "ape_private.h"
#include "../actor.h"

#include "renderer/renderer_particle.h"

#define MODEL_SCALE 10.0f

typedef struct ASGActor
{
	//PLMModel *model;

	float forwardVelocity;
	float scale;

	SS_Arl_ParticleEmitter *particleEmitter;
	SS_Arl_ParticleEmitter *emitLeft, *emitRight;

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
