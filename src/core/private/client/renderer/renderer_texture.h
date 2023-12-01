// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <yin/core_renderer.h>

PL_EXTERN_C

typedef enum SSArlDefaultTexture
{
	SS_ARL_TEXTURE_FALLBACK,

	SS_ARL_MAX_DEFAULT_TEXTURES
} SSArlDefaultTexture;

#define SS_ARL_TEXTURE_FLAG_PRESERVE ( 1 << 0 )

typedef struct SSArlTexture
{
	ApeMemoryReference reference;
	PLGTexture *internal;

	PLGTextureWrapMode wrapMode;
	PLGTextureFilter filterMode;
	unsigned int flags;

	PLPath path;// for reloading
} SSArlTexture;

SSArlTexture *ss_arl_texture_cache_( const char *path, bool useFallback );

SSArlTexture *ss_arl_get_default_texture_( SSArlDefaultTexture defaultTexture );

PL_EXTERN_C_END
