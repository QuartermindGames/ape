// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "renderer.h"
#include "ape_image.h"

static PLLinkedList *textures;

/////////////////////////////////////////////////////////////////
// Old API crap

static PLGTexture *fallbackTexture = NULL;

PLGTexture *ss_arl_texture_get_fallback( void )
{
	return fallbackTexture;
}

static PLGTexture *GenerateTextureFromData( uint8_t *data, unsigned int w, unsigned int h, unsigned int numChannels, bool generateMipMap )
{
	PLColourFormat cFormat;
	PLImageFormat iFormat;

	switch ( numChannels )
	{
		default:
			PRINT_WARNING( "Invalid number of colour channels specified!\n" );
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

	PLImage *imageData = PlCreateImage( data, w, h, 0, cFormat, iFormat );
	if ( imageData == NULL )
		PRINT_WARNING( "Failed to generate image data!\nPL: %s\n", PlGetError() );

#if 0
    char outName[ 64 ];
	snprintf( outName, sizeof( outName ), "test_%dx%d-%d.png", w, h, numChannels );
	plWriteImage( imageData, outName );
#endif

	PLGTexture *texture = PlgCreateTexture();
	if ( texture == NULL )
		PRINT_ERROR( "Failed to create texture!\nPL: %s\n", PlGetError() );

	if ( !generateMipMap )
	{
		texture->flags &= PLG_TEXTURE_FLAG_NOMIPS;
		texture->filter = PLG_TEXTURE_FILTER_NEAREST;
	}
	else
		texture->filter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;

	if ( !PlgUploadTextureImage( texture, imageData ) )
		PRINT_ERROR( "Failed to generate texture from image!\nPL: %s\n", PlGetError() );

	PlDestroyImage( imageData );

	return texture;
}

void ss_arl_initialize_textures_( void )
{
	textures = PlCreateLinkedList();

	/* generate fallback texture */
	static PLColour fallbackData[] = {
	        {128,  0,   128, 255},
	        { 0,   128, 128, 255},
	        { 0,   128, 128, 255},
	        { 128, 0,   128, 255},
	};
	fallbackTexture = GenerateTextureFromData( ( uint8_t * ) fallbackData, 2, 2, 4, false );

	/* register the standard image loaders, and our package image loader */
	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );
	PlRegisterImageLoader( "gfx", Image_LoadPackedImage );
}

static PLGTexture *GetTexture( const char *path )
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

PLGTexture *ss_arl_texture_load_direct_( const char *path, PLGTextureFilter filterMode )
{
	/* check if it's already loaded */
	PLGTexture *texture = GetTexture( path );
	if ( texture != NULL )
		return texture;

	texture = PlgLoadTextureFromImage( path, filterMode );
	if ( texture == NULL )
	{
		PRINT_WARNING( "Failed to load texture \"%s\"!\nPL: %s\n", path, PlGetError() );
		return fallbackTexture;
	}

	//TODO: thrown in for Rayman Alive, but we should probably implement a proper API for this
	if ( filterMode == PLG_TEXTURE_FILTER_MIPMAP_LINEAR )
		PlgSetTextureAnisotropy( texture, 16 );

	PlInsertLinkedListNode( textures, texture );
	return texture;
}
