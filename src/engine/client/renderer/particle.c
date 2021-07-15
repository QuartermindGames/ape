/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plcore/pl_linkedlist.h>

#include "yin.h"
#include "particle.h"
#include "renderer.h"

#include "common/node.h"

void PS_Initialize( void )
{
}

void PS_Shutdown( void )
{
}

static void PS_CB_DestroyEmitterTemplate( void *userData )
{
	PSEmitter *emitter = userData;
	u_assert( emitter != NULL );
	if ( emitter == NULL )
		return;

	RM_ReleaseMaterial( emitter->material );

	PlgDestroyMesh( emitter->mesh );

	globalSystem.Free( emitter );
}

void PS_CacheEmitterTemplate( const char *path )
{
	PSEmitter *emitter = MEM_GetCachedData( path, MEM_CACHE_PARTICLES );
	if ( emitter != NULL )
		return emitter;

	NLNode *root = NL_LoadFile( path, "particleEmitter" );
	if ( root == NULL )
	{
		PrintWarn( "Failed to load particle emitter template: %s\n" );
		return;
	}

	emitter = globalSystem.MAlloc( sizeof( PSEmitter ), true );

	SG_DS_Transform( root, "transform", &emitter->transform );
	SG_DS_Transform( root, "transformVar", &emitter->transformVar );

	emitter->emissionRate = NL_GetI32ByName( root, "emissionRate", 0 );
	emitter->emissionVar  = NL_GetI32ByName( root, "emissionVar", 0 );

	emitter->particleLife    = NL_GetI32ByName( root, "particleLife", 0 );
	emitter->particleLifeVar = NL_GetI32ByName( root, "particleLifeVar", 0 );
	emitter->maxParticles    = NL_GetI32ByName( root, "maxParticles", 0 );

	emitter->life = NL_GetI32ByName( root, "life", 0 );

	NLNode *n;
	if ( ( n = NL_GetChildByName( root, "startColour" ) ) == NULL )
		NL_DS_DeserializeColour( n, &emitter->startColour );
	if ( ( n = NL_GetChildByName( root, "endColour" ) ) == NULL )
		NL_DS_DeserializeColour( n, &emitter->endColour );
	if ( ( n = NL_GetChildByName( root, "startColourVar" ) ) == NULL )
		NL_DS_DeserializeColour( n, &emitter->startColourVar );
	if ( ( n = NL_GetChildByName( root, "endColourVar" ) ) == NULL )
		NL_DS_DeserializeColour( n, &emitter->endColourVar );

	MEM_SetupReferenceInstance( "psemitter", &emitter->mem, PS_CB_DestroyEmitterTemplate, emitter );
	MEM_AddReference( &emitter->mem );

	MEM_CacheData( path, MEM_CACHE_PARTICLES, emitter );

	return emitter;
}

PSEmitter *PS_SpawnEmitter( const char *path )
{
	PSEmitter *emitterTemplate = MEM_GetCachedData( path, MEM_CACHE_PARTICLES );
	if ( emitterTemplate == NULL )
	{
		PrintWarn( "Emitter type was not cached: %s\n", path );
		return NULL;
	}

	PSEmitter *emitter = globalSystem.MAlloc( sizeof( PSEmitter ), true );
	memcpy( emitter, emitterTemplate, sizeof( PSEmitter ) );

	return emitter;
}

static void PS_TickParticle( PSParticle *particle )
{
	if ( particle->life <= 0 )
	{
		PlDestroyLinkedListNode( particle->emitter->particles, particle->node );
		globalSystem.Free( particle );
		return;
	}

	particle->oldColour = particle->colour;
	particle->colour.r += particle->deltaColour.r;
	particle->colour.g += particle->deltaColour.g;
	particle->colour.b += particle->deltaColour.b;

	particle->life--;
}

void PS_TickEmitter( PSEmitter *emitter )
{
	/* simulate all of the existing particles that we've emitted */
	PLLinkedListNode *node = PlGetFirstNode( emitter->particles );
	while ( node != NULL )
	{
		PSParticle *particle = PlGetLinkedListNodeUserData( node );
		node                 = PlGetNextLinkedListNode( node );
		PS_TickParticle( particle );
	}
}

void PS_Draw( const PLGCamera *camera )
{
}
