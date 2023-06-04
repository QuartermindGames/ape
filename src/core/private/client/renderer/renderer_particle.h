// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "renderer_scenegraph.h"

typedef struct ApeCamera ApeCamera;

typedef enum PSParticleDrawType
{
	PS_DRAW_SPRITE,
	PS_DRAW_MODEL,
} PSParticleDrawType;

typedef struct PSEmitter
{
	SGTransform transform, transformVar;

	PLVector3 force, forceVar; /* exterior forces, such as gravity */

	int emissionRate, emissionVar; /* how many particles to emit per tick */

	int numTicks, maxTicks; /* number of ticks since last emission and maximum ticks until we emit again */

	int particleLife, particleLifeVar; /* how long the particles spawned by the emitter will live until they die */
	int life;						   /* how long this emitter will live until it's removed */

	float speed, speedVar;

	/* particle colour */
	PLColourF32 startColour, startColourVar;
	PLColourF32 endColour, endColourVar;

	float startScale, endScale, scaleVar;

	int maxParticles; /* maximum number of particles at a time */

	PLCollisionAABB bounds;

	struct PLGMesh  *mesh;
	struct ApeMaterial *material;
	ApeMemoryReference mem;

	struct PLLinkedList *particles;
} PSEmitter;

typedef struct PSParticle
{
	SGTransform transform, oldTransform;

	PLVector3 dir;

	PLColourF32 colour;
	PLColourF32 oldColour;
	PLColourF32 deltaColour;

	float scale, oldScale, deltaScale;

	int		   life;
	PSEmitter *emitter;

	PLCollisionAABB bounds;

	struct PLLinkedListNode *node;
} PSParticle;

void PS_Initialize( void );
void PS_Shutdown( void );

void	   PS_CacheEmitterTemplate( const char *path );
PSEmitter *PS_SpawnEmitter( void );
void	   PS_DestroyEmitter( PSEmitter *emitter );

void PS_TickEmitter( PSEmitter *emitter );
void PS_Draw( const PSEmitter *emitter, const ApeCamera *camera );
