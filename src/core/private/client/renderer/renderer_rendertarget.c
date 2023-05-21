// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_hashtable.h>

#include <plgraphics/plg_framebuffer.h>

#include "core_private.h"
#include "renderer.h"

typedef struct OgeRenderTarget
{
	char            id[ 16 ];// 'rt_menu_0'
	PLGTexture     *texture;
	PLGFrameBuffer *frameBuffer;
	OgeMemoryReference reference;
} OgeRenderTarget;

static PLHashTable *renderTargets;

void ogeInitializeRenderTargets( void )
{
	renderTargets = PlCreateHashTable();
	if ( renderTargets == NULL )
	{
		PRINT_ERROR( "Failed to create render target hash table: %s\n", PlGetError() );
	}
}

void ogeShutdownRenderTargets( void )
{
	ogeMemoryManager_FlushUnreferencedResources();

	PLHashTableNode *node = PlGetFirstHashTableNode( renderTargets );
	while ( node != NULL )
	{
		OgeRenderTarget *renderTarget = ( OgeRenderTarget * ) PlGetHashTableNodeUserData( node );

		int numReferences = ogeMemoryManager_GetNumberOfReferences( &renderTarget->reference );
		if ( numReferences > 0 )
		{
			PRINT( "%s with %u references on shutdown!\n", renderTarget->id, numReferences );
		}

		node = PlGetNextHashTableNode( renderTargets, node );
	}

	PlDestroyHashTable( renderTargets );
}

OgeRenderTarget *ogeRenderTarget_GetByKey( const char *key )
{
	return ( OgeRenderTarget * ) PlLookupHashTableUserData( renderTargets, key, strlen( key ) );
}

static PLGFrameBuffer *CreateFrameBuffer( unsigned int width, unsigned int height, unsigned int flags )
{
	PLGFrameBuffer *frameBuffer = PlgCreateFrameBuffer( width, height, flags );
	if ( frameBuffer == NULL )
	{
		PRINT_WARNING( "Failed to create specified framebuffer: %s\n", PlGetError() );
		return NULL;
	}

	return frameBuffer;
}

static void DestroyRenderTarget( void *user )
{
	OgeRenderTarget *renderTarget = ( OgeRenderTarget * ) user;
	PlgDestroyTexture( renderTarget->texture );
}

OgeRenderTarget *ogeRenderTarget_Create( const char *key, unsigned int width, unsigned int height, unsigned int flags )
{
	// Check if it's already been created, and if so, update size to match
	OgeRenderTarget *renderTarget = ogeRenderTarget_GetByKey( key );
	if ( renderTarget != NULL )
	{
		if ( flags == 0 )
		{
			PRINT_DEBUG( "Placeholder render target \"%s\" was already generated, returning existing\n", key );
			ogeMemoryManager_AddReference( &renderTarget->reference );
			return renderTarget;
		}

		if ( renderTarget->frameBuffer == NULL )
		{
			renderTarget->frameBuffer = PlgCreateFrameBuffer( width, height, flags );
			if ( renderTarget->frameBuffer == NULL )
			{
				PRINT_WARNING( "Failed to create specified framebuffer for target \"%s\": %s\n", key, PlGetError() );
				return NULL;
			}
		}

		PRINT_DEBUG( "Render target already exists, updating size\n" );
		ogeRenderTarget_SetSize( renderTarget, width, height );
		ogeMemoryManager_AddReference( &renderTarget->reference );
		return renderTarget;
	}

	PLGFrameBuffer *frameBuffer;
	if ( flags != 0 )
	{
		frameBuffer = CreateFrameBuffer( width, height, flags );
		if ( frameBuffer == NULL )
		{
			PRINT_WARNING( "Failed to create render target, \"%s\"\n", key );
			return NULL;
		}
	}
	else
	{
		PRINT_DEBUG( "Creating placeholder render target, \"%s\"\n", key );
		frameBuffer = NULL;
	}

	renderTarget              = PL_NEW( OgeRenderTarget );
	renderTarget->frameBuffer = frameBuffer;
	snprintf( renderTarget->id, sizeof( renderTarget->id ), "%s", key );

	ogeMemoryManager_SetupReference( "RenderTarget", MEM_CACHE_TEXTURES, &renderTarget->reference, DestroyRenderTarget, renderTarget );
	ogeMemoryManager_AddReference( &renderTarget->reference );

	PlInsertHashTableNode( renderTargets, key, strlen( key ), renderTarget );

	return renderTarget;
}

void ogeRenderTarget_Release( OgeRenderTarget *renderTarget )
{
	ogeMemoryManager_ReleaseReference( &renderTarget->reference );
}

void ogeRenderTarget_SetSize( OgeRenderTarget *renderTarget, unsigned int width, unsigned int height )
{
	PlgSetFrameBufferSize( renderTarget->frameBuffer, width, height );
	if ( PlGetFunctionResult() != PL_RESULT_SUCCESS )
		PRINT_WARNING( "Failed to resize framebuffer: %s\n", PlGetError() );
}

PLGTexture *ogeRenderTarget_GetTextureAttachment( OgeRenderTarget *renderTarget )
{
	return renderTarget->texture;
}
