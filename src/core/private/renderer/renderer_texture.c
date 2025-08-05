// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "plcore/pl_hashtable.h"

#include "ape_private.h"
#include "ape_image.h"

#include "renderer.h"
#include "renderer_texture.h"

/////////////////////////////////////////////////////////////////
// Old API crap

PLGTexture *ape_texture_get_fallback( void )
{
	return ape_get_default_texture_( APE_TEXTURE_FALLBACK )->internal;
}

PLGTexture *ape_texture_load_direct_( const char *path, PLGTextureFilter filterMode )
{
	return ape_texture_cache_( path, filterMode, true )->internal;
}

/////////////////////////////////////////////////////////////////
// Private

static PLHashTable *textureTable;
static ApeTexture  *defaultTextures[ APE_MAX_DEFAULT_TEXTURES ];

static void destroy_texture( void *userData )
{
	ApeTexture *texture = userData;
	if ( texture == NULL || texture->flags & APE_TEXTURE_FLAG_PRESERVE )
		return;

	//TODO: set hashtable lookup index to null...

	PlgDestroyTexture( texture->internal );
	PL_DELETE( texture );
}

static ApeTexture *generate_texture( const char *id, void *data, unsigned int w, unsigned int h, unsigned int numChannels, bool generateMipMap )
{
	PLColourFormat cFormat;
	PLImageFormat  iFormat;

	switch ( numChannels )
	{
		default:
			ape_warning_( "Invalid number of colour channels specified!\n" );
			return nullptr;
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
	if ( imageData == nullptr )
	{
		ape_warning_( "Failed to generate image (%s) data: %s\n", id, PlGetError() );
	}

#if 0
    char outName[ 64 ];
	snprintf( outName, sizeof( outName ), "test_%dx%d-%d.png", w, h, numChannels );
	plWriteImage( imageData, outName );
#endif

	PLGTexture *internalTexture = PlgCreateTexture();
	if ( internalTexture == NULL )
	{
		ape_error_( true, "Failed to create texture (%s): %s\n", id, PlGetError() );
	}

	if ( !generateMipMap )
	{
		internalTexture->filter = PLG_TEXTURE_FILTER_NEAREST;
	}
	else
	{
		internalTexture->filter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;
	}

	if ( !PlgUploadTextureImage( internalTexture, imageData ) )
	{
		ape_error_( true, "Failed to generate texture from image (%s): %s\n", id, PlGetError() );
	}

	PlDestroyImage( imageData );

	ApeTexture *texture = PL_NEW( ApeTexture );
	texture->filterMode = internalTexture->filter;

	ape_memory_setup_reference( id, APE_CACHE_POOL_TEXTURES, &texture->reference, destroy_texture, NULL );

	return texture;
}

static void fetch_texture_config( ApeTexture *texture )
{
	PLPath configPath;
	PlSetupPath( configPath, "%s", texture->path );
	char *c = strrchr( configPath, '/' );
	if ( c == NULL )
	{
		PRINT_WARNING( "Failed to find path separator for path (%s)!\n", configPath );
		return;
	}

	c = strchr( c, '.' );
	if ( c == NULL )
	{
		PRINT_WARNING( "Failed to find extension denominator (%s)!\n", configPath );
		return;
	}

	*c = '\0';
	strcat( configPath, ".tex.n" );

	if ( !PlFileExists( configPath ) )
	{
		return;
	}

	AcmBranch *root = com_acm_load_file( configPath, "texture" );
	if ( root == NULL )
	{
		return;
	}

	const char *wrapMode = acm_get_string( root, "wrapMode", "repeat" );
	if ( strcmp( wrapMode, "repeat" ) == 0 )
	{
		texture->wrapMode = PLG_TEXTURE_WRAP_MODE_REPEAT;
	}
	else if ( strcmp( wrapMode, "mirrored_repeat" ) == 0 )
	{
		texture->wrapMode = PLG_TEXTURE_WRAP_MODE_MIRRORED_REPEAT;
	}
	else if ( strcmp( wrapMode, "clamp" ) == 0 )
	{
		texture->wrapMode = PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE;
	}
	else if ( strcmp( wrapMode, "clamp_border" ) == 0 )
	{
		texture->wrapMode = PLG_TEXTURE_WRAP_MODE_CLAMP_BORDER;
	}

	const char *filterMode = acm_get_string( root, "filterMode", "linear" );
	if ( strcmp( filterMode, "mipmap_linear" ) == 0 )
	{
		texture->filterMode = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;
	}
	else if ( strcmp( filterMode, "linear" ) == 0 )
	{
		texture->filterMode = PLG_TEXTURE_FILTER_LINEAR;
	}
	else if ( strcmp( filterMode, "mipmap_nearest" ) == 0 )
	{
		texture->filterMode = PLG_TEXTURE_FILTER_MIPMAP_NEAREST;
	}
	else if ( strcmp( filterMode, "nearest" ) == 0 )
	{
		texture->filterMode = PLG_TEXTURE_FILTER_NEAREST;
	}

	acm_branch_destroy( root );
}

/////////////////////////////////////////////////////////////////
// Public

void ape_initialize_textures_( void )
{
	// register the standard image loaders, and our package image loader
	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );
	PlRegisterImageLoader( "gfx", Image_LoadPackedImage );

	textureTable = PlCreateHashTable();
	if ( textureTable == NULL )
	{
		ape_error_( true, "Failed to create texture table: %s\n", PlGetError() );
	}

	// generate fallback texture
	static PLColour fallbackData[] = {
	        {128, 0,   128, 255},
	        {0,   128, 128, 255},
	        {0,   128, 128, 255},
	        {128, 0,   128, 255},
	};
	defaultTextures[ APE_TEXTURE_FALLBACK ] = generate_texture( "fallback", fallbackData, 2, 2, 4, false );
	defaultTextures[ APE_TEXTURE_FALLBACK ]->flags |= APE_TEXTURE_FLAG_PRESERVE;
}

ApeTexture *ape_texture_cache_( const char *path, PLGTextureFilter filter, bool useFallback )
{
	ApeTexture *texture = PlLookupHashTableUserData( textureTable, path, strlen( path ) );
	if ( texture != NULL )
	{
		ape_memory_add_reference( &texture->reference );
		return texture;
	}

	texture             = PL_NEW( ApeTexture );
	texture->filterMode = filter;
	texture->wrapMode   = PLG_TEXTURE_WRAP_MODE_REPEAT;
	PlSetupPath( texture->path, true, "%s", path );

	fetch_texture_config( texture );

	texture->internal = PlgLoadTextureFromImage( path, texture->filterMode );
	if ( texture->internal == nullptr )
	{
		ape_warning_( "Failed to load texture (%s): %s\n", path, PlGetError() );
		PL_DELETE( texture );
		return ( useFallback ) ? defaultTextures[ APE_TEXTURE_FALLBACK ] : nullptr;
	}

	PlgSetTextureWrapMode( texture->internal, texture->wrapMode );
	PlgSetTextureFilter( texture->internal, texture->filterMode );

	ape_memory_setup_reference( texture->path, APE_CACHE_POOL_TEXTURES, &texture->reference, destroy_texture, nullptr );
	ape_memory_add_reference( &texture->reference );

	return texture;
}

ApeTexture *ape_get_default_texture_( ApeDefaultTexture defaultTexture )
{
	return defaultTextures[ defaultTexture ];
}
