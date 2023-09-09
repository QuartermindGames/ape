// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "renderer.h"

typedef struct ApeRenderTarget {
	char id[ 16 ];// 'rt_menu_0'

	PLGTexture *textureAttachment;
	unsigned int textureAttachmentComponent;
	PLGTextureFilter textureAttachmentFilter;

	PLGFrameBuffer *frameBuffer;
	ApeMemoryReference reference;
} ApeRenderTarget;

static PLHashTable *renderTargets;

void apeInitializeRenderTargets_( void ) {
	renderTargets = PlCreateHashTable();
	if ( renderTargets == NULL ) {
		PRINT_ERROR( "Failed to create render target hash table: %s\n", PlGetError() );
	}
}

void apeShutdownRenderTargets_( void ) {
	apeFlushUnreferencedResources();

	PLHashTableNode *node = PlGetFirstHashTableNode( renderTargets );
	while ( node != NULL ) {
		ApeRenderTarget *renderTarget = ( ApeRenderTarget * ) PlGetHashTableNodeUserData( node );

		int numReferences = apeGetNumberOfReferences( &renderTarget->reference );
		if ( numReferences > 0 ) {
			PRINT( "%s with %u references on shutdown!\n", renderTarget->id, numReferences );
		}

		node = PlGetNextHashTableNode( renderTargets, node );
	}

	PlDestroyHashTable( renderTargets );
}

ApeRenderTarget *apeGetRenderTargetByKey( const char *key ) {
	return ( ApeRenderTarget * ) PlLookupHashTableUserData( renderTargets, key, strlen( key ) );
}

static PLGFrameBuffer *CreateFrameBuffer( unsigned int width, unsigned int height, unsigned int flags ) {
	PLGFrameBuffer *frameBuffer = PlgCreateFrameBuffer( width, height, flags );
	if ( frameBuffer == NULL ) {
		PRINT_WARNING( "Failed to create specified framebuffer: %s\n", PlGetError() );
		return NULL;
	}

	return frameBuffer;
}

static void DestroyRenderTarget( void *user ) {
	ApeRenderTarget *renderTarget = ( ApeRenderTarget * ) user;
	PlgDestroyTexture( renderTarget->textureAttachment );
}

ApeRenderTarget *apeCreateRenderTarget( const char *key, unsigned int width, unsigned int height, unsigned int flags,
                                        unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter ) {
	// Check if it's already been created, and if so, update size to match
	ApeRenderTarget *renderTarget = apeGetRenderTargetByKey( key );
	if ( renderTarget != NULL ) {
		if ( flags == 0 ) {
			PRINT_DEBUG( "Placeholder render target \"%s\" was already generated, returning existing\n", key );
			apeAddReference( &renderTarget->reference );
			return renderTarget;
		}

		if ( renderTarget->frameBuffer == NULL ) {
			renderTarget->frameBuffer = PlgCreateFrameBuffer( width, height, flags );
			if ( renderTarget->frameBuffer == NULL ) {
				PRINT_WARNING( "Failed to create specified framebuffer for target \"%s\": %s\n", key, PlGetError() );
				return NULL;
			}
		}

		PRINT_DEBUG( "Render target already exists, updating size\n" );
		apeSetRenderTargetSize( renderTarget, width, height );
		apeAddReference( &renderTarget->reference );
		return renderTarget;
	}

	PLGFrameBuffer *frameBuffer;
	PLGTexture *textureAttachment;
	if ( flags != 0 ) {
		frameBuffer = CreateFrameBuffer( width, height, flags );
		if ( frameBuffer == NULL ) {
			PRINT_WARNING( "Failed to create render target, \"%s\"\n", key );
			return NULL;
		}

		textureAttachment = PlgGetFrameBufferTextureAttachment( frameBuffer, textureAttachmentComponent, textureAttachmentFilter );
		if ( textureAttachment == NULL ) {
			PRINT_WARNING( "Failed to create texture attachment, \"%s\":\n", key, PlGetError() );
		}
	} else {
		PRINT_DEBUG( "Creating placeholder render target, \"%s\"\n", key );
		frameBuffer = NULL;
	}

	renderTarget = PL_NEW( ApeRenderTarget );
	renderTarget->frameBuffer = frameBuffer;
	renderTarget->textureAttachment = textureAttachment;
	renderTarget->textureAttachmentComponent = textureAttachmentComponent;
	renderTarget->textureAttachmentFilter = textureAttachmentFilter;
	snprintf( renderTarget->id, sizeof( renderTarget->id ), "%s", key );

	apeSetupReference( "RenderTarget", APE_CACHE_POOL_TEXTURES, &renderTarget->reference, DestroyRenderTarget, renderTarget );
	apeAddReference( &renderTarget->reference );

	PlInsertHashTableNode( renderTargets, key, strlen( key ), renderTarget );

	return renderTarget;
}

void apeReleaseRenderTarget( ApeRenderTarget *renderTarget ) {
	apeReleaseReference( &renderTarget->reference );
}

void apeSetRenderTargetSize( ApeRenderTarget *renderTarget, unsigned int width, unsigned int height ) {
	PlgSetFrameBufferSize( renderTarget->frameBuffer, width, height );
	if ( PlGetFunctionResult() != PL_RESULT_SUCCESS ) {
		PRINT_WARNING( "Failed to resize framebuffer: %s\n", PlGetError() );
	}

	if ( renderTarget->textureAttachment != NULL ) {
		PlgDestroyTexture( renderTarget->textureAttachment );
	}

	renderTarget->textureAttachment = PlgGetFrameBufferTextureAttachment(
	        renderTarget->frameBuffer,
	        renderTarget->textureAttachmentComponent,
	        renderTarget->textureAttachmentFilter );
}

PLGTexture *apeGetRenderTargetTextureAttachment( ApeRenderTarget *renderTarget ) {
	return renderTarget->textureAttachment;
}

void apeBindRenderTarget( ApeRenderTarget *renderTarget, PLGFrameBufferObjectTarget target ) {
	PlgBindFrameBuffer( renderTarget->frameBuffer, target );
}
