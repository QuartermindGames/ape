/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

#include "scenegraph.h"

typedef struct PSEmitter
{
	SGTransform  transform, transformVar;
	unsigned int emissionRate, emissionVar;     /* how many particles to emit per tick */
	int          particleLife, particleLifeVar; /* how long the particles spawned by the emitter will live until they die */
	int          life;                          /* how long this emitter will live until it's removed */
	PLColour     startColour, startColourVar;
	PLColour     endColour, endColourVar;
	unsigned int maxParticles; /* maximum number of particles at a time */

	struct PLGMesh * mesh;
	struct Material *material;
	MEMReference     mem;

	struct PLLinkedList *particles;
} PSEmitter;

typedef struct PSParticle
{
	SGTransform transform, oldTransform;
	PLColour    colour, oldColour, deltaColour;
	int         life;
	PSEmitter * emitter;

	struct PLLinkedListNode *node;
} PSParticle;

void PS_Initialize( void );
void PS_Shutdown( void );

void       PS_CacheEmitterTemplate( const char *path );
PSEmitter *PS_SpawnEmitter( const char *path );

void PS_TickEmitter( PSEmitter *emitter );
void PS_Draw( const struct PLGCamera *camera );
