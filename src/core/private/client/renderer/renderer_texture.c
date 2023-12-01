// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "renderer.h"
#include "ape_image.h"

#include "plcore/pl_hashtable.h"

/////////////////////////////////////////////////////////////////
// Old API crap

PLGTexture *ss_arl_texture_get_fallback( void )
{
	return ss_arl_get_default_texture_( SS_ARL_TEXTURE_FALLBACK )->internal;
}

PLGTexture *ss_arl_texture_load_direct_( const char *path, PLGTextureFilter filterMode )
{
	return ss_arl_texture_cache_( path, true )->internal;
}

/////////////////////////////////////////////////////////////////
// Private

static PLHashTable *textureTable;
static SSArlTexture *defaultTextures[ SS_ARL_MAX_DEFAULT_TEXTURES ];

static void destroy_texture( void *userData )
{
	SSArlTexture *texture = ( SSArlTexture * ) userData;
	if ( texture == NULL || texture->flags & SS_ARL_TEXTURE_FLAG_PRESERVE )
		return;

	//TODO: set hashtable lookup index to null...

	PlgDestroyTexture( texture->internal );
	PL_DELETE( texture );
}

static SSArlTexture *generate_texture( const char *id, uint8_t *data, unsigned int w, unsigned int h, unsigned int numChannels, bool generateMipMap )
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

	PLGTexture *internalTexture = PlgCreateTexture();
	if ( internalTexture == NULL )
		PRINT_ERROR( "Failed to create texture!\nPL: %s\n", PlGetError() );

	if ( !generateMipMap )
	{
		internalTexture->flags &= PLG_TEXTURE_FLAG_NOMIPS;
		internalTexture->filter = PLG_TEXTURE_FILTER_NEAREST;
	}
	else
		internalTexture->filter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;

	if ( !PlgUploadTextureImage( internalTexture, imageData ) )
		PRINT_ERROR( "Failed to generate texture from image!\nPL: %s\n", PlGetError() );

	PlDestroyImage( imageData );

	SSArlTexture *texture = PL_NEW( SSArlTexture );
	texture->filterMode = internalTexture->filter;

	ss_acl_mm_setup_reference( "texture", APE_CACHE_POOL_TEXTURES, &texture->reference, destroy_texture, NULL );

	return texture;
}

static void fetch_texture_config( SSArlTexture *texture )
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

	NdBranch *root = ndLoadFile( configPath, "texture" );
	if ( root == NULL )
		return;

	const char *wrapMode = ndGetStringByName( root, "wrapMode", "repeat" );
	if ( strcmp( wrapMode, "repeat" ) == 0 )
		texture->wrapMode = PLG_TEXTURE_WRAP_MODE_REPEAT;
	else if ( strcmp( wrapMode, "mirrored_repeat" ) == 0 )
		texture->wrapMode = PLG_TEXTURE_WRAP_MODE_MIRRORED_REPEAT;
	else if ( strcmp( wrapMode, "clamp" ) == 0 )
		texture->wrapMode = PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE;
	else if ( strcmp( wrapMode, "clamp_border" ) == 0 )
		texture->wrapMode = PLG_TEXTURE_WRAP_MODE_CLAMP_BORDER;
	PlgSetTextureWrapMode( texture->internal, texture->wrapMode );

	const char *filterMode = ndGetStringByName( root, "filterMode", "linear" );
	if ( strcmp( filterMode, "mipmap_linear" ) == 0 )
		texture->filterMode = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;
	else if ( strcmp( filterMode, "linear" ) == 0 )
		texture->filterMode = PLG_TEXTURE_FILTER_LINEAR;
	else if ( strcmp( filterMode, "mipmap_nearest" ) == 0 )
		texture->filterMode = PLG_TEXTURE_FILTER_MIPMAP_NEAREST;
	else if ( strcmp( filterMode, "nearest" ) == 0 )
		texture->filterMode = PLG_TEXTURE_FILTER_NEAREST;
	PlgSetTextureFilter( texture->internal, texture->filterMode );

	ndDestroyBranch( root );
}

/////////////////////////////////////////////////////////////////
// Public

void ss_arl_initialize_textures_( void )
{
	// register the standard image loaders, and our package image loader
	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );
	PlRegisterImageLoader( "gfx", Image_LoadPackedImage );

	textureTable = PlCreateHashTable();
	if ( textureTable == NULL )
		PRINT_ERROR( "Failed to create texture table: %s\n", PlGetError() );

	// generate fallback texture
	static PLColour fallbackData[] = {
	        {128,  0,   128, 255},
	        { 0,   128, 128, 255},
	        { 0,   128, 128, 255},
	        { 128, 0,   128, 255},
	};
	defaultTextures[ SS_ARL_TEXTURE_FALLBACK ] = generate_texture( NULL, ( uint8_t * ) fallbackData, 2, 2, 4, false );
	defaultTextures[ SS_ARL_TEXTURE_FALLBACK ]->flags |= SS_ARL_TEXTURE_FLAG_PRESERVE;
}

SSArlTexture *ss_arl_texture_cache_( const char *path, bool useFallback )
{
	SSArlTexture *texture = PlLookupHashTableUserData( textureTable, path, strlen( path ) );
	if ( texture != NULL )
	{
		ss_acl_mm_add_reference( &texture->reference );
		return texture;
	}

	PLGTexture *internalTexture = PlgLoadTextureFromImage( path, PLG_TEXTURE_FILTER_MIPMAP_LINEAR );
	if ( internalTexture == NULL )
	{
		PRINT_WARNING( "Failed to load texture (%s): %s\n", path, PlGetError() );
		return ( useFallback ) ? defaultTextures[ SS_ARL_TEXTURE_FALLBACK ] : NULL;
	}

	texture = PL_NEW( SSArlTexture );
	texture->internal = internalTexture;
	texture->filterMode = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;
	texture->wrapMode = PLG_TEXTURE_WRAP_MODE_REPEAT;
	PlSetupPath( texture->path, true, "%s", path );

	fetch_texture_config( texture );

	ss_acl_mm_setup_reference( "texture", APE_CACHE_POOL_TEXTURES, &texture->reference, destroy_texture, NULL );
	ss_acl_mm_add_reference( &texture->reference );

	//TODO: thrown in for Rayman Alive, but we should probably implement a proper API for this
	PlgSetTextureAnisotropy( texture->internal, 16 );

	return texture;
}

SSArlTexture *ss_arl_get_default_texture_( SSArlDefaultTexture defaultTexture )
{
	return defaultTextures[ defaultTexture ];
}
