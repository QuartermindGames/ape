/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"
#include "model.h"

/**
 * Callback for garbage day.
 */
static void MDL_CB_Destroy( void *userData )
{
	PLMModel *model = userData;
	u_assert( model != NULL );
	if ( model == NULL )
		return;

	ModelUserData *additionalData = model->userData;
	if ( additionalData != NULL )
	{
		for ( unsigned int i = 0; i < additionalData->numMaterials; ++i )
			RM_ReleaseMaterial( additionalData->materials[ i ] );
	}

	PlmDestroyModel( model );
}

/**
 * Setup the additional data we need.
 */
static void MDL_SetupUserData( PLMModel *model )
{
	ModelUserData *userData = globalSystem.MAlloc( sizeof( ModelUserData ), true );
	userData->numMaterials  = model->numMaterials;
	if ( userData->numMaterials > MODEL_MAX_MATERIALS )
	{
		PrintWarn( "Invalid number of materials: " COM_FMT_uint32 " vs " COM_FMT_uint32 "\n",
		           userData->numMaterials, MODEL_MAX_MATERIALS );
		userData->numMaterials = MODEL_MAX_MATERIALS;
	}

	for ( unsigned int i = 0; i < userData->numMaterials; ++i )
		userData->materials[ i ] = RM_CacheMaterial( model->materials[ i ], CACHE_GROUP_WORLD, true );

	Mem_SetupReferenceInstance( "model", &userData->mem, MDL_CB_Destroy, model );
}

PLMModel *MDL_CacheModel( const char *path )
{
	return NULL;
}

/**
 * Release a model handle.
 * If it's not tracked by the memory
 * manager then it'll be immediately
 * destroyed.
 */
void MDL_Release( PLMModel *model )
{
	ModelUserData *additionalData = model->userData;
	if ( additionalData == NULL )
	{
		PrintWarn( "Destroying model not tracked by memory manager!\n" );
		PlmDestroyModel( model );
		return;
	}

	Mem_ReleaseReference( &additionalData->mem );
}
