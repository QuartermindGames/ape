// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Render target management
// Author:  Mark E. Sowden

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "ar_render_target.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct ArRenderTarget
{
	char id[ 16 ];// 'rt_menu_0'

	PLGTexture *textureAttachment;
	unsigned int textureAttachmentComponent;
	PLGTextureFilter textureAttachmentFilter;

	PLGFrameBuffer *frameBuffer;
	ApeMemoryReference reference;
} ArRenderTarget;

static PLHashTable *renderTargets;

static void destroy_render_target( void *user )
{
	ArRenderTarget *renderTarget = ( ArRenderTarget * ) user;
	PlgDestroyTexture( renderTarget->textureAttachment );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ar_initialize_render_targets( void )
{
	renderTargets = PlCreateHashTable();
	if ( renderTargets == NULL )
		PRINT_ERROR( "Failed to create render target hash table: %s\n", PlGetError() );
}

void ar_shutdown_render_targets( void )
{
	apeFlushUnreferencedResources();

	PLHashTableNode *node = PlGetFirstHashTableNode( renderTargets );
	while ( node != NULL )
	{
		ArRenderTarget *renderTarget = ( ArRenderTarget * ) PlGetHashTableNodeUserData( node );

		int numReferences = apeGetNumberOfReferences( &renderTarget->reference );
		if ( numReferences > 0 )
			PRINT( "%s with %u references on shutdown!\n", renderTarget->id, numReferences );

		node = PlGetNextHashTableNode( renderTargets, node );
	}

	PlDestroyHashTable( renderTargets );
}

ArRenderTarget *ar_render_target_get_by_key( const char *key )
{
	return ( ArRenderTarget * ) PlLookupHashTableUserData( renderTargets, key, strlen( key ) );
}

ArRenderTarget *ar_render_target_create( const char *key, unsigned int width, unsigned int height, unsigned int flags,
                                         unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter )
{
	// Check if it's already been created, and if so, update size to match
	ArRenderTarget *renderTarget = ar_render_target_get_by_key( key );
	if ( renderTarget != NULL )
	{
		if ( flags == 0 )
		{
			PRINT_DEBUG( "Placeholder render target \"%s\" was already generated, returning existing\n", key );
			apeAddReference( &renderTarget->reference );
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
		ar_render_target_set_size( renderTarget, width, height );
		apeAddReference( &renderTarget->reference );
		return renderTarget;
	}

	PLGFrameBuffer *frameBuffer;
	PLGTexture *textureAttachment;
	if ( flags != 0 )
	{
		frameBuffer = PlgCreateFrameBuffer( width, height, flags );
		if ( frameBuffer == NULL )
		{
			PRINT_WARNING( "Failed to create specified framebuffer: %s\n", PlGetError() );
			return NULL;
		}

		textureAttachment = PlgGetFrameBufferTextureAttachment( frameBuffer, textureAttachmentComponent, textureAttachmentFilter );
		if ( textureAttachment == NULL )
			PRINT_WARNING( "Failed to create texture attachment, \"%s\":\n", key, PlGetError() );
	}
	else
	{
		PRINT_DEBUG( "Creating placeholder render target, \"%s\"\n", key );
		frameBuffer = NULL;
		textureAttachment = NULL;
	}

	renderTarget = PL_NEW( ArRenderTarget );
	renderTarget->frameBuffer = frameBuffer;
	renderTarget->textureAttachment = textureAttachment;
	renderTarget->textureAttachmentComponent = textureAttachmentComponent;
	renderTarget->textureAttachmentFilter = textureAttachmentFilter;
	snprintf( renderTarget->id, sizeof( renderTarget->id ), "%s", key );

	apeSetupReference( "RenderTarget", APE_CACHE_POOL_TEXTURES, &renderTarget->reference, destroy_render_target, renderTarget );
	apeAddReference( &renderTarget->reference );

	PlInsertHashTableNode( renderTargets, key, strlen( key ), renderTarget );

	return renderTarget;
}

void ar_render_target_release( ArRenderTarget *renderTarget )
{
	apeReleaseReference( &renderTarget->reference );
}

void ar_render_target_set_size( ArRenderTarget *renderTarget, unsigned int width, unsigned int height )
{
	if ( !PlgSetFrameBufferSize( renderTarget->frameBuffer, width, height ) )
		PRINT_WARNING( "Failed to resize framebuffer: %s\n", PlGetError() );

	if ( renderTarget->textureAttachment != NULL )
		PlgDestroyTexture( renderTarget->textureAttachment );

	renderTarget->textureAttachment = PlgGetFrameBufferTextureAttachment(
	        renderTarget->frameBuffer,
	        renderTarget->textureAttachmentComponent,
	        renderTarget->textureAttachmentFilter );
}

void ar_render_target_get_size( const ArRenderTarget *renderTarget, unsigned int *width, unsigned int *height )
{
	*width = renderTarget->frameBuffer->width;
	*height = renderTarget->frameBuffer->height;
}

PLGTexture *ar_render_target_get_texture( ArRenderTarget *renderTarget )
{
	return renderTarget->textureAttachment;
}

void ar_render_target_bind( ArRenderTarget *renderTarget, PLGFrameBufferObjectTarget target )
{
	PlgBindFrameBuffer( renderTarget->frameBuffer, target );
}

PLGFrameBuffer *ar_render_target_get_frame_buffer( ArRenderTarget *renderTarget )
{
	return renderTarget->frameBuffer;
}
