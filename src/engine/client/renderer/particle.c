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

	RM_ReleaseMaterial( emitter->material );

	PlgDestroyMesh( emitter->mesh );

	globalSystem.Free( emitter );
}

NLNode *PS_SerializeEmitter( const PSEmitter *emitter )
{
	NLNode *root = NL_PushBackObj( NULL, "particleEmitter" );
	if ( root != NULL )
	{
		NL_PushBackI32( root, "emissionRate", emitter->emissionRate );
		NL_PushBackI32( root, "emissionVar", emitter->emissionVar );

		NL_PushBackI32( root, "particleLife", emitter->particleLife );
		NL_PushBackI32( root, "particleLifeVar", emitter->particleLifeVar );

		NL_PushBackF32( root, "speed", emitter->speed );
		NL_PushBackF32( root, "speedVar", emitter->speedVar );

		NL_PushBackI32( root, "maxParticles", emitter->maxParticles );
	}

	return root;
}

void PS_CacheEmitterTemplate( const char *path )
{
	PSEmitter *emitter = MEM_GetCachedData( path, MEM_CACHE_PARTICLES );
	if ( emitter != NULL )
		return;

	NLNode *root = NL_LoadFile( path, "particleEmitter" );
	if ( root == NULL )
	{
		PrintWarn( "Failed to load particle emitter template: %s\n" );
		return;
	}

	emitter = globalSystem.MAlloc( sizeof( PSEmitter ), true );

	SG_DS_Transform( root, "transform", &emitter->transform );
	SG_DS_Transform( root, "transformVar", &emitter->transformVar );

	emitter->emissionRate = NL_GetI32ByName( root, "emissionRate", 2 );
	emitter->emissionVar  = NL_GetI32ByName( root, "emissionVar", 2 );

	emitter->particleLife	 = NL_GetI32ByName( root, "particleLife", 10 );
	emitter->particleLifeVar = NL_GetI32ByName( root, "particleLifeVar", 5 );
	emitter->maxParticles	 = NL_GetI32ByName( root, "maxParticles", 100 );

	emitter->life = NL_GetI32ByName( root, "life", 0 );

	NLNode *n;
	if ( ( n = NL_GetChildByName( root, "startColour" ) ) == NULL )
		NL_DS_DeserializeColourF32( n, &emitter->startColour );
	if ( ( n = NL_GetChildByName( root, "endColour" ) ) == NULL )
		NL_DS_DeserializeColourF32( n, &emitter->endColour );
	if ( ( n = NL_GetChildByName( root, "startColourVar" ) ) == NULL )
		NL_DS_DeserializeColourF32( n, &emitter->startColourVar );
	if ( ( n = NL_GetChildByName( root, "endColourVar" ) ) == NULL )
		NL_DS_DeserializeColourF32( n, &emitter->endColourVar );

	MEM_SetupReferenceInstance( "psemitter", &emitter->mem, PS_CB_DestroyEmitterTemplate, emitter );
	MEM_AddReference( &emitter->mem );

	MEM_CacheData( path, MEM_CACHE_PARTICLES, emitter );
}

PSEmitter *PS_SpawnEmitterTemplateInstance( const char *path )
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

PSEmitter *PS_SpawnEmitter( void )
{
	PSEmitter *emitter = globalSystem.MAlloc( sizeof( PSEmitter ), true );
	emitter->particles = PlCreateLinkedList();

	emitter->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_STRIP, PLG_DRAW_DYNAMIC, 1000, 1000 );
	if ( emitter->mesh == NULL )
		PrintError( "Failed to create emitter mesh!\nPL: %s\n", PlGetError() );

	emitter->startScale = 10.0f;
	emitter->endScale	= 0.0f;

	return emitter;
}

void PS_DestroyEmitter( PSEmitter *emitter )
{
	/* todo: 	push it into a queue to be removed once
	 * 			all the particles are dead */
	if ( emitter == NULL )
		return;

	/* free all the particles we've created */
	PLLinkedListNode *node = PlGetFirstNode( emitter->particles );
	while ( node != NULL )
	{
		PSParticle *particle = PlGetLinkedListNodeUserData( node );
		node				 = PlGetNextLinkedListNode( node );
		globalSystem.Free( particle );
	}

	if ( emitter->material != NULL )
		RM_ReleaseMaterial( emitter->material );

	PlDestroyLinkedList( emitter->particles );
	globalSystem.Free( emitter );
}

int U_Rand_I32( int max )
{
	return ( rand() % max );
}

static void PS_TickParticle( PSParticle *particle, PSEmitter *emitter )
{
	if ( particle->life <= 0 )
	{
		PlDestroyLinkedListNode( particle->emitter->particles, particle->node );
		globalSystem.Free( particle );
		return;
	}

	particle->oldTransform = particle->transform;

	PLVector3 force;
	force.x = emitter->force.x + ( PlGenerateRandomFloat( emitter->forceVar.x ) );
	force.y = emitter->force.y + ( PlGenerateRandomFloat( emitter->forceVar.y ) );
	force.z = emitter->force.z + ( PlGenerateRandomFloat( emitter->forceVar.z ) );

	particle->transform.translation = PlAddVector3( particle->transform.translation, force );

	particle->bounds.origin = particle->transform.translation;

	particle->oldColour = particle->colour;
	particle->colour	= PlAddColourF32( &particle->colour, &particle->deltaColour );

	particle->scale += particle->deltaScale;

	/* keep the emitter bounds updated */
	if ( emitter->bounds.maxs.x < particle->transform.translation.x ) emitter->bounds.maxs.x = particle->transform.translation.x;
	if ( emitter->bounds.maxs.y < particle->transform.translation.y ) emitter->bounds.maxs.y = particle->transform.translation.y;
	if ( emitter->bounds.maxs.z < particle->transform.translation.z ) emitter->bounds.maxs.z = particle->transform.translation.z;
	if ( emitter->bounds.mins.x > particle->transform.translation.x ) emitter->bounds.mins.x = particle->transform.translation.x;
	if ( emitter->bounds.mins.y > particle->transform.translation.y ) emitter->bounds.mins.y = particle->transform.translation.y;
	if ( emitter->bounds.mins.z > particle->transform.translation.z ) emitter->bounds.mins.z = particle->transform.translation.z;

	particle->life--;
}

void PS_TickEmitter( PSEmitter *emitter )
{
	int numParticles = ( int ) PlGetNumLinkedListNodes( emitter->particles );
	if ( numParticles < emitter->maxParticles && emitter->numTicks > emitter->maxTicks )
	{
		PSParticle *particle = globalSystem.MAlloc( sizeof( PSParticle ), true );
		particle->emitter	 = emitter;

		PLVector3 translationMod;
		translationMod.x				= emitter->transform.translation.x + ( PlGenerateRandomFloat( emitter->transformVar.translation.x ) + PlGenerateRandomFloat( -emitter->transformVar.translation.x ) );
		translationMod.y				= emitter->transform.translation.y + ( PlGenerateRandomFloat( emitter->transformVar.translation.y ) + PlGenerateRandomFloat( -emitter->transformVar.translation.y ) );
		translationMod.z				= emitter->transform.translation.z + ( PlGenerateRandomFloat( emitter->transformVar.translation.z ) + PlGenerateRandomFloat( -emitter->transformVar.translation.z ) );
		particle->transform.translation = translationMod;

		particle->life = emitter->particleLife + ( emitter->particleLifeVar * U_Rand_I32( 100 ) );

		PLColourF32 startColour, endColour;
		startColour.r = emitter->startColour.r + ( emitter->startColourVar.r * PlGenerateRandomFloat( 1.0f ) );
		startColour.g = emitter->startColour.g + ( emitter->startColourVar.g * PlGenerateRandomFloat( 1.0f ) );
		startColour.b = emitter->startColour.b + ( emitter->startColourVar.b * PlGenerateRandomFloat( 1.0f ) );
		startColour.a = emitter->startColour.a + ( emitter->startColourVar.a * PlGenerateRandomFloat( 1.0f ) );
		endColour.r	  = emitter->endColour.r + ( emitter->endColourVar.r * PlGenerateRandomFloat( 1.0f ) );
		endColour.g	  = emitter->endColour.g + ( emitter->endColourVar.g * PlGenerateRandomFloat( 1.0f ) );
		endColour.b	  = emitter->endColour.b + ( emitter->endColourVar.b * PlGenerateRandomFloat( 1.0f ) );
		endColour.a	  = emitter->endColour.a + ( emitter->endColourVar.a * PlGenerateRandomFloat( 1.0f ) );

		particle->colour		= startColour;
		particle->deltaColour.r = ( ( endColour.r - startColour.r ) / 1.0f ) / ( float ) particle->life;
		particle->deltaColour.g = ( ( endColour.g - startColour.g ) / 1.0f ) / ( float ) particle->life;
		particle->deltaColour.b = ( ( endColour.b - startColour.b ) / 1.0f ) / ( float ) particle->life;
		particle->deltaColour.a = ( ( endColour.a - startColour.a ) / 1.0f ) / ( float ) particle->life;

		float startScale	 = emitter->startScale + ( emitter->scaleVar * PlGenerateRandomFloat( 1.0f ) );
		float endScale		 = emitter->endScale + ( emitter->scaleVar * PlGenerateRandomFloat( 1.0f ) );
		particle->deltaScale = ( ( endScale - startScale ) / 1.0f ) / ( float ) particle->life;

		particle->bounds.maxs = PLVector3( 2.0f, 2.0f, 2.0f );
		particle->bounds.mins = PLVector3( -2.0f, -2.0f, -2.0f );

		particle->node = PlInsertLinkedListNode( emitter->particles, particle );

		emitter->numTicks = 0;
		emitter->maxTicks = emitter->emissionRate + ( emitter->emissionVar * U_Rand_I32( 100 ) );
	}

	/* simulate all of the existing particles that we've emitted */
	unsigned int	  i	   = 0;
	PLLinkedListNode *node = PlGetFirstNode( emitter->particles );
	while ( node != NULL )
	{
		PSParticle *particle = PlGetLinkedListNodeUserData( node );
		if ( i == 0 )
		{
			emitter->bounds.maxs = ( PLVector3 ){ particle->transform.translation.x, particle->transform.translation.y, particle->transform.translation.z };
			emitter->bounds.mins = ( PLVector3 ){ particle->transform.translation.x, particle->transform.translation.y, particle->transform.translation.z };
		}

		node = PlGetNextLinkedListNode( node );
		PS_TickParticle( particle, emitter );

		++i;
	}

	emitter->bounds.absOrigin = PLVector3( ( emitter->bounds.mins.x + emitter->bounds.maxs.x ) / 2, ( emitter->bounds.mins.y + emitter->bounds.maxs.y ) / 2, ( emitter->bounds.mins.z + emitter->bounds.maxs.z ) / 2 );
	emitter->bounds.origin	  = emitter->transform.translation;

	emitter->numTicks++;
}

void PS_Draw( const PSEmitter *emitter, const Camera *camera )
{
	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_ALPHA ] );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	//R_DrawAxesPivot( emitter->transform.translation, PlQuaternionToEuler( &emitter->transform.rotation ) );
	//PlgDrawBoundingVolume( &emitter->bounds, PLColour( 255, 0, 255, 255 ) );

	PlgClearMesh( emitter->mesh );

	PlgSetCullMode( PLG_CULL_NONE );

	PLLinkedListNode *node = PlGetFirstNode( emitter->particles );
	while ( node != NULL )
	{
		PSParticle *particle = PlGetLinkedListNodeUserData( node );

		//PlgDrawBoundingVolume( &particle->bounds, PlColourF32ToU8( &particle->colour ) );

		float x = particle->transform.translation.x;
		float y = particle->transform.translation.y;
		float z = particle->transform.translation.z;

		PLColour colour = PlColourF32ToU8( &particle->colour );

		unsigned int a = PlgAddMeshVertex( emitter->mesh, PLVector3( x - particle->scale, y - particle->scale, z - particle->scale ), pl_vecOrigin3, colour, PLVector2( 0.0f, 0.0f ) );
		unsigned int b = PlgAddMeshVertex( emitter->mesh, PLVector3( x - particle->scale, y - particle->scale, z + particle->scale ), pl_vecOrigin3, colour, PLVector2( 0.0f, 1.0f ) );
		//unsigned int c = PlgAddMeshVertex( emitter->mesh, PLVector3( x + particle->scale, y - particle->scale, z - particle->scale ), pl_vecOrigin3, colour, PLVector2( 1.0f, 0.0f ) );
		//unsigned int d = PlgAddMeshVertex( emitter->mesh, PLVector3( x + particle->scale, y - particle->scale, z + particle->scale ), pl_vecOrigin3, colour, PLVector2( 1.0f, 1.0f ) );

		//PlgAddMeshTriangle( emitter->mesh, a, b, c );
		//PlgAddMeshTriangle( emitter->mesh, c, b, d );

		node = PlGetNextLinkedListNode( node );
	}

	RM_DrawMesh( emitter->material, emitter->mesh );

	PlgSetCullMode( PLG_CULL_POSTIVE );

	PlPopMatrix();
}
