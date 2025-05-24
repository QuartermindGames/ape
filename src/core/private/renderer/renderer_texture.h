// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape_memory.h"

PL_EXTERN_C

typedef enum ApeDefaultTexture
{
	APE_TEXTURE_FALLBACK,

	APE_MAX_DEFAULT_TEXTURES
} ApeDefaultTexture;

#define APE_TEXTURE_FLAG_PRESERVE ( 1 << 0 )

typedef struct ApeTexture
{
	ApeMemoryReference reference;
	PLGTexture        *internal;

	PLGTextureWrapMode wrapMode;
	PLGTextureFilter   filterMode;
	unsigned int       flags;

	PLPath path;// for reloading
} ApeTexture;

ApeTexture *ape_texture_cache_( const char *path, PLGTextureFilter filter, bool useFallback );

ApeTexture *ape_get_default_texture_( ApeDefaultTexture defaultTexture );

PL_EXTERN_C_END
