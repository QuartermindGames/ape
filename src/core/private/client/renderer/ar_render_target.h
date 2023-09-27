// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "renderer.h"

typedef struct ArRenderTarget ArRenderTarget;

ArRenderTarget *ar_render_target_get_by_key( const char *key );
ArRenderTarget *ar_render_target_create( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter );
void ar_render_target_release( ArRenderTarget *renderTarget );
void ar_render_target_set_size( ArRenderTarget *renderTarget, unsigned int width, unsigned int height );
void ar_render_target_get_size( const ArRenderTarget *renderTarget, unsigned int *width, unsigned int *height );
PLGTexture *ar_render_target_get_texture( ArRenderTarget *renderTarget );
void ar_render_target_bind( ArRenderTarget *renderTarget, PLGFrameBufferObjectTarget target );
PLGFrameBuffer *ar_render_target_get_frame_buffer( ArRenderTarget *renderTarget );
