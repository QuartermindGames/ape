// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Render target management
// Author:  Mark E. Sowden

#include <plcore/pl_hashtable.h>

#include "ape_private.h"

#include "renderer_render_target.h"

typedef struct ApeRenderTarget
{
	char id[ 16 ];// 'rt_menu_0'

	PLGFrameBuffer  *frameBuffer;
	PLGTexture      *attachments[ APE_RENDER_TARGET_MAX_ATTACHMENT_TYPES ];
	unsigned int     desiredAttachments;
	PLGTextureFilter attachmentFilter;

	ApeMemoryReference reference;
} ApeRenderTarget;

static PLHashTable *renderTargets;

static void destroy_texture_attachments( ApeRenderTarget *self )
{
	for ( unsigned int i = 0; i < APE_RENDER_TARGET_MAX_ATTACHMENT_TYPES; ++i )
	{
		if ( self->attachments[ i ] == nullptr )
		{
			continue;
		}

		PlgDestroyTexture( self->attachments[ i ] );
		self->attachments[ i ] = nullptr;
	}
}

static void destroy_render_target( void *user )
{
	ApeRenderTarget *renderTarget = user;

	destroy_texture_attachments( renderTarget );

	if ( renderTarget->frameBuffer != nullptr )
	{
		PlgDestroyFrameBuffer( renderTarget->frameBuffer );
	}

	qm_os_memory_free( renderTarget );
}

static void setup_texture_attachments( ApeRenderTarget *self )
{
	if ( self->desiredAttachments & PLG_BUFFER_COLOUR )
	{
		self->attachments[ APE_RENDER_TARGET_ATTACHMENT_TYPE_COLOUR ] = PlgGetFrameBufferTextureAttachment(
		        self->frameBuffer,
		        PLG_BUFFER_COLOUR,
		        self->attachmentFilter,
		        PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE );
		if ( self->attachments[ APE_RENDER_TARGET_ATTACHMENT_TYPE_COLOUR ] == nullptr )
		{
			ape_console_warning_( "Failed to create colour attachment for buffer: %s\n", PlGetError() );
		}
	}

	if ( self->desiredAttachments & PLG_BUFFER_DEPTH )
	{
		self->attachments[ APE_RENDER_TARGET_ATTACHMENT_TYPE_DEPTH ] = PlgGetFrameBufferTextureAttachment(
		        self->frameBuffer,
		        PLG_BUFFER_DEPTH,
		        self->attachmentFilter,
		        PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE );
		if ( self->attachments[ APE_RENDER_TARGET_ATTACHMENT_TYPE_DEPTH ] == nullptr )
		{
			ape_console_warning_( "Failed to create depth attachment for buffer: %s\n", PlGetError() );
		}
	}

	if ( self->desiredAttachments & PLG_BUFFER_STENCIL )
	{
		self->attachments[ APE_RENDER_TARGET_ATTACHMENT_TYPE_STENCIL ] = PlgGetFrameBufferTextureAttachment(
		        self->frameBuffer,
		        PLG_BUFFER_STENCIL,
		        self->attachmentFilter,
		        PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE );
		if ( self->attachments[ APE_RENDER_TARGET_ATTACHMENT_TYPE_STENCIL ] == nullptr )
		{
			ape_console_warning_( "Failed to create stencil attachment for buffer: %s\n", PlGetError() );
		}
	}
}

void ape_initialize_render_targets_( void )
{
	renderTargets = PlCreateHashTable();
	if ( renderTargets == NULL )
	{
		ape_console_error_( true, "Failed to create render target hash table: %s\n", PlGetError() );
	}
}

void ape_shutdown_render_targets_( void )
{
	ape_memory_flush_unreferenced_resources();

	PLHashTableNode *node = PlGetFirstHashTableNode( renderTargets );
	while ( node != NULL )
	{
		ApeRenderTarget *renderTarget = PlGetHashTableNodeUserData( node );

		int numReferences = ape_memory_get_num_references( &renderTarget->reference );
		if ( numReferences > 0 )
		{
			ape_console_print_( "%s with %u references on shutdown!\n", renderTarget->id, numReferences );
		}

		node = PlGetNextHashTableNode( node );
	}

	PlDestroyHashTable( renderTargets );
}

ApeRenderTarget *ape_render_target_get_by_key_( const char *key )
{
	return PlLookupHashTableUserData( renderTargets, key, strlen( key ) );
}

ApeRenderTarget *ape_render_target_create_( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponents, PLGTextureFilter textureAttachmentFilter, bool useMsaa )
{
	unsigned int numSamples = useMsaa ? ape_config_.renderer.msaaSamples : 0;

	// Check if it's already been created, and if so, update size to match
	ApeRenderTarget *renderTarget = ape_render_target_get_by_key_( key );
	if ( renderTarget != NULL )
	{
		if ( flags == 0 )
		{
			ape_console_verbose_( "Placeholder render target \"%s\" was already generated, returning existing\n", key );

			ape_memory_add_reference( &renderTarget->reference );
			return renderTarget;
		}

		if ( renderTarget->frameBuffer == NULL )
		{
			renderTarget->frameBuffer = PlgCreateFrameBuffer( width, height, flags, numSamples );
			if ( renderTarget->frameBuffer == NULL )
			{
				ape_console_warning_( "Failed to create specified framebuffer for target \"%s\": %s\n", key, PlGetError() );
				return nullptr;
			}
		}

		ape_console_verbose_( "Render target already exists, updating size\n" );

		ape_render_target_set_size_( renderTarget, width, height );
		ape_memory_add_reference( &renderTarget->reference );
		return renderTarget;
	}

	PLGFrameBuffer *frameBuffer;
	if ( flags != 0 )
	{
		frameBuffer = PlgCreateFrameBuffer( width, height, flags, numSamples );
		if ( frameBuffer == NULL )
		{
			ape_console_warning_( "Failed to create specified framebuffer: %s\n", PlGetError() );
			return nullptr;
		}
	}
	else
	{
		ape_console_verbose_( "Creating placeholder render target, \"%s\"\n", key );
		frameBuffer = nullptr;
	}

	renderTarget                     = QM_OS_MEMORY_NEW( ApeRenderTarget );
	renderTarget->desiredAttachments = textureAttachmentComponents;
	renderTarget->attachmentFilter   = textureAttachmentFilter;
	renderTarget->frameBuffer        = frameBuffer;
	if ( renderTarget->frameBuffer != nullptr )
	{
		setup_texture_attachments( renderTarget );
	}

	snprintf( renderTarget->id, sizeof( renderTarget->id ), "%s", key );

	ape_memory_setup_reference( renderTarget->id, APE_CACHE_POOL_TEXTURES, &renderTarget->reference, destroy_render_target, renderTarget );
	ape_memory_add_reference( &renderTarget->reference );

	PlInsertHashTableNode( renderTargets, key, strlen( key ), renderTarget );

	return renderTarget;
}

void ape_render_target_release_( ApeRenderTarget *renderTarget )
{
	ape_memory_release( &renderTarget->reference );
}

void ape_render_target_set_size_( ApeRenderTarget *self, unsigned int width, unsigned int height )
{
	if ( !PlgSetFrameBufferSize( self->frameBuffer, width, height ) )
	{
		ape_console_warning_( "Failed to resize framebuffer: %s\n", PlGetError() );
	}

	destroy_texture_attachments( self );
	setup_texture_attachments( self );
}

void ape_render_target_get_size_( const ApeRenderTarget *renderTarget, unsigned int *width, unsigned int *height )
{
	*width  = renderTarget->frameBuffer->width;
	*height = renderTarget->frameBuffer->height;
}

PLGTexture *ape_render_target_get_texture_( ApeRenderTarget *self, const ApeRenderTargetAttachmentType type )
{
	//TODO: if we requested a type that wasn't setup originally, should we set that up here?
	return self->attachments[ type ];
}

void ape_render_target_bind_( ApeRenderTarget *self, PLGFrameBufferObjectTarget target )
{
	PlgBindFrameBuffer( ( self == NULL ) ? nullptr : self->frameBuffer, target );
}

PLGFrameBuffer *ape_render_target_get_frame_buffer_( ApeRenderTarget *renderTarget )
{
	return renderTarget->frameBuffer;
}
