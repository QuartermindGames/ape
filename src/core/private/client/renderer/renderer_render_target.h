// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "renderer.h"

typedef struct SSArlRenderTarget SSArlRenderTarget;

SSArlRenderTarget *ss_arl_render_target_get_by_key( const char *key );
SSArlRenderTarget *ss_arl_render_target_create( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter );
void ss_arl_render_target_release( SSArlRenderTarget *renderTarget );
void ss_arl_render_target_set_size( SSArlRenderTarget *renderTarget, unsigned int width, unsigned int height );
void ss_arl_render_target_get_size( const SSArlRenderTarget *renderTarget, unsigned int *width, unsigned int *height );
PLGTexture *ss_arl_render_target_get_texture( SSArlRenderTarget *renderTarget );
void ss_arl_render_target_bind( SSArlRenderTarget *renderTarget, PLGFrameBufferObjectTarget target );
PLGFrameBuffer *ss_arl_render_target_get_frame_buffer( SSArlRenderTarget *renderTarget );
