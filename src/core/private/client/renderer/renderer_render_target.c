// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Render target management
// Author:  Mark E. Sowden

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "renderer_render_target.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct SSArlRenderTarget
{
	char id[ 16 ];// 'rt_menu_0'

	PLGTexture *textureAttachment;
	unsigned int textureAttachmentComponent;
	PLGTextureFilter textureAttachmentFilter;

	PLGFrameBuffer *frameBuffer;
	ApeMemoryReference reference;
} SSArlRenderTarget;

static PLHashTable *renderTargets;

static void destroy_render_target( void *user )
{
	SSArlRenderTarget *renderTarget = ( SSArlRenderTarget * ) user;
	PlgDestroyTexture( renderTarget->textureAttachment );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ss_arl_initialize_render_targets_( void )
{
	renderTargets = PlCreateHashTable();
	if ( renderTargets == NULL )
		PRINT_ERROR( "Failed to create render target hash table: %s\n", PlGetError() );
}

void ss_arl_shutdown_render_targets_( void )
{
	apeFlushUnreferencedResources();

	PLHashTableNode *node = PlGetFirstHashTableNode( renderTargets );
	while ( node != NULL )
	{
		SSArlRenderTarget *renderTarget = ( SSArlRenderTarget * ) PlGetHashTableNodeUserData( node );

		int numReferences = apeGetNumberOfReferences( &renderTarget->reference );
		if ( numReferences > 0 )
			PRINT( "%s with %u references on shutdown!\n", renderTarget->id, numReferences );

		node = PlGetNextHashTableNode( renderTargets, node );
	}

	PlDestroyHashTable( renderTargets );
}

SSArlRenderTarget *ss_arl_render_target_get_by_key( const char *key )
{
	return ( SSArlRenderTarget * ) PlLookupHashTableUserData( renderTargets, key, strlen( key ) );
}

SSArlRenderTarget *ss_arl_render_target_create( const char *key, unsigned int width, unsigned int height, unsigned int flags,
                                                unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter )
{
	// Check if it's already been created, and if so, update size to match
	SSArlRenderTarget *renderTarget = ss_arl_render_target_get_by_key( key );
	if ( renderTarget != NULL )
	{
		if ( flags == 0 )
		{
			PRINT_DEBUG( "Placeholder render target \"%s\" was already generated, returning existing\n", key );
			ss_acl_mm_add_reference( &renderTarget->reference );
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
		ss_arl_render_target_set_size( renderTarget, width, height );
		ss_acl_mm_add_reference( &renderTarget->reference );
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

	renderTarget = PL_NEW( SSArlRenderTarget );
	renderTarget->frameBuffer = frameBuffer;
	renderTarget->textureAttachment = textureAttachment;
	renderTarget->textureAttachmentComponent = textureAttachmentComponent;
	renderTarget->textureAttachmentFilter = textureAttachmentFilter;
	snprintf( renderTarget->id, sizeof( renderTarget->id ), "%s", key );

	ss_acl_mm_setup_reference( "RenderTarget", APE_CACHE_POOL_TEXTURES, &renderTarget->reference, destroy_render_target, renderTarget );
	ss_acl_mm_add_reference( &renderTarget->reference );

	PlInsertHashTableNode( renderTargets, key, strlen( key ), renderTarget );

	return renderTarget;
}

void ss_arl_render_target_release( SSArlRenderTarget *renderTarget )
{
	ss_acl_mm_release( &renderTarget->reference );
}

void ss_arl_render_target_set_size( SSArlRenderTarget *renderTarget, unsigned int width, unsigned int height )
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

void ss_arl_render_target_get_size( const SSArlRenderTarget *renderTarget, unsigned int *width, unsigned int *height )
{
	*width = renderTarget->frameBuffer->width;
	*height = renderTarget->frameBuffer->height;
}

PLGTexture *ss_arl_render_target_get_texture( SSArlRenderTarget *renderTarget )
{
	return renderTarget->textureAttachment;
}

void ss_arl_render_target_bind( SSArlRenderTarget *renderTarget, PLGFrameBufferObjectTarget target )
{
	PlgBindFrameBuffer( renderTarget->frameBuffer, target );
}

PLGFrameBuffer *ss_arl_render_target_get_frame_buffer( SSArlRenderTarget *renderTarget )
{
	return renderTarget->frameBuffer;
}
