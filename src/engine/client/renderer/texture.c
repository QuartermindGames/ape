/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"
#include "renderer.h"
#include "image.h"

typedef struct RGBMap
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGBMap;
static RGBMap playPal[ 256 ];

static PLLinkedList *textures;

static PLGTexture *fallbackTexture = NULL;

PLGTexture *RT_GetFallbackTexture( void )
{
	return fallbackTexture;
}

static PLGTexture *RT_GenerateTextureFromData( uint8_t *data, unsigned int w, unsigned int h, unsigned int numChannels,
											   bool generateMipMap )
{
	PLColourFormat cFormat;
	PLImageFormat  iFormat;

	switch ( numChannels )
	{
		default:
			PrintWarn( "Invalid number of colour channels specified!\n" );
			return NULL;
		case 3:
			cFormat = PL_COLOURFORMAT_RGB;
			iFormat = PL_IMAGEFORMAT_RGB8;
			break;
		case 4:
			cFormat = PL_COLOURFORMAT_RGBA;
			iFormat = PL_IMAGEFORMAT_RGBA8;
			break;
	}

	PLImage *imageData = PlCreateImage( data, w, h, cFormat, iFormat );
	if ( imageData == NULL )
		PrintWarn( "Failed to generate image data!\nPL: %s\n", PlGetError() );

#if 0
    char outName[ 64 ];
	snprintf( outName, sizeof( outName ), "test_%dx%d-%d.png", w, h, numChannels );
	plWriteImage( imageData, outName );
#endif

	PLGTexture *texture = PlgCreateTexture();
	if ( texture == NULL )
		PrintError( "Failed to create texture!\nPL: %s\n", PlGetError() );

	if ( !generateMipMap )
	{
		texture->flags &= PLG_TEXTURE_FLAG_NOMIPS;
		texture->filter = PLG_TEXTURE_FILTER_LINEAR;
	}
	else
		texture->filter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;

	if ( !PlgUploadTextureImage( texture, imageData ) )
		PrintError( "Failed to generate texture from image!\nPL: %s\n", PlGetError() );

	PlDestroyImage( imageData );

	return texture;
}

void RT_InitializeTextures( void )
{
	textures = PlCreateLinkedList();

	/* generate fallback texture */
	static PLColour fallbackData[] = {
			{ 128, 0, 128, 255 },
			{ 0, 128, 128, 255 },
			{ 0, 128, 128, 255 },
			{ 128, 0, 128, 255 },
	};
	fallbackTexture = RT_GenerateTextureFromData( ( uint8_t * ) fallbackData, 2, 2, 4, false );
	if ( fallbackTexture == NULL )
		PrintError( "Failed to create fallback texture!\n" );

	/* register the standard image loaders, and our package image loader */
	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );
	PlRegisterImageLoader( "gfx", Image_LoadPackedImage );

	/* load the numbers */
	/*
    for ( unsigned int i = 0; i < 10; ++i ) {
        char numName[ 16 ];
        snprintf( numName, sizeof( numName ), "WNUMBER%d", i );
        numTextureTable[ i ] = Gfx_LoadLumpTexture( titlePal, numName );
    }
    */
}

PLGTexture *RT_GetTexture( const char *path )
{
	PLLinkedListNode *node = PlGetFirstNode( textures );
	while ( node != NULL )
	{
		PLGTexture *texture = PlGetLinkedListNodeUserData( node );
		if ( pl_strcasecmp( path, texture->path ) == 0 )
			return texture;

		node = PlGetNextLinkedListNode( node );
	}

	return NULL;
}

PLGTexture *RT_LoadTexture( const char *path )
{
	/* check if it's already loaded */
	PLGTexture *texture = RT_GetTexture( path );
	if ( texture != NULL )
		return texture;

	texture = PlgLoadTextureFromImage( path, PLG_TEXTURE_FILTER_NEAREST );
	if ( texture == NULL )
	{
		PrintWarn( "Failed to load texture \"%s\"!\nPL: %s\n", path, PlGetError() );
		return fallbackTexture;
	}

	PlInsertLinkedListNode( textures, texture );
	return texture;
}
