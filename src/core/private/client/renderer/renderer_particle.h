// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

typedef struct ApeCamera ApeCamera;

typedef enum SS_Arl_ParticleDrawType
{
	SS_ARL_PARTICLE_DRAW_TYPE_SPRITE,
	SS_ARL_PARTICLE_DRAW_TYPE_MODEL,
} SS_Arl_ParticleDrawType;

typedef struct SS_Arl_ParticleEmitter
{
	ApeSceneTransform transform, transformVar;

	PLVector3 force, forceVar; /* exterior forces, such as gravity */

	int emissionRate, emissionVar; /* how many particles to emit per tick */

	int numTicks, maxTicks; /* number of ticks since last emission and maximum ticks until we emit again */

	int particleLife, particleLifeVar; /* how long the particles spawned by the emitter will live until they die */
	int life;                          /* how long this emitter will live until it's removed */

	float speed, speedVar;

	/* particle colour */
	PLColourF32 startColour, startColourVar;
	PLColourF32 endColour, endColourVar;

	float startScale, endScale, scaleVar;

	int maxParticles; /* maximum number of particles at a time */

	PLCollisionAABB bounds;

	struct PLGMesh *mesh;
	struct ApeMaterial *material;
	ApeMemoryReference mem;

	struct PLLinkedList *particles;
} SS_Arl_ParticleEmitter;

typedef struct SS_Arl_Particle
{
	ApeSceneTransform transform, oldTransform;

	PLVector3 dir;

	PLColourF32 colour;
	PLColourF32 oldColour;
	PLColourF32 deltaColour;

	float scale, oldScale, deltaScale;

	int life;
	SS_Arl_ParticleEmitter *emitter;

	PLCollisionAABB bounds;

	struct PLLinkedListNode *node;
} SS_Arl_Particle;

void ss_arl_cache_particle_emitter_template( const char *path );
SS_Arl_ParticleEmitter *ss_arl_particle_emitter_create( void );
void ss_arl_particle_emitter_destroy( SS_Arl_ParticleEmitter *emitter );

void ss_arl_particle_emitter_tick( SS_Arl_ParticleEmitter *emitter );
void ss_arl_particle_emitter_draw( const SS_Arl_ParticleEmitter *emitter, const ApeCamera *camera );
