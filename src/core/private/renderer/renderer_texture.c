// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "plcore/pl_hashtable.h"
#include "qmos/public/qm_os_string.h"

#include "ape_private.h"
#include "ape_image.h"

#include "renderer.h"
#include "renderer_texture.h"

#include "editor/editor.h"

static PLHashTable *textureTable;
static ApeTexture  *defaultTextures[ APE_MAX_DEFAULT_TEXTURES ];

APE_MEMORY_IMPLEMENT_INTERFACE( ape_texture, ApeTexture, reference )

static void destroy_texture( void *userData )
{
	ApeTexture *texture = userData;
	if ( texture == NULL || texture->flags & APE_TEXTURE_FLAG_PRESERVE )
		return;

	//TODO: set hashtable lookup index to null...

	qm_os_memory_free( texture->path );

	PlDestroyImage( texture->image );
	qm_os_memory_free( texture->internal );
	qm_os_memory_free( texture );
}

static void compute_average_colour( ApeTexture *texture )
{
	if ( !ape_editor_is_active() )
	{
		return;
	}

	PLImageFormat format = PlGetImageFormat( texture->image );
	if ( format != PL_IMAGEFORMAT_RGB8 && format != PL_IMAGEFORMAT_RGBA8 )
	{
		return;
	}

	// for the sake of speed, when determining the average colour,
	// we're only going over parts of the image rather than the entire thing.
	// this obviously isn't accurate but it's good enough for our needs.

	unsigned int w     = qm_image_get_width( texture->image );
	unsigned int h     = qm_image_get_height( texture->image );
	unsigned int hw    = w / 2;
	unsigned int hh    = h / 2;
	unsigned int hsize = hw * hh;
	if ( hsize == 0 )
	{
		ape_console_warning_( "Suspicious image size, skipping computation of average colour!\n" );
		return;
	}

	unsigned int stride = PlGetImageFormatPixelSize( format );

	struct
	{
		unsigned int r;
		unsigned int g;
		unsigned int b;
		unsigned int a;
	} out = {};

	uint8_t *p = PlGetImageData( texture->image, 0, 0 );
	for ( unsigned int i = 0; i < hsize; ++i, p += stride )
	{
		if ( format == PL_IMAGEFORMAT_RGB8 )
		{
			QmMathColour3ub *pixel = ( QmMathColour3ub * ) p;
			out.r += pixel->r;
			out.g += pixel->g;
			out.b += pixel->b;
		}
		else
		{
			QmMathColour4ub *pixel = ( QmMathColour4ub * ) p;
			out.r += pixel->r;
			out.g += pixel->g;
			out.b += pixel->b;
			out.a += pixel->a;
		}
	}

	texture->average.r = ( uint8_t ) ( out.r / hsize );
	texture->average.g = ( uint8_t ) ( out.g / hsize );
	texture->average.b = ( uint8_t ) ( out.b / hsize );
	texture->average.a = ( uint8_t ) ( out.a / hsize );
}

ApeTexture *ape_texture_generate_( const char *id, void *data, unsigned int w, unsigned int h, const QmImagePixelFormatDescriptor *format, const QmGfxTextureFilter filter )
{
	PLColourFormat cFormat;
	PLImageFormat  iFormat;

	switch ( format->numChannels )
	{
		default:
			ape_console_warning_( "Invalid number of colour channels specified!\n" );
			return nullptr;
		case 3:
			cFormat = PL_COLOURFORMAT_RGB;
			if ( format->channels[ 0 ].format == QM_IMAGE_DATA_FORMAT_F32 )
			{
				iFormat = PL_IMAGEFORMAT_RGB32F;
			}
			else if ( format->channels[ 0 ].format == QM_IMAGE_DATA_FORMAT_F16 )
			{
				iFormat = PL_IMAGEFORMAT_RGB16F;
			}
			else
			{
				iFormat = PL_IMAGEFORMAT_RGB8;
			}
			break;
		case 4:
			cFormat = PL_COLOURFORMAT_RGBA;
			if ( format->channels[ 0 ].format == QM_IMAGE_DATA_FORMAT_F32 )
			{
				iFormat = PL_IMAGEFORMAT_RGBA32F;
			}
			else if ( format->channels[ 0 ].format == QM_IMAGE_DATA_FORMAT_F16 )
			{
				iFormat = PL_IMAGEFORMAT_RGBA16F;
			}
			else
			{
				iFormat = PL_IMAGEFORMAT_RGBA8;
			}
			break;
	}

	QmImage *imageData = PlCreateImage( data, w, h, 0, cFormat, iFormat );
	if ( imageData == nullptr )
	{
		ape_console_warning_( "Failed to generate image (%s) data: %s\n", id, PlGetError() );
	}

#if 0
    char outName[ 64 ];
	snprintf( outName, sizeof( outName ), "test_%dx%d-%d.png", w, h, numChannels );
	plWriteImage( imageData, outName );
#endif

	QmGfxTexture *internalTexture = qm_gfx_texture_create( QM_GFX_TEXTURE_TYPE_2D );
	if ( internalTexture == nullptr )
	{
		ape_console_error_( true, "Failed to create texture (%s): %s\n", id, PlGetError() );
		return nullptr;
	}

	internalTexture->filter = filter;

	if ( !qm_gfx_texture_upload( internalTexture, imageData ) )
	{
		ape_console_error_( true, "Failed to generate texture from image (%s): %s\n", id, PlGetError() );
	}

	ApeTexture *texture = QM_OS_MEMORY_NEW( ApeTexture );
	texture->filterMode = internalTexture->filter;
	texture->internal   = internalTexture;
	texture->image      = imageData;

	compute_average_colour( texture );

	if ( !ape_editor_is_active() )
	{
		PlDestroyImage( texture->image );
		texture->image = nullptr;
	}

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
		ape_console_warning_( "Failed to find path separator for path (%s)!\n", configPath );
		return;
	}

	c = strchr( c, '.' );
	if ( c == NULL )
	{
		ape_console_warning_( "Failed to find extension denominator (%s)!\n", configPath );
		return;
	}

	*c = '\0';
	strcat( configPath, ".tex.n" );

	if ( !qm_fs_check_file_exists( configPath ) )
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
		ape_console_error_( true, "Failed to create texture table: %s\n", PlGetError() );
	}

	// generate fallback texture
	static QmMathColour4ub fallbackData[] = {
	        QM_MATH_COLOUR4UB_RGB( 255, 0, 255 ),
	        QM_MATH_COLOUR4UB_RGB( 0, 0, 0 ),
	        QM_MATH_COLOUR4UB_RGB( 0, 0, 0 ),
	        QM_MATH_COLOUR4UB_RGB( 255, 0, 255 ),
	};
	defaultTextures[ APE_TEXTURE_FALLBACK ] = ape_texture_generate_( "fallback", fallbackData, 2, 2, &QM_IMAGE_FORMAT_RGBA8_DESC(), PLG_TEXTURE_FILTER_NEAREST );
	defaultTextures[ APE_TEXTURE_FALLBACK ]->flags |= APE_TEXTURE_FLAG_PRESERVE;

	defaultTextures[ APE_TEXTURE_WHITE ] = ape_texture_cache_( "materials/shaders/textures/white.png", PLG_TEXTURE_FILTER_NEAREST, true );
	defaultTextures[ APE_TEXTURE_WHITE ]->flags |= APE_TEXTURE_FLAG_PRESERVE;

	defaultTextures[ APE_TEXTURE_BLACK ] = ape_texture_cache_( "materials/shaders/textures/black.png", PLG_TEXTURE_FILTER_NEAREST, true );
	defaultTextures[ APE_TEXTURE_BLACK ]->flags |= APE_TEXTURE_FLAG_PRESERVE;
}

ApeTexture *ape_texture_cache_( const char *path, QmGfxTextureFilter filter, bool useFallback )
{
	ApeTexture *texture = PlLookupHashTableUserData( textureTable, path, strlen( path ) );
	if ( texture != NULL )
	{
		ape_memory_reference_add( &texture->reference );
		return texture;
	}

	texture             = QM_OS_MEMORY_NEW( ApeTexture );
	texture->path       = qm_os_string_alloc( "%s", path );
	texture->filterMode = filter;
	texture->wrapMode   = PLG_TEXTURE_WRAP_MODE_REPEAT;

	fetch_texture_config( texture );

	texture->image = qm_image_load( path );
	if ( texture->image == nullptr )
	{
		ape_console_warning_( "Failed to load image (%s): %s\n", path, PlGetError() );
		goto cleanup;
	}

	// upload it

	texture->internal = qm_gfx_texture_create( QM_GFX_TEXTURE_TYPE_2D );
	if ( texture->internal == nullptr )
	{
		ape_console_warning_( "Failed to allocate internal texture (%s): %s\n", path, PlGetError() );
		goto cleanup;
	}

	//TODO: wat?
	texture->internal->filter = texture->filterMode;

	if ( !qm_gfx_texture_upload( texture->internal, texture->image ) )
	{
		ape_console_warning_( "Failed to upload texture (%s): %s\n", path, PlGetError() );
		goto cleanup;
	}

	compute_average_colour( texture );

	if ( !ape_editor_is_active() )
	{
		PlDestroyImage( texture->image );
		texture->image = nullptr;
	}

	qm_gfx_texture_set_wrap_mode( texture->internal, texture->wrapMode );
	qm_gfx_texture_set_filter( texture->internal, texture->filterMode );

	ape_memory_setup_reference( texture->path, APE_CACHE_POOL_TEXTURES, &texture->reference, destroy_texture, nullptr );
	ape_memory_reference_add( &texture->reference );

	return texture;

cleanup:
	destroy_texture( texture );

	return useFallback ? defaultTextures[ APE_TEXTURE_FALLBACK ] : nullptr;
}

ApeTexture *ape_texture_cache_cubemap_( char **paths, const QmGfxTextureFilter filter )
{
	ApeTexture *texture = QM_OS_MEMORY_NEW( ApeTexture );
	if ( texture == nullptr )
	{
		return nullptr;
	}

	texture->internal = qm_gfx_texture_create( QM_GFX_TEXTURE_TYPE_CUBEMAP );
	if ( texture->internal == nullptr )
	{
		ape_console_warning_( "Failed to allocate internal texture: %s\n", PlGetError() );
		goto cleanup;
	}

	qm_gfx_texture_set_filter( texture->internal, filter );
	qm_gfx_texture_set_wrap_mode( texture->internal, PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE );

	// need to make sure they're all consistent
	unsigned int w = 0;
	unsigned int h = 0;

	// attempt to load all the faces in
	QmImage *images[ QM_GFX_TEXTURE_MAX_CUBEMAP_FACES ] = {};
	for ( unsigned int i = 0; i < QM_GFX_TEXTURE_MAX_CUBEMAP_FACES; ++i )
	{
		images[ i ] = qm_image_load( paths[ i ] );
		if ( images[ i ] == nullptr )
		{
			ape_console_warning_( "Failed to load image for cubemap (%s): %s\n", paths[ i ], PlGetError() );
			goto cleanup;
		}

		if ( w == 0 )
		{
			w = images[ i ]->width;
		}
		else if ( images[ i ]->width != w )
		{
			ape_console_warning_( "Incorrect cubemap slot width (%u) (%u != %u)!\n", i, images[ i ]->width, w );
			goto cleanup;
		}

		if ( h == 0 )
		{
			h = images[ i ]->height;
		}
		else if ( images[ i ]->height != h )
		{
			ape_console_warning_( "Incorrect cubemap slot height (%u) (%u != %u)!\n", i, images[ i ]->height, h );
			goto cleanup;
		}

		qm_gfx_texture_upload( texture->internal, images[ i ] );

		PlDestroyImage( images[ i ] );
		images[ i ] = nullptr;
	}

	return texture;

cleanup:
	for ( unsigned int i = 0; i < QM_GFX_TEXTURE_MAX_CUBEMAP_FACES; ++i )
	{
		if ( images[ i ] == nullptr )
		{
			continue;
		}

		PlDestroyImage( images[ i ] );
	}

	destroy_texture( texture );

	return nullptr;
}

ApeTexture *ape_get_default_texture_( ApeDefaultTexture defaultTexture )
{
	return defaultTextures[ defaultTexture ];
}
