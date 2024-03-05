// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_linkedlist.h>

#include "ape_private.h"
#include "renderer_particle.h"
#include "renderer.h"

static void PS_CB_DestroyEmitterTemplate( void *userData )
{
	SS_Arl_ParticleEmitter *emitter = userData;
	assert( emitter != NULL );

	ss_arl_material_release( emitter->material );

	PlgDestroyMesh( emitter->mesh );

	PlFree( emitter );
}

NdBranch *PS_SerializeEmitter( const SS_Arl_ParticleEmitter *emitter )
{
	NdBranch *root = nd_branch_push_back_object( NULL, "particleEmitter" );
	if ( root != NULL )
	{
		nd_branch_push_back_int32( root, "emissionRate", emitter->emissionRate );
		nd_branch_push_back_int32( root, "emissionVar", emitter->emissionVar );

		nd_branch_push_back_int32( root, "particleLife", emitter->particleLife );
		nd_branch_push_back_int32( root, "particleLifeVar", emitter->particleLifeVar );

		nd_branch_push_back_float32( root, "speed", emitter->speed );
		nd_branch_push_back_float32( root, "speedVar", emitter->speedVar );

		nd_branch_push_back_int32( root, "maxParticles", emitter->maxParticles );
	}

	return root;
}

void ss_arl_cache_particle_emitter_template( const char *path )
{
	SS_Arl_ParticleEmitter *emitter = apeGetCachedData( path, APE_CACHE_POOL_PARTICLES );
	if ( emitter != NULL )
		return;

	NdBranch *root = nd_load_file( path, "particleEmitter" );
	if ( root == NULL )
	{
		PRINT_WARNING( "Failed to load particle emitter template: %s\n" );
		return;
	}

	emitter = PL_NEW( SS_Arl_ParticleEmitter );

	//SG_DS_Transform( root, "transform", &emitter->transform );
	//SG_DS_Transform( root, "transformVar", &emitter->transformVar );

	emitter->emissionRate = nd_branch_get_child_int( root, "emissionRate", 2 );
	emitter->emissionVar = nd_branch_get_child_int( root, "emissionVar", 2 );

	emitter->particleLife = nd_branch_get_child_int( root, "particleLife", 10 );
	emitter->particleLifeVar = nd_branch_get_child_int( root, "particleLifeVar", 5 );
	emitter->maxParticles = nd_branch_get_child_int( root, "maxParticles", 100 );

	emitter->life = nd_branch_get_child_int( root, "life", 0 );

	emitter->startColour = nd_get_colour_f32( root, "startColour", &PL_COLOURF32_WHITE );
	emitter->endColour = nd_get_colour_f32( root, "endColour", &PL_COLOURF32_WHITE );
	emitter->startColourVar = nd_get_colour_f32( root, "startColourVar", &emitter->startColourVar );
	emitter->endColourVar = nd_get_colour_f32( root, "endColourVar", &emitter->endColourVar );

	apeAddToCachePool( path, APE_CACHE_POOL_PARTICLES, emitter );

	ape_mm_setup_reference( "psemitter", APE_CACHE_POOL_PARTICLES, &emitter->mem, PS_CB_DestroyEmitterTemplate, emitter );
	ape_mm_add_reference( &emitter->mem );
}

SS_Arl_ParticleEmitter *PS_SpawnEmitterTemplateInstance( const char *path )
{
	SS_Arl_ParticleEmitter *emitterTemplate = apeGetCachedData( path, APE_CACHE_POOL_PARTICLES );
	if ( emitterTemplate == NULL )
	{
		PRINT_WARNING( "Emitter type was not cached: %s\n", path );
		return NULL;
	}

	SS_Arl_ParticleEmitter *emitter = PlMAlloc( sizeof( SS_Arl_ParticleEmitter ), true );
	memcpy( emitter, emitterTemplate, sizeof( SS_Arl_ParticleEmitter ) );

	return emitter;
}

SS_Arl_ParticleEmitter *ss_arl_particle_emitter_create( void )
{
	SS_Arl_ParticleEmitter *emitter = PL_NEW( SS_Arl_ParticleEmitter );
	emitter->particles = PlCreateLinkedList();

	emitter->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_STRIP, PLG_DRAW_DYNAMIC, 1000, 1000 );
	if ( emitter->mesh == NULL )
		PRINT_ERROR( "Failed to create emitter mesh!\nPL: %s\n", PlGetError() );

	emitter->startScale = 10.0f;
	emitter->endScale = 0.0f;

	return emitter;
}

void ss_arl_particle_emitter_destroy( SS_Arl_ParticleEmitter *emitter )
{
	/* todo: 	push it into a queue to be removed once
	 * 			all the particles are dead */
	if ( emitter == NULL )
		return;

	/* free all the particles we've created */
	PLLinkedListNode *node = PlGetFirstNode( emitter->particles );
	while ( node != NULL )
	{
		SS_Arl_Particle *particle = PlGetLinkedListNodeUserData( node );
		node = PlGetNextLinkedListNode( node );
		PlFree( particle );
	}

	if ( emitter->material != NULL )
		ss_arl_material_release( emitter->material );

	PlDestroyLinkedList( emitter->particles );
	PlFree( emitter );
}

static int rand_int( int max )
{
	return ( rand() % max );
}

static void tick_particle( SS_Arl_Particle *particle, SS_Arl_ParticleEmitter *emitter )
{
	if ( particle->life <= 0 )
	{
		PlDestroyLinkedListNode( particle->node );
		PlFree( particle );
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
	particle->colour = PlAddColourF32( &particle->colour, &particle->deltaColour );

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

void ss_arl_particle_emitter_tick( SS_Arl_ParticleEmitter *emitter )
{
	int numParticles = ( int ) PlGetNumLinkedListNodes( emitter->particles );
	if ( numParticles < emitter->maxParticles && emitter->numTicks > emitter->maxTicks )
	{
		SS_Arl_Particle *particle = PlMAlloc( sizeof( SS_Arl_Particle ), true );
		particle->emitter = emitter;

		PLVector3 translationMod;
		translationMod.x = emitter->transform.translation.x + ( PlGenerateRandomFloat( emitter->transformVar.translation.x ) + PlGenerateRandomFloat( -emitter->transformVar.translation.x ) );
		translationMod.y = emitter->transform.translation.y + ( PlGenerateRandomFloat( emitter->transformVar.translation.y ) + PlGenerateRandomFloat( -emitter->transformVar.translation.y ) );
		translationMod.z = emitter->transform.translation.z + ( PlGenerateRandomFloat( emitter->transformVar.translation.z ) + PlGenerateRandomFloat( -emitter->transformVar.translation.z ) );
		particle->transform.translation = translationMod;

		particle->life = emitter->particleLife + ( emitter->particleLifeVar * rand_int( 100 ) );

		PLColourF32 startColour, endColour;
		startColour.r = emitter->startColour.r + ( emitter->startColourVar.r * PlGenerateRandomFloat( 1.0f ) );
		startColour.g = emitter->startColour.g + ( emitter->startColourVar.g * PlGenerateRandomFloat( 1.0f ) );
		startColour.b = emitter->startColour.b + ( emitter->startColourVar.b * PlGenerateRandomFloat( 1.0f ) );
		startColour.a = emitter->startColour.a + ( emitter->startColourVar.a * PlGenerateRandomFloat( 1.0f ) );
		endColour.r = emitter->endColour.r + ( emitter->endColourVar.r * PlGenerateRandomFloat( 1.0f ) );
		endColour.g = emitter->endColour.g + ( emitter->endColourVar.g * PlGenerateRandomFloat( 1.0f ) );
		endColour.b = emitter->endColour.b + ( emitter->endColourVar.b * PlGenerateRandomFloat( 1.0f ) );
		endColour.a = emitter->endColour.a + ( emitter->endColourVar.a * PlGenerateRandomFloat( 1.0f ) );

		particle->colour = startColour;
		particle->deltaColour.r = ( ( endColour.r - startColour.r ) / 1.0f ) / ( float ) particle->life;
		particle->deltaColour.g = ( ( endColour.g - startColour.g ) / 1.0f ) / ( float ) particle->life;
		particle->deltaColour.b = ( ( endColour.b - startColour.b ) / 1.0f ) / ( float ) particle->life;
		particle->deltaColour.a = ( ( endColour.a - startColour.a ) / 1.0f ) / ( float ) particle->life;

		float startScale = emitter->startScale + ( emitter->scaleVar * PlGenerateRandomFloat( 1.0f ) );
		float endScale = emitter->endScale + ( emitter->scaleVar * PlGenerateRandomFloat( 1.0f ) );
		particle->deltaScale = ( ( endScale - startScale ) / 1.0f ) / ( float ) particle->life;

		particle->bounds.maxs = PLVector3( 2.0f, 2.0f, 2.0f );
		particle->bounds.mins = PLVector3( -2.0f, -2.0f, -2.0f );

		particle->node = PlInsertLinkedListNode( emitter->particles, particle );

		emitter->numTicks = 0;
		emitter->maxTicks = emitter->emissionRate + ( emitter->emissionVar * rand_int( 100 ) );
	}

	/* simulate all of the existing particles that we've emitted */
	unsigned int i = 0;
	PLLinkedListNode *node = PlGetFirstNode( emitter->particles );
	while ( node != NULL )
	{
		SS_Arl_Particle *particle = PlGetLinkedListNodeUserData( node );
		if ( i == 0 )
		{
			emitter->bounds.maxs = ( PLVector3 ){ particle->transform.translation.x, particle->transform.translation.y, particle->transform.translation.z };
			emitter->bounds.mins = ( PLVector3 ){ particle->transform.translation.x, particle->transform.translation.y, particle->transform.translation.z };
		}

		node = PlGetNextLinkedListNode( node );
		tick_particle( particle, emitter );

		++i;
	}

	emitter->bounds.absOrigin = PLVector3( ( emitter->bounds.mins.x + emitter->bounds.maxs.x ) / 2, ( emitter->bounds.mins.y + emitter->bounds.maxs.y ) / 2, ( emitter->bounds.mins.z + emitter->bounds.maxs.z ) / 2 );
	emitter->bounds.origin = emitter->transform.translation;

	emitter->numTicks++;
}

void ss_arl_particle_emitter_draw( const SS_Arl_ParticleEmitter *emitter, const ApeCamera *camera )
{
	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_ALPHA ] );

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
		SS_Arl_Particle *particle = PlGetLinkedListNodeUserData( node );

		//PlgDrawBoundingVolume( &particle->bounds, PlColourF32ToU8( &particle->colour ) );

		float x = particle->transform.translation.x;
		float y = particle->transform.translation.y;
		float z = particle->transform.translation.z;

		PLColour colour = PlColourF32ToU8( &particle->colour );

		unsigned int a = PlgAddMeshVertex( emitter->mesh, &PLVector3( x - particle->scale, y - particle->scale, z - particle->scale ), &pl_vecOrigin3, &colour, &PLVector2( 0.0f, 0.0f ) );
		unsigned int b = PlgAddMeshVertex( emitter->mesh, &PLVector3( x - particle->scale, y - particle->scale, z + particle->scale ), &pl_vecOrigin3, &colour, &PLVector2( 0.0f, 1.0f ) );
		//unsigned int c = PlgAddMeshVertex( emitter->mesh, PLVector3( x + particle->scale, y - particle->scale, z - particle->scale ), pl_vecOrigin3, colour, PLVector2( 1.0f, 0.0f ) );
		//unsigned int d = PlgAddMeshVertex( emitter->mesh, PLVector3( x + particle->scale, y - particle->scale, z + particle->scale ), pl_vecOrigin3, colour, PLVector2( 1.0f, 1.0f ) );

		//PlgAddMeshTriangle( emitter->mesh, a, b, c );
		//PlgAddMeshTriangle( emitter->mesh, c, b, d );

		node = PlGetNextLinkedListNode( node );
	}

	ss_arl_material_draw( emitter->material, emitter->mesh, NULL, 0 );

	PlgSetCullMode( PLG_CULL_POSITIVE );

	PlPopMatrix();
}
