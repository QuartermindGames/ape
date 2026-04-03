// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "renderer.h"

typedef struct ApeRenderTarget ApeRenderTarget;

typedef enum ApeRenderTargetAttachmentType
{
	APE_RENDER_TARGET_ATTACHMENT_TYPE_COLOUR,
	APE_RENDER_TARGET_ATTACHMENT_TYPE_DEPTH,
	APE_RENDER_TARGET_ATTACHMENT_TYPE_STENCIL,

	APE_RENDER_TARGET_MAX_ATTACHMENT_TYPES
} ApeRenderTargetAttachmentType;

ApeRenderTarget *ape_render_target_get_by_key_( const char *key );

ApeRenderTarget *ape_render_target_create_( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponents, PLGTextureFilter textureAttachmentFilter, bool useMsaa );
void             ape_render_target_release_( ApeRenderTarget *renderTarget );
void             ape_render_target_set_size_( ApeRenderTarget *self, unsigned int width, unsigned int height );
void             ape_render_target_get_size_( const ApeRenderTarget *renderTarget, unsigned int *width, unsigned int *height );
PLGTexture      *ape_render_target_get_texture_( ApeRenderTarget *self, ApeRenderTargetAttachmentType type );

/**
 * If the provided render target is null, this will clear whatever is currently set back to the default.
 */
void ape_render_target_bind_( ApeRenderTarget *self, PLGFrameBufferObjectTarget target );

QmGfxFramebuffer *ape_render_target_get_frame_buffer_( ApeRenderTarget *renderTarget );
