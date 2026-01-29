// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "../memory/memory.h"

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

PLGTexture *ape_texture_load_direct_( const char *path, PLGTextureFilter filterMode );
PLGTexture *ape_texture_get_fallback( void );

ApeTexture *ape_texture_generate_( const char *id, void *data, unsigned int w, unsigned int h, const QmImagePixelFormatDescriptor *format, bool generateMipMap );

ApeTexture *ape_texture_cache_( const char *path, PLGTextureFilter filter, bool useFallback );
void        ape_texture_release_( ApeTexture *texture );

ApeTexture *ape_get_default_texture_( ApeDefaultTexture defaultTexture );

PL_EXTERN_C_END
