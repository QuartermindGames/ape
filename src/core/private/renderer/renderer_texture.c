// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "plcore/pl_hashtable.h"
#include "qmos/public/qm_os_string.h"

#include "yin/core_fs.h"

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

static void compute_average_colour( ApeTexture *texture, QmImage *image )
{
	if ( !ape_editor_is_active() )
	{
		return;
	}

	const PLImageFormat format = PlGetImageFormat( image );
	if ( format != PL_IMAGEFORMAT_RGB8 && format != PL_IMAGEFORMAT_RGBA8 )
	{
		return;
	}

	// for the sake of speed, when determining the average colour,
	// we're only going over parts of the image rather than the entire thing.
	// this obviously isn't accurate but it's good enough for our needs.

	const unsigned int w     = qm_image_get_width( image );
	const unsigned int h     = qm_image_get_height( image );
	const unsigned int hw    = w / 2;
	const unsigned int hh    = h / 2;
	const unsigned int hsize = hw * hh;
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

	uint8_t *p = PlGetImageData( image, 0, 0 );
	for ( unsigned int i = 0; i < hsize; ++i, p += stride )
	{
		if ( format == PL_IMAGEFORMAT_RGB8 )
		{
			const QmMathColour3ub *pixel = ( QmMathColour3ub * ) p;
			out.r += pixel->r;
			out.g += pixel->g;
			out.b += pixel->b;
		}
		else
		{
			const QmMathColour4ub *pixel = ( QmMathColour4ub * ) p;
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

	QmImage *image = PlCreateImage( data, w, h, 0, cFormat, iFormat );
	if ( image == nullptr )
	{
		ape_console_warning_( "Failed to generate image (%s) data: %s\n", id, PlGetError() );
	}

#if 0
    char outName[ 64 ];
	snprintf( outName, sizeof( outName ), "test_%dx%d-%d.png", w, h, numChannels );
	plWriteImage( image, outName );
#endif

	QmGfxTexture *internalTexture = qm_gfx_texture_create( QM_GFX_TEXTURE_TYPE_2D );
	if ( internalTexture == nullptr )
	{
		ape_console_error_( true, "Failed to create texture (%s): %s\n", id, PlGetError() );
		return nullptr;
	}

	internalTexture->filter = filter;

	if ( !qm_gfx_texture_upload( internalTexture, image ) )
	{
		ape_console_error_( true, "Failed to generate texture from image (%s): %s\n", id, PlGetError() );
	}

	ApeTexture *texture = QM_OS_MEMORY_NEW( ApeTexture );
	texture->internal   = internalTexture;

	compute_average_colour( texture, image );

	// no point retaining this for generated images
	PlDestroyImage( image );

	ape_memory_setup_reference( id, APE_CACHE_POOL_TEXTURES, &texture->reference, destroy_texture, NULL );

	return texture;
}

static void fetch_texture_config( const char *path,
                                  // if this is ever expanded, revise this
                                  QmGfxTextureWrapMode *dstWrap,
                                  QmGfxTextureFilter   *dstFilter )
{
	PLPath configPath;
	PlSetupPath( configPath, "%s", path );
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
		*dstWrap = PLG_TEXTURE_WRAP_MODE_REPEAT;
	}
	else if ( strcmp( wrapMode, "mirrored_repeat" ) == 0 )
	{
		*dstWrap = PLG_TEXTURE_WRAP_MODE_MIRRORED_REPEAT;
	}
	else if ( strcmp( wrapMode, "clamp" ) == 0 )
	{
		*dstWrap = PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE;
	}
	else if ( strcmp( wrapMode, "clamp_border" ) == 0 )
	{
		*dstWrap = PLG_TEXTURE_WRAP_MODE_CLAMP_BORDER;
	}

	const char *filterMode = acm_get_string( root, "filterMode", "linear" );
	if ( strcmp( filterMode, "mipmap_linear" ) == 0 )
	{
		*dstFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;
	}
	else if ( strcmp( filterMode, "linear" ) == 0 )
	{
		*dstFilter = PLG_TEXTURE_FILTER_LINEAR;
	}
	else if ( strcmp( filterMode, "mipmap_nearest" ) == 0 )
	{
		*dstFilter = PLG_TEXTURE_FILTER_MIPMAP_NEAREST;
	}
	else if ( strcmp( filterMode, "nearest" ) == 0 )
	{
		*dstFilter = PLG_TEXTURE_FILTER_NEAREST;
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

static QmGfxTexture *create_internal_texture( const QmGfxTextureType type, QmGfxTextureFilter filter, const char *path, const QmImage *image )
{
	QmGfxTexture *texture = qm_gfx_texture_create( type );
	if ( texture == nullptr )
	{
		ape_console_warning_( "Failed to allocate internal texture (%s): %s\n", path, PlGetError() );
		return nullptr;
	}

	QmGfxTextureWrapMode wrap = PLG_TEXTURE_WRAP_MODE_REPEAT;
	fetch_texture_config( path, &wrap, &filter );

	qm_gfx_texture_set_wrap_mode( texture, wrap );
	qm_gfx_texture_set_filter( texture, filter );

	// upload it
	if ( !qm_gfx_texture_upload( texture, image ) )
	{
		ape_console_warning_( "Failed to upload texture (%s): %s\n", path, PlGetError() );
		qm_os_memory_free( texture );
		texture = nullptr;
	}

	return texture;
}

ApeTexture *ape_texture_cache_( const char *path, QmGfxTextureFilter filter, bool useFallback )
{
	ApeTexture *texture = PlLookupHashTableUserData( textureTable, path, strlen( path ) );
	if ( texture != NULL )
	{
		ape_memory_reference_add( &texture->reference );
		return texture;
	}

	QmGfxTexture *internal = nullptr;
	QmImage      *image    = qm_image_load( path );
	if ( image == nullptr )
	{
		ape_console_warning_( "Failed to load image (%s): %s\n", path, PlGetError() );
		goto cleanup;
	}

	if ( ( internal = create_internal_texture( QM_GFX_TEXTURE_TYPE_2D, filter, path, image ) ) == nullptr )
	{
		ape_console_warning_( "Failed to allocate internal texture (%s): %s\n", path, PlGetError() );
		goto cleanup;
	}

	texture           = QM_OS_MEMORY_NEW( ApeTexture );
	texture->internal = internal;
	texture->path     = qm_os_string_alloc( "%s", path );
#ifdef APE_SUPPORT_EDITOR
	texture->modifiedTime = ape_fs_get_timestamp( path );
#endif

	compute_average_colour( texture, image );

	if ( ape_editor_is_active() )
	{
		texture->image = image;
	}
	else
	{
		PlDestroyImage( image );
	}

	ape_memory_setup_reference( texture->path, APE_CACHE_POOL_TEXTURES, &texture->reference, destroy_texture, nullptr );
	ape_memory_reference_add( &texture->reference );

	return texture;

cleanup:
	PlDestroyImage( image );
	qm_os_memory_free( internal );

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

#ifdef APE_SUPPORT_EDITOR

void ape_texture_reload_( ApeTexture *self )
{
	//TODO: this isn't going to work for cubemaps
	//		for now, we'll deal with that later

	if ( self->path == nullptr )
	{
		return;
	}

	const time_t now = ape_fs_get_timestamp( self->path );
	if ( now == self->modifiedTime )
	{
		return;
	}

	// update the timestamp anyway, even if it fails, I guess
	// in theory this'll stop us from failing over and over -
	// as the odds are if it failed to load this time, it'll
	// still fail the next time :(
	self->modifiedTime = ape_fs_get_timestamp( self->path );

	QmImage *image = qm_image_load( self->path );
	if ( image == nullptr )
	{
		ape_console_warning_( "Failed to load image (%s): %s\n", self->path, PlGetError() );
		return;
	}

	QmGfxTexture *internal;
	if ( ( internal = create_internal_texture( self->internal->type,
	                                           self->internal->filter,
	                                           self->path,
	                                           image ) ) == nullptr )
	{
		PlDestroyImage( image );
		return;
	}

	// swap out to the new one
	qm_os_memory_free( self->internal );
	self->internal = internal;

	compute_average_colour( self, image );

	if ( ape_editor_is_active() )
	{
		self->image = image;
	}
	else
	{
		PlDestroyImage( image );
	}

	ape_console_print_( "Reloaded texture: %s\n", self->path );
}

#endif

ApeTexture *ape_get_default_texture_( ApeDefaultTexture defaultTexture )
{
	return defaultTextures[ defaultTexture ];
}
