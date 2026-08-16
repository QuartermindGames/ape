// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "memory/memory.h"

PL_EXTERN_C

typedef enum ApeDefaultTexture
{
	APE_TEXTURE_FALLBACK,
	APE_TEXTURE_WHITE,
	APE_TEXTURE_BLACK,

	APE_MAX_DEFAULT_TEXTURES
} ApeDefaultTexture;

#define APE_TEXTURE_FLAG_PRESERVE ( 1 << 0 )

typedef struct ApeTexture
{
	ApeMemoryReference reference;

	QmImage      *image;   // ram copy, usually free'd after load but editor will retain
	QmGfxTexture *internal;// vram copy

	char *path;// for reloading
#ifdef APE_SUPPORT_EDITOR
	time_t modifiedTime;// modified timestamp, for reload
#endif

	unsigned int flags;

	QmMathColour4ub average;
} ApeTexture;

APE_MEMORY_IMPLEMENT_INTERFACE_DECL( ape_texture, ApeTexture )

ApeTexture *ape_texture_generate_( const char *id, void *data, unsigned int w, unsigned int h, const QmImagePixelFormatDescriptor *format, QmGfxTextureFilter filter );

ApeTexture *ape_texture_cache_( const char *path, QmGfxTextureFilter filter, bool useFallback );
ApeTexture *ape_texture_cache_cubemap_( char **paths, QmGfxTextureFilter filter );

#ifdef APE_SUPPORT_EDITOR
void ape_texture_reload_( ApeTexture *self );
#endif

ApeTexture *ape_get_default_texture_( ApeDefaultTexture defaultTexture );

PL_EXTERN_C_END
