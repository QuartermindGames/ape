// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

typedef struct ApeCamera ApeCamera;

typedef enum ApeParticleDrawType
{
	SS_ARL_PARTICLE_DRAW_TYPE_SPRITE,
	SS_ARL_PARTICLE_DRAW_TYPE_MODEL,
} ApeParticleDrawType;

typedef struct ApeParticleEmitter
{
	ApeSceneTransform transform, transformVar;

	QmMathVector3f force, forceVar; /* exterior forces, such as gravity */

	int emissionRate, emissionVar; /* how many particles to emit per tick */

	int numTicks, maxTicks; /* number of ticks since last emission and maximum ticks until we emit again */

	int particleLife, particleLifeVar; /* how long the particles spawned by the emitter will live until they die */
	int life;                          /* how long this emitter will live until it's removed */

	float speed, speedVar;

	/* particle colour */
	QmMathColour4f startColour, startColourVar;
	QmMathColour4f endColour, endColourVar;

	float startScale, endScale, scaleVar;

	int maxParticles; /* maximum number of particles at a time */

	PLCollisionAABB bounds;

	struct QmGfxMesh   *mesh;
	struct ApeMaterial *material;
	ApeMemoryReference  mem;

	struct PLLinkedList *particles;

	unsigned int seed;
} ApeParticleEmitter;

typedef struct ApeParticle
{
	ApeSceneTransform transform, oldTransform;

	QmMathVector3f dir;

	QmMathColour4f colour;
	QmMathColour4f oldColour;
	QmMathColour4f deltaColour;

	float scale, oldScale, deltaScale;

	int                 life;
	ApeParticleEmitter *emitter;

	PLCollisionAABB bounds;

	struct PLLinkedListNode *node;
} ApeParticle;

void                ss_arl_cache_particle_emitter_template( const char *path );
ApeParticleEmitter *ss_arl_particle_emitter_create( void );
void                ss_arl_particle_emitter_destroy( ApeParticleEmitter *emitter );

void ss_arl_particle_emitter_tick( ApeParticleEmitter *emitter );
void ss_arl_particle_emitter_draw( const ApeParticleEmitter *emitter, const ApeCamera *camera );
