// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "renderer.h"

typedef struct ArRenderTarget ArRenderTarget;

ArRenderTarget *ar_render_target_get_by_key( const char *key );
ArRenderTarget *arl_render_target_create( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter );
void arl_render_target_release( ArRenderTarget *renderTarget );
void arl_render_target_set_size( ArRenderTarget *renderTarget, unsigned int width, unsigned int height );
void arl_render_target_get_size( const ArRenderTarget *renderTarget, unsigned int *width, unsigned int *height );
PLGTexture *arl_render_target_get_texture( ArRenderTarget *renderTarget );
void arl_render_target_bind( ArRenderTarget *renderTarget, PLGFrameBufferObjectTarget target );
PLGFrameBuffer *arl_render_target_get_frame_buffer( ArRenderTarget *renderTarget );
