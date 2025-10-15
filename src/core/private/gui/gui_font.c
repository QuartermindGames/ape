// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_hashtable.h>
#include <plcore/pl_array_vector.h>

#include "ape_private.h"

#include "gui_private.h"
#include "common_format_fnt.h"

#include "renderer/renderer.h"
#include "renderer/material/material.h"

/****************************************
 * GUI BITMAP FONT API
 ****************************************/

typedef struct ApeGuiFont
{
	PLGTexture *texture;

	uint32_t      numGlyphs;
	ComFontGlyph *glyphs;
	PLHashTable  *glyphTable;

	float lineSpacing;
	float tabWidth;

	PLGMesh *mesh;
} ApeGuiFont;

static PLVectorArray *cachedFonts;
static PLHashTable   *cachedFontsTable;
static ApeGuiFont    *defaultFonts[ GUI_MAX_FONT_DEFAULTS ];

static float          fontSlant        = 0.0f;
static QmMathVector2f fontShadowOffset = QM_MATH_VECTOR2F( 1.0f, 1.0f );

static uint32_t decode_utf8_char( const char **string )
{
	uint32_t c = 0;
	if ( ( **string & 0x80 ) == 0 )
	{
		c = ( uint32_t ) *( *string )++;
	}
	else if ( ( **string & 0xE0 ) == 0xC0 )
	{
		c = ( *( *string )++ & 0x1F ) << 6;
		if ( ( **string & 0xC0 ) == 0x80 )
		{
			c |= *( *string )++ & 0x3F;
		}
	}
	else if ( ( **string & 0xF0 ) == 0xE0 )
	{
		c = ( *( *string )++ & 0x0F ) << 12;
		if ( ( **string & 0xC0 ) == 0x80 )
		{
			c |= ( *( *string )++ & 0x3F ) << 6;
			if ( ( **string & 0xC0 ) == 0x80 )
			{
				c |= *( *string )++ & 0x3F;
			}
		}
	}
	else if ( ( **string & 0xF8 ) == 0xF0 )
	{
		c = ( *( *string )++ & 0x07 ) << 18;
		if ( ( **string & 0xC0 ) == 0x80 )
		{
			c |= ( *( *string )++ & 0x3F ) << 12;
			if ( ( **string & 0xC0 ) == 0x80 )
			{
				c |= ( *( *string )++ & 0x3F ) << 6;
				if ( ( **string & 0xC0 ) == 0x80 )
				{
					c |= *( *string )++ & 0x3F;
				}
			}
		}
	}

	return c;
}

float gui_font_get_line_spacing( const ApeGuiFont *font ) { return font->lineSpacing; }

ApeGuiFont *gui_get_default_font( GuiFontDefaultType defaultType )
{
	return defaultFonts[ defaultType ];
}

void ape_gui_font_destroy( ApeGuiFont *font )
{
	PlDestroyHashTable( font->glyphTable );
	PlgDestroyTexture( font->texture );
	PlgDestroyMesh( font->mesh );
	qm_os_memory_free( font->glyphs );
	qm_os_memory_free( font );
}

static ApeGuiFont *font_deserialize( PLFile *file )
{
	uint32_t magic = PL_READUINT32( file, false, NULL );
	if ( magic != COM_FORMAT_FONT_MAGIC )
	{
		ape_console_warning_( "Invalid font file!\n" );
		return nullptr;
	}

	uint16_t version = PL_READUINT16( file, false, NULL );
	assert( version <= COM_FORMAT_FONT_VERSION );
	if ( version > COM_FORMAT_FONT_VERSION )
	{
		ape_console_warning_( "Unsupported font version (%u)!\n", version );
		return nullptr;
	}

	uint32_t numGlyphs = PL_READUINT32( file, false, NULL );
	if ( numGlyphs == 0 )
	{
		ape_console_warning_( "Empty font file!\n" );
		return nullptr;
	}

	ApeGuiFont *font = QM_OS_MEMORY_NEW( ApeGuiFont );
	font->glyphTable = PlCreateHashTable();
	font->glyphs     = QM_OS_MEMORY_NEW_( ComFontGlyph, numGlyphs );
	for ( uint32_t i = 0; i < numGlyphs; ++i )
	{
		font->glyphs[ i ].codepoint = PL_READUINT32( file, false, NULL );
		font->glyphs[ i ].x         = PL_READUINT16( file, false, NULL );
		font->glyphs[ i ].y         = PL_READUINT16( file, false, NULL );
		font->glyphs[ i ].w         = PL_READUINT16( file, false, NULL );
		font->glyphs[ i ].h         = PL_READUINT16( file, false, NULL );

		if ( version >= 2 )
		{
			font->glyphs[ i ].rect.x = PlReadInt16( file, false, nullptr );
			font->glyphs[ i ].rect.y = PlReadInt16( file, false, nullptr );
			font->glyphs[ i ].rect.z = PL_READUINT16( file, false, nullptr );
			font->glyphs[ i ].rect.w = PL_READUINT16( file, false, nullptr );
		}
		else
		{
			font->glyphs[ i ].rect.z = font->glyphs[ i ].w;
			font->glyphs[ i ].rect.w = font->glyphs[ i ].h;
		}

		PlInsertHashTableNode( font->glyphTable, &font->glyphs[ i ].codepoint, sizeof( uint32_t ), &font->glyphs[ i ] );

		// for now, just determine line spacing and tab width based on the w/h of a space...
		if ( font->glyphs[ i ].codepoint == ' ' )
		{
			font->lineSpacing = font->glyphs[ i ].h;
			font->tabWidth    = font->glyphs[ i ].w;
		}
	}

	uint16_t bitmapW = PL_READUINT16( file, false, NULL );
	uint16_t bitmapH = PL_READUINT16( file, false, NULL );
	assert( bitmapW != 0 && bitmapH != 0 );
	if ( bitmapW == 0 || bitmapH == 0 )
	{
		ape_gui_font_destroy( font );
		ape_console_warning_( "Invalid bitmap size for font!\n" );
		return nullptr;
	}

	size_t   bitmapSize  = bitmapW * bitmapH;
	PLImage *bitmapImage = PlCreateImage( NULL, bitmapW, bitmapH, 0, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_R8 );
	if ( PlReadFile( file, PlGetImageData( bitmapImage, 0, 0 ), sizeof( uint8_t ), bitmapSize ) != bitmapSize )
	{
		ape_gui_font_destroy( font );
		ape_console_warning_( "Failed to load entirity of bitmap image from font!\n" );
		return nullptr;
	}

	font->texture         = PlgCreateTexture();
	font->texture->filter = PLG_TEXTURE_FILTER_LINEAR;
	if ( !PlgUploadTextureImage( font->texture, bitmapImage ) )
	{
		ape_gui_font_destroy( font );
		ape_console_warning_( "Failed to upload texture data for font!\n" );
		return nullptr;
	}

	PlDestroyImage( bitmapImage );

	font->mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 32, 32 );

	return font;
}

ApeGuiFont *gui_font_load( const char *path, ApeGuiFont *fallback )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		ape_console_warning_( "Failed to load font: %s\n", PlGetError() );
		return fallback;
	}

	ApeGuiFont *font = font_deserialize( file );

	PlCloseFile( file );

	return font;
}

bool ape_gui_initialize_fonts_( void )
{
	cachedFonts      = PlCreateVectorArray( GUI_MAX_FONT_DEFAULTS );
	cachedFontsTable = PlCreateHashTable();

	static const char *fontPaths[ GUI_MAX_FONT_DEFAULTS ] = {
	        [GUI_FONT_DEFAULT_LARGE]  = "guis/fonts/noto_mono_24.fnt",
	        [GUI_FONT_DEFAULT_MEDIUM] = "guis/fonts/noto_mono_12.fnt",
	        [GUI_FONT_DEFAULT_SMALL]  = "guis/fonts/proggysmalltt_12.fnt",
	        [GUI_FONT_DEFAULT_TINY]   = "guis/fonts/proggytinytt_12.fnt",
	};
	for ( uint32_t i = 0; i < GUI_MAX_FONT_DEFAULTS; ++i )
	{
		defaultFonts[ i ] = gui_font_load( fontPaths[ i ], nullptr );
		assert( defaultFonts[ i ] != NULL );
		if ( defaultFonts[ i ] == NULL )
		{
			ape_console_warning_( "Failed to load default font (%s)!\n", fontPaths[ i ] );
			return false;
		}
	}

	return true;
}

void ape_gui_font_get_character_pixel_size( const ApeGuiFont *font, const float scale, const uint32_t character, float *dw, float *dh )
{
	const ComFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &character, sizeof( uint32_t ) );
	if ( glyph == NULL )
	{
		if ( dw != NULL ) { *dw = 0.0f; }
		if ( dh != NULL ) { *dh = 0.0f; }
		return;
	}

	if ( dw != NULL ) { *dw = ( float ) glyph->w * scale; }
	if ( dh != NULL ) { *dh = ( float ) glyph->h * scale; }
}

float ape_gui_font_get_character_pixel_width( const ApeGuiFont *font, const float scale, const uint32_t character )
{
	float w;
	ape_gui_font_get_character_pixel_size( font, scale, character, &w, nullptr );
	return w;
}

void gui_font_set_slant( float slant )
{
	fontSlant = slant;
}

void gui_font_set_shadow_offset( float x, float y )
{
	fontShadowOffset.x = x;
	fontShadowOffset.y = y;
}

void gui_font_get_string_pixel_size( const ApeGuiFont *self, float scale, const char *string, size_t length, float *dw, float *dh )
{
	float w = 0;
	float h = 0;

	const char *end = string + length;
	while ( string < end )
	{
		uint32_t c = decode_utf8_char( &string );
		if ( c == '\0' )
		{
			break;
		}
		if ( c == '\n' )
		{
			h += ( self->lineSpacing * scale );
			continue;
		}
		if ( c == '\t' )
		{
			w += ( self->tabWidth * scale ) * 4.0f;
			continue;
		}

		const ComFontGlyph *glyph = PlLookupHashTableUserData( self->glyphTable, &c, sizeof( uint32_t ) );
		if ( glyph == nullptr )
		{
			continue;
		}

		w += ( float ) ( glyph->rect.z - glyph->rect.x ) * scale;
	}

	if ( dw != NULL ) { *dw = w; }
	if ( dh != NULL ) { *dh = h; }
}

void gui_font_draw_glyph( const ApeGuiFont *font, float x, float y, float scale, const QmMathColour4ub *colour, const ComFontGlyph *glyph )
{
	float tw = ( float ) glyph->w / ( float ) font->texture->w;
	float th = ( float ) glyph->h / ( float ) font->texture->h;
	float tx = ( float ) glyph->x / ( float ) font->texture->w;
	float ty = ( float ) glyph->y / ( float ) font->texture->h;

	//HACK: round it, to stop shit looking blurry...
	x = roundf( x );
	y = roundf( y );

	unsigned int vX = PlgAddMeshVertex( font->mesh, &QM_MATH_VECTOR3F( x + fontSlant, y, 0 ), &pl_vecOrigin3, colour, &QM_MATH_VECTOR2F( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( font->mesh, &QM_MATH_VECTOR3F( x, y + ( float ) glyph->h * scale, 0 ), &pl_vecOrigin3, colour, &QM_MATH_VECTOR2F( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( font->mesh, &QM_MATH_VECTOR3F( x + ( float ) glyph->w * scale + fontSlant, y, 0 ), &pl_vecOrigin3, colour, &QM_MATH_VECTOR2F( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( font->mesh, &QM_MATH_VECTOR3F( x + ( float ) glyph->w * scale, y + ( ( float ) glyph->h * scale ), 0 ), &pl_vecOrigin3, colour, &QM_MATH_VECTOR2F( tx + tw, ty + th ) );

	PlgAddMeshTriangle( font->mesh, vX, vY, vZ );
	PlgAddMeshTriangle( font->mesh, vZ, vY, vW );
}

void gui_font_draw_character( const ApeGuiFont *font, float x, float y, float scale, const QmMathColour4ub *colour, uint32_t character )
{
	ComFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &character, sizeof( uint32_t ) );
	if ( glyph == NULL )
	{
		return;
	}

	gui_font_draw_glyph( font, x, y, scale, colour, glyph );
}

static unsigned char hex_to_num( char c )
{
	if ( c >= '0' && c <= '9' ) return c - '0';
	if ( c >= 'a' && c <= 'f' ) return 10 + ( c - 'a' );
	if ( c >= 'A' && c <= 'F' ) return 10 + ( c - 'A' );
	return 0;
}

static void parse_hex_component( const char **str, const char *end, unsigned char *dst )
{
	if ( *str + 1 < end && isxdigit( ( *str )[ 0 ] ) && isxdigit( ( *str )[ 1 ] ) )
	{
		*dst = ( hex_to_num( ( *str )[ 0 ] ) << 4 ) | hex_to_num( ( *str )[ 1 ] );
		*str += 2;
	}
	else
	{
		*dst = 0;
	}
}

void gui_font_draw_string( const ApeGuiFont *self, float x, float y, float *ox, float *oy, float scale, const QmMathColour4ub *colour, const char *string, size_t length, bool shadow )
{
	float nx = x;
	float ny = y;

	QmMathColour4ub currentColour = *colour;

	const char *end = string + length;
	while ( string < end )
	{
		if ( *string == '$' && *( string + 1 ) == 'c' )
		{
			string += 2;

			parse_hex_component( &string, end, &currentColour.r );
			parse_hex_component( &string, end, &currentColour.g );
			parse_hex_component( &string, end, &currentColour.b );
			parse_hex_component( &string, end, &currentColour.a );
			continue;
		}

		uint32_t c = decode_utf8_char( &string );
		if ( c == '\0' )
		{
			break;
		}
		if ( c == '\n' )
		{
			ny += self->lineSpacing * scale;
			nx = x;
			continue;
		}
		if ( c == '\t' )
		{
			nx += self->tabWidth * scale * 4.0f;
			continue;
		}

		const ComFontGlyph *glyph = PlLookupHashTableUserData( self->glyphTable, &c, sizeof( uint32_t ) );
		if ( glyph == nullptr )
		{
			continue;
		}

		if ( shadow )
		{
			gui_font_draw_glyph( self, nx + fontShadowOffset.x, ny + fontShadowOffset.y, scale, &QM_MATH_COLOUR4UB( 0, 0, 0, currentColour.a ), glyph );
		}

		gui_font_draw_glyph( self, nx, ny, scale, &currentColour, glyph );
		nx += ( float ) ( glyph->rect.z - glyph->rect.x ) * scale;
	}

	if ( ox != NULL ) *ox = nx;
	if ( oy != NULL ) *oy = ny;
}

void gui_font_display( ApeGuiFont *font )
{
	//TODO: update this to use the material system instead!

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	ApeShaderProgram *program = ape_get_default_shader( APE_SHADER_DEFAULT_FONT );
	PlgSetShaderProgram( program->internal );
	PlgSetShaderUniformValue( program->internal, "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );
	PlgSetShaderUniformValue( program->internal, "pl_texture", PlGetMatrix( PL_TEXTURE_MATRIX ), false );

	PlgSetBlendMode( PLG_BLEND_DEFAULT );

	PlgSetTexture( font->texture, 0 );

	PlgUploadMesh( font->mesh );
	PlgDrawMesh( font->mesh );

	ape_rendererPerformance_.numTriangles += font->mesh->num_triangles;
	ape_rendererPerformance_.numBatches++;

	PlPopMatrix();

	PlgClearMesh( font->mesh );

	PlgSetShaderProgram( nullptr );

	PlgSetBlendMode( PLG_BLEND_DISABLE );
}
