// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <plcore/pl_linkedlist.h>

#include "qmos/public/qm_os_random.h"

#include "ape_private.h"
#include "renderer_particle.h"
#include "material/material.h"

static void DestroyEmitterTemplateCallback( void *userData )
{
	ApeParticleEmitter *emitter = userData;
	assert( emitter != NULL );

	ape_material_release( emitter->material );

	PlgDestroyMesh( emitter->mesh );

	qm_os_memory_free( emitter );
}

AcmBranch *PS_SerializeEmitter( const ApeParticleEmitter *emitter )
{
	AcmBranch *root = acm_push_object( nullptr, "particleEmitter" );
	if ( root != NULL )
	{
		acm_push_i32( root, "emissionRate", emitter->emissionRate );
		acm_push_i32( root, "emissionVar", emitter->emissionVar );

		acm_push_i32( root, "particleLife", emitter->particleLife );
		acm_push_i32( root, "particleLifeVar", emitter->particleLifeVar );

		acm_push_f32( root, "speed", emitter->speed );
		acm_push_f32( root, "speedVar", emitter->speedVar );

		acm_push_i32( root, "maxParticles", emitter->maxParticles );
	}

	return root;
}

void ss_arl_cache_particle_emitter_template( const char *path )
{
	ApeParticleEmitter *emitter = ape_memory_get_from_pool_( path, APE_CACHE_POOL_PARTICLES );
	if ( emitter != NULL )
		return;

	AcmBranch *root = com_acm_load_file( path, "particleEmitter" );
	if ( root == NULL )
	{
		ape_console_warning_( "Failed to load particle emitter template: %s\n" );
		return;
	}

	emitter = QM_OS_MEMORY_NEW( ApeParticleEmitter );

	//SG_DS_Transform( root, "transform", &emitter->transform );
	//SG_DS_Transform( root, "transformVar", &emitter->transformVar );

	emitter->emissionRate = acm_get_int( root, "emissionRate", 2 );
	emitter->emissionVar  = acm_get_int( root, "emissionVar", 2 );

	emitter->particleLife    = acm_get_int( root, "particleLife", 10 );
	emitter->particleLifeVar = acm_get_int( root, "particleLifeVar", 5 );
	emitter->maxParticles    = acm_get_int( root, "maxParticles", 100 );

	emitter->life = acm_get_int( root, "life", 0 );

	emitter->startColour    = com_acm_get_colour_f32( root, "startColour", &PL_COLOURF32_WHITE );
	emitter->endColour      = com_acm_get_colour_f32( root, "endColour", &PL_COLOURF32_WHITE );
	emitter->startColourVar = com_acm_get_colour_f32( root, "startColourVar", &emitter->startColourVar );
	emitter->endColourVar   = com_acm_get_colour_f32( root, "endColourVar", &emitter->endColourVar );

	ape_memory_setup_reference( path, APE_CACHE_POOL_PARTICLES, &emitter->mem, DestroyEmitterTemplateCallback, emitter );
	ape_memory_add_reference( &emitter->mem );
}

ApeParticleEmitter *PS_SpawnEmitterTemplateInstance( const char *path )
{
	ApeParticleEmitter *emitterTemplate = ape_memory_get_from_pool_( path, APE_CACHE_POOL_PARTICLES );
	if ( emitterTemplate == NULL )
	{
		ape_console_warning_( "Emitter type was not cached: %s\n", path );
		return nullptr;
	}

	ApeParticleEmitter *emitter = QM_OS_MEMORY_MALLOC_( sizeof( ApeParticleEmitter ) );
	memcpy( emitter, emitterTemplate, sizeof( ApeParticleEmitter ) );

	return emitter;
}

ApeParticleEmitter *ss_arl_particle_emitter_create( void )
{
	ApeParticleEmitter *emitter = QM_OS_MEMORY_NEW( ApeParticleEmitter );
	emitter->particles          = PlCreateLinkedList();

	emitter->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_STRIP, PLG_DRAW_DYNAMIC, 1000, 1000 );
	if ( emitter->mesh == NULL )
		ape_console_error_( true, "Failed to create emitter mesh!\nPL: %s\n", PlGetError() );

	emitter->startScale = 10.0f;
	emitter->endScale   = 0.0f;

	emitter->seed = qm_os_random_seed_initialize();

	return emitter;
}

void ss_arl_particle_emitter_destroy( ApeParticleEmitter *emitter )
{
	/* todo: 	push it into a queue to be removed once
	 * 			all the particles are dead */
	if ( emitter == NULL )
		return;

	/* free all the particles we've created */
	PLLinkedListNode *node = PlGetFirstNode( emitter->particles );
	while ( node != NULL )
	{
		ApeParticle *particle = PlGetLinkedListNodeUserData( node );
		node                  = PlGetNextLinkedListNode( node );
		qm_os_memory_free( particle );
	}

	if ( emitter->material != NULL )
		ape_material_release( emitter->material );

	PlDestroyLinkedList( emitter->particles );
	qm_os_memory_free( emitter );
}

static void tick_particle( ApeParticle *particle, ApeParticleEmitter *emitter )
{
	if ( particle->life <= 0 )
	{
		PlDestroyLinkedListNode( particle->node );
		qm_os_memory_free( particle );
		return;
	}

	particle->oldTransform = particle->transform;

	QmMathVector3f force;
	force.x = emitter->force.x + qm_os_random_float( &emitter->seed, emitter->forceVar.x );
	force.y = emitter->force.y + qm_os_random_float( &emitter->seed, emitter->forceVar.y );
	force.z = emitter->force.z + qm_os_random_float( &emitter->seed, emitter->forceVar.z );

	particle->transform.translation = qm_math_vector3f_add( particle->transform.translation, force );

	particle->bounds.origin = particle->transform.translation;

	particle->oldColour = particle->colour;
	particle->colour    = qm_math_colour4f_add( particle->colour, particle->deltaColour );

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

void ss_arl_particle_emitter_tick( ApeParticleEmitter *emitter )
{
	int numParticles = PlGetNumLinkedListNodes( emitter->particles );
	if ( numParticles < emitter->maxParticles && emitter->numTicks > emitter->maxTicks )
	{
		ApeParticle *particle = QM_OS_MEMORY_NEW( ApeParticle );

		particle->emitter = emitter;

		QmMathVector3f translationMod;
		translationMod.x                = emitter->transform.translation.x + ( qm_os_random_float( &emitter->seed, emitter->transformVar.translation.x ) + qm_os_random_float( &emitter->seed, -emitter->transformVar.translation.x ) );
		translationMod.y                = emitter->transform.translation.y + ( qm_os_random_float( &emitter->seed, emitter->transformVar.translation.y ) + qm_os_random_float( &emitter->seed, -emitter->transformVar.translation.y ) );
		translationMod.z                = emitter->transform.translation.z + ( qm_os_random_float( &emitter->seed, emitter->transformVar.translation.z ) + qm_os_random_float( &emitter->seed, -emitter->transformVar.translation.z ) );
		particle->transform.translation = translationMod;

		particle->life = emitter->particleLife + emitter->particleLifeVar * qm_os_random_int( &emitter->seed ) % 100;

		QmMathColour4f startColour, endColour;
		startColour.r = emitter->startColour.r + emitter->startColourVar.r * qm_os_random_float( &emitter->seed, 1.0f );
		startColour.g = emitter->startColour.g + emitter->startColourVar.g * qm_os_random_float( &emitter->seed, 1.0f );
		startColour.b = emitter->startColour.b + emitter->startColourVar.b * qm_os_random_float( &emitter->seed, 1.0f );
		startColour.a = emitter->startColour.a + emitter->startColourVar.a * qm_os_random_float( &emitter->seed, 1.0f );
		endColour.r   = emitter->endColour.r + emitter->endColourVar.r * qm_os_random_float( &emitter->seed, 1.0f );
		endColour.g   = emitter->endColour.g + emitter->endColourVar.g * qm_os_random_float( &emitter->seed, 1.0f );
		endColour.b   = emitter->endColour.b + emitter->endColourVar.b * qm_os_random_float( &emitter->seed, 1.0f );
		endColour.a   = emitter->endColour.a + emitter->endColourVar.a * qm_os_random_float( &emitter->seed, 1.0f );

		particle->colour        = startColour;
		particle->deltaColour.r = ( ( endColour.r - startColour.r ) / 1.0f ) / ( float ) particle->life;
		particle->deltaColour.g = ( ( endColour.g - startColour.g ) / 1.0f ) / ( float ) particle->life;
		particle->deltaColour.b = ( ( endColour.b - startColour.b ) / 1.0f ) / ( float ) particle->life;
		particle->deltaColour.a = ( ( endColour.a - startColour.a ) / 1.0f ) / ( float ) particle->life;

		float startScale     = emitter->startScale + emitter->scaleVar * qm_os_random_float( &emitter->seed, 1.0f );
		float endScale       = emitter->endScale + emitter->scaleVar * qm_os_random_float( &emitter->seed, 1.0f );
		particle->deltaScale = ( ( endScale - startScale ) / 1.0f ) / ( float ) particle->life;

		particle->bounds.maxs = qm_math_vector3f( 2.0f, 2.0f, 2.0f );
		particle->bounds.mins = qm_math_vector3f( -2.0f, -2.0f, -2.0f );

		particle->node = PlInsertLinkedListNode( emitter->particles, particle );

		emitter->numTicks = 0;
		emitter->maxTicks = emitter->emissionRate + emitter->emissionVar * qm_os_random_int( &emitter->seed ) % 100;
	}

	/* simulate all of the existing particles that we've emitted */
	unsigned int      i    = 0;
	PLLinkedListNode *node = PlGetFirstNode( emitter->particles );
	while ( node != NULL )
	{
		ApeParticle *particle = PlGetLinkedListNodeUserData( node );
		if ( i == 0 )
		{
			emitter->bounds.maxs = qm_math_vector3f( particle->transform.translation.x, particle->transform.translation.y, particle->transform.translation.z );
			emitter->bounds.mins = qm_math_vector3f( particle->transform.translation.x, particle->transform.translation.y, particle->transform.translation.z );
		}

		node = PlGetNextLinkedListNode( node );
		tick_particle( particle, emitter );

		++i;
	}

	emitter->bounds.absOrigin = qm_math_vector3f( ( emitter->bounds.mins.x + emitter->bounds.maxs.x ) / 2, ( emitter->bounds.mins.y + emitter->bounds.maxs.y ) / 2, ( emitter->bounds.mins.z + emitter->bounds.maxs.z ) / 2 );
	emitter->bounds.origin    = emitter->transform.translation;

	emitter->numTicks++;
}

void ss_arl_particle_emitter_draw( const ApeParticleEmitter *emitter, const ApeCamera *camera )
{
	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_ALPHA );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	//R_DrawAxesPivot( emitter->transform.translation, PlQuaternionToEuler( &emitter->transform.rotation ) );
	//PlgDrawBoundingVolume( &emitter->bounds, QmMathColour4ub( 255, 0, 255, 255 ) );

	PlgClearMesh( emitter->mesh );

	PlgSetCullMode( PLG_CULL_NONE );

	PLLinkedListNode *node = PlGetFirstNode( emitter->particles );
	while ( node != NULL )
	{
		ApeParticle *particle = PlGetLinkedListNodeUserData( node );

		//PlgDrawBoundingVolume( &particle->bounds, PlColourF32ToU8( &particle->colour ) );

		float x = particle->transform.translation.x;
		float y = particle->transform.translation.y;
		float z = particle->transform.translation.z;

		QmMathColour4ub colour = QM_MATH_COLOUR4F_TO_4UB( particle->colour );

		unsigned int a = PlgAddMeshVertex( emitter->mesh, &QM_MATH_VECTOR3F( x - particle->scale, y - particle->scale, z - particle->scale ), &QM_MATH_VECTOR3F_ZERO, &colour, &QM_MATH_VECTOR2F( 0.0f, 0.0f ) );
		unsigned int b = PlgAddMeshVertex( emitter->mesh, &QM_MATH_VECTOR3F( x - particle->scale, y - particle->scale, z + particle->scale ), &QM_MATH_VECTOR3F_ZERO, &colour, &QM_MATH_VECTOR2F( 0.0f, 1.0f ) );
		//unsigned int c = PlgAddMeshVertex( emitter->mesh, QmMathVector3f( x + particle->scale, y - particle->scale, z - particle->scale ), pl_vecOrigin3, colour, QmMathVector2f( 1.0f, 0.0f ) );
		//unsigned int d = PlgAddMeshVertex( emitter->mesh, QmMathVector3f( x + particle->scale, y - particle->scale, z + particle->scale ), pl_vecOrigin3, colour, QmMathVector2f( 1.0f, 1.0f ) );

		//PlgAddMeshTriangle( emitter->mesh, a, b, c );
		//PlgAddMeshTriangle( emitter->mesh, c, b, d );

		node = PlGetNextLinkedListNode( node );
	}

	ape_material_draw( emitter->material, emitter->mesh, NULL );

	PlgSetCullMode( PLG_CULL_POSITIVE );

	PlPopMatrix();
}
