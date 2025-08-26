// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Render target management
// Author:  Mark E. Sowden

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "renderer_render_target.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct ApeRenderTarget
{
	char id[ 16 ];// 'rt_menu_0'

	PLGTexture      *textureAttachment;
	unsigned int     textureAttachmentComponent;
	PLGTextureFilter textureAttachmentFilter;

	PLGFrameBuffer    *frameBuffer;
	ApeMemoryReference reference;
} ApeRenderTarget;

static PLHashTable *renderTargets;

static void destroy_render_target( void *user )
{
	ApeRenderTarget *renderTarget = ( ApeRenderTarget * ) user;
	PlgDestroyTexture( renderTarget->textureAttachment );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_render_targets_( void )
{
	renderTargets = PlCreateHashTable();
	if ( renderTargets == NULL )
	{
		ape_error_( true, "Failed to create render target hash table: %s\n", PlGetError() );
	}
}

void ape_shutdown_render_targets_( void )
{
	ape_memory_flush_unreferenced_resources();

	PLHashTableNode *node = PlGetFirstHashTableNode( renderTargets );
	while ( node != NULL )
	{
		ApeRenderTarget *renderTarget = ( ApeRenderTarget * ) PlGetHashTableNodeUserData( node );

		int numReferences = ape_memory_get_num_references( &renderTarget->reference );
		if ( numReferences > 0 )
			ape_print_( "%s with %u references on shutdown!\n", renderTarget->id, numReferences );

		node = PlGetNextHashTableNode( node );
	}

	PlDestroyHashTable( renderTargets );
}

ApeRenderTarget *ape_render_target_get_by_key( const char *key )
{
	return ( ApeRenderTarget * ) PlLookupHashTableUserData( renderTargets, key, strlen( key ) );
}

ApeRenderTarget *ape_render_target_create( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter, bool useMsaa )
{
	unsigned int numSamples = useMsaa ? ape_config_.renderer.msaaSamples : 0;

	// Check if it's already been created, and if so, update size to match
	ApeRenderTarget *renderTarget = ape_render_target_get_by_key( key );
	if ( renderTarget != NULL )
	{
		if ( flags == 0 )
		{
			PRINT_DEBUG( "Placeholder render target \"%s\" was already generated, returning existing\n", key );
			ape_memory_add_reference( &renderTarget->reference );
			return renderTarget;
		}

		if ( renderTarget->frameBuffer == NULL )
		{
			renderTarget->frameBuffer = PlgCreateFrameBuffer( width, height, flags, numSamples );
			if ( renderTarget->frameBuffer == NULL )
			{
				ape_warning_( "Failed to create specified framebuffer for target \"%s\": %s\n", key, PlGetError() );
				return NULL;
			}
		}

		PRINT_DEBUG( "Render target already exists, updating size\n" );
		ape_render_target_set_size( renderTarget, width, height );
		ape_memory_add_reference( &renderTarget->reference );
		return renderTarget;
	}

	PLGFrameBuffer *frameBuffer;
	PLGTexture     *textureAttachment;
	if ( flags != 0 )
	{
		frameBuffer = PlgCreateFrameBuffer( width, height, flags, numSamples );
		if ( frameBuffer == NULL )
		{
			ape_warning_( "Failed to create specified framebuffer: %s\n", PlGetError() );
			return NULL;
		}

		textureAttachment = PlgGetFrameBufferTextureAttachment( frameBuffer, textureAttachmentComponent, textureAttachmentFilter, PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE );
		if ( textureAttachment == NULL )
		{
			ape_warning_( "Failed to create texture attachment, \"%s\":\n", key, PlGetError() );
		}
	}
	else
	{
		PRINT_DEBUG( "Creating placeholder render target, \"%s\"\n", key );
		frameBuffer       = NULL;
		textureAttachment = NULL;
	}

	renderTarget                             = QM_OS_MEMORY_NEW( ApeRenderTarget );
	renderTarget->frameBuffer                = frameBuffer;
	renderTarget->textureAttachment          = textureAttachment;
	renderTarget->textureAttachmentComponent = textureAttachmentComponent;
	renderTarget->textureAttachmentFilter    = textureAttachmentFilter;
	snprintf( renderTarget->id, sizeof( renderTarget->id ), "%s", key );

	ape_memory_setup_reference( renderTarget->id, APE_CACHE_POOL_TEXTURES, &renderTarget->reference, destroy_render_target, renderTarget );
	ape_memory_add_reference( &renderTarget->reference );

	PlInsertHashTableNode( renderTargets, key, strlen( key ), renderTarget );

	return renderTarget;
}

void ape_render_target_release( ApeRenderTarget *renderTarget )
{
	ape_memory_release( &renderTarget->reference );
}

void ape_render_target_set_size( ApeRenderTarget *renderTarget, unsigned int width, unsigned int height )
{
	if ( !PlgSetFrameBufferSize( renderTarget->frameBuffer, width, height ) )
	{
		ape_warning_( "Failed to resize framebuffer: %s\n", PlGetError() );
	}

	if ( renderTarget->textureAttachment != NULL )
	{
		PlgDestroyTexture( renderTarget->textureAttachment );
	}

	renderTarget->textureAttachment = PlgGetFrameBufferTextureAttachment(
	        renderTarget->frameBuffer,
	        renderTarget->textureAttachmentComponent,
	        renderTarget->textureAttachmentFilter,
	        PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE );
}

void ape_render_target_get_size( const ApeRenderTarget *renderTarget, unsigned int *width, unsigned int *height )
{
	*width  = renderTarget->frameBuffer->width;
	*height = renderTarget->frameBuffer->height;
}

PLGTexture *ape_render_target_get_texture( ApeRenderTarget *renderTarget )
{
	return renderTarget->textureAttachment;
}

void ape_render_target_bind( ApeRenderTarget *renderTarget, PLGFrameBufferObjectTarget target )
{
	PlgBindFrameBuffer( ( renderTarget == NULL ) ? NULL : renderTarget->frameBuffer, target );
}

PLGFrameBuffer *ape_render_target_get_frame_buffer( ApeRenderTarget *renderTarget )
{
	return renderTarget->frameBuffer;
}
