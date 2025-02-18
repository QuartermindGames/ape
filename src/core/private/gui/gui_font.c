// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_hashtable.h>
#include <plcore/pl_array_vector.h>

#include "ape_private.h"

#include "gui_private.h"
#include "common_format_fnt.h"
#include "renderer/renderer.h"

/****************************************
 * GUI BITMAP FONT API
 ****************************************/

typedef struct GuiFont
{
	PLGTexture *texture;

	uint32_t      numGlyphs;
	ComFontGlyph *glyphs;
	PLHashTable  *glyphTable;

	float lineSpacing;
	float tabWidth;

	PLGMesh *mesh;
} GuiFont;

static PLVectorArray *cachedFonts;
static PLHashTable   *cachedFontsTable;
static GuiFont       *defaultFonts[ GUI_MAX_FONT_DEFAULTS ];

static float     fontSlant        = 0.0f;
static PLVector2 fontShadowOffset = PL_VECTOR2( 1.0f, 1.0f );

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

float gui_font_get_line_spacing( const GuiFont *font ) { return font->lineSpacing; }

GuiFont *gui_get_default_font( GuiFontDefaultType defaultType )
{
	return defaultFonts[ defaultType ];
}

void guiDestroyFont( GuiFont *font )
{
	PlDestroyHashTable( font->glyphTable );
	PlgDestroyTexture( font->texture );
	PlgDestroyMesh( font->mesh );
	PL_DELETE( font->glyphs );
	PL_DELETE( font );
}

static GuiFont *font_deserialize( PLFile *file )
{
	uint32_t magic = PL_READUINT32( file, false, NULL );
	if ( magic != COM_FORMAT_FONT_MAGIC )
	{
		GUI_WARNING( "Invalid font file!\n" );
		return nullptr;
	}

	uint16_t version = PL_READUINT16( file, false, NULL );
	assert( version <= COM_FORMAT_FONT_VERSION );
	if ( version > COM_FORMAT_FONT_VERSION )
	{
		GUI_WARNING( "Unsupported font version (%u)!\n", version );
		return nullptr;
	}

	uint32_t numGlyphs = PL_READUINT32( file, false, NULL );
	if ( numGlyphs == 0 )
	{
		GUI_WARNING( "Empty font file!\n" );
		return nullptr;
	}

	GuiFont *font    = PL_NEW( GuiFont );
	font->glyphTable = PlCreateHashTable();
	font->glyphs     = PL_NEW_( ComFontGlyph, numGlyphs );
	for ( uint32_t i = 0; i < numGlyphs; ++i )
	{
		font->glyphs[ i ].codepoint = PL_READUINT32( file, false, NULL );
		font->glyphs[ i ].x         = PL_READUINT16( file, false, NULL );
		font->glyphs[ i ].y         = PL_READUINT16( file, false, NULL );
		font->glyphs[ i ].w         = PL_READUINT16( file, false, NULL );
		font->glyphs[ i ].h         = PL_READUINT16( file, false, NULL );
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
		guiDestroyFont( font );
		GUI_WARNING( "Invalid bitmap size for font!\n" );
		return nullptr;
	}

	size_t   bitmapSize  = bitmapW * bitmapH;
	PLImage *bitmapImage = PlCreateImage( NULL, bitmapW, bitmapH, 0, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_R8 );
	if ( PlReadFile( file, PlGetImageData( bitmapImage, 0, 0 ), sizeof( uint8_t ), bitmapSize ) != bitmapSize )
	{
		guiDestroyFont( font );
		GUI_WARNING( "Failed to load entirity of bitmap image from font!\n" );
		return nullptr;
	}

	font->texture         = PlgCreateTexture();
	font->texture->filter = PLG_TEXTURE_FILTER_LINEAR;
	if ( !PlgUploadTextureImage( font->texture, bitmapImage ) )
	{
		guiDestroyFont( font );
		GUI_WARNING( "Failed to upload texture data for font!\n" );
		return nullptr;
	}

	PlDestroyImage( bitmapImage );

	font->mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 32, 32 );

	return font;
}

GuiFont *gui_font_load( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		GUI_WARNING( "Failed to load font: %s\n", PlGetError() );
		return nullptr;
	}

	GuiFont *font = font_deserialize( file );

	PlCloseFile( file );

	return font;
}

bool guiInitializeFonts_( void )
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
		defaultFonts[ i ] = gui_font_load( fontPaths[ i ] );
		assert( defaultFonts[ i ] != NULL );
		if ( defaultFonts[ i ] == NULL )
		{
			GUI_ERROR( "Failed to load default font (%s)!\n", fontPaths[ i ] );
			return false;
		}
	}

	return true;
}

void guiGetCharacterPixelSize( const GuiFont *font, float scale, uint32_t character, float *dw, float *dh )
{
	const ComFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &character, sizeof( uint32_t ) );
	if ( glyph == NULL )
	{
		if ( dw != NULL ) { *dw = 0.0f; }
		if ( dh != NULL ) { *dh = 0.0f; }
		return;
	}

	if ( dw != NULL ) { *dw = glyph->w; }
	if ( dh != NULL ) { *dh = glyph->h; }
}

float guiGetCharacterPixelWidth( const GuiFont *font, float scale, uint32_t character )
{
	float w;
	guiGetCharacterPixelSize( font, scale, character, &w, NULL );
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

void gui_font_get_string_pixel_size( const GuiFont *self, float scale, const char *string, size_t length, float *dw, float *dh )
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
			w += ( self->lineSpacing * scale ) * 4.0f;
			continue;
		}

		const ComFontGlyph *glyph = PlLookupHashTableUserData( self->glyphTable, &c, sizeof( uint32_t ) );
		if ( glyph == nullptr )
		{
			continue;
		}

		w += ( ( float ) glyph->w ) * scale;
	}

	if ( dw != NULL ) { *dw = w; }
	if ( dh != NULL ) { *dh = h; }
}

void gui_font_draw_glyph( const GuiFont *font, float x, float y, float scale, const PLColour *colour, const ComFontGlyph *glyph )
{
	float tw = ( float ) glyph->w / ( float ) font->texture->w;
	float th = ( float ) glyph->h / ( float ) font->texture->h;
	float tx = ( float ) glyph->x / ( float ) font->texture->w;
	float ty = ( float ) glyph->y / ( float ) font->texture->h;

	//HACK: round it, to stop shit looking blurry...
	x = roundf( x );
	y = roundf( y );

	unsigned int vX = PlgAddMeshVertex( font->mesh, &PL_VECTOR3( x + fontSlant, y, 0 ), &pl_vecOrigin3, colour, &PL_VECTOR2( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( font->mesh, &PL_VECTOR3( x, y + ( ( float ) glyph->h * scale ), 0 ), &pl_vecOrigin3, colour, &PL_VECTOR2( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( font->mesh, &PL_VECTOR3( x + ( ( float ) glyph->w * scale ) + fontSlant, y, 0 ), &pl_vecOrigin3, colour, &PL_VECTOR2( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( font->mesh, &PL_VECTOR3( x + ( ( float ) glyph->w * scale ), y + ( ( float ) glyph->h * scale ), 0 ), &pl_vecOrigin3, colour, &PL_VECTOR2( tx + tw, ty + th ) );

	PlgAddMeshTriangle( font->mesh, vX, vY, vZ );
	PlgAddMeshTriangle( font->mesh, vZ, vY, vW );
}

void gui_font_draw_character( const GuiFont *font, float x, float y, float scale, const PLColour *colour, uint32_t character )
{
	ComFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &character, sizeof( uint32_t ) );
	if ( glyph == NULL )
	{
		return;
	}

	gui_font_draw_glyph( font, x, y, scale, colour, glyph );
}

void gui_font_draw_string( const GuiFont *self, float x, float y, float *ox, float *oy, float scale, const PLColour *colour, const char *string, size_t length, bool shadow )
{
	float nx = x;
	float ny = y;

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
			ny += ( self->lineSpacing * scale );
			nx = x;
			continue;
		}

		if ( c == '\t' )
		{
			nx += ( self->lineSpacing * scale ) * 4.0f;
			continue;
		}

		const ComFontGlyph *glyph = PlLookupHashTableUserData( self->glyphTable, &c, sizeof( uint32_t ) );
		if ( glyph == nullptr )
		{
			continue;
		}

		if ( shadow )
		{
			gui_font_draw_glyph( self, nx + fontShadowOffset.x, ny + fontShadowOffset.y, scale, &PLColour( 0, 0, 0, colour->a ), glyph );
		}

		gui_font_draw_glyph( self, nx, ny, scale, colour, glyph );
		nx += ( ( float ) glyph->w ) * scale;
	}

	if ( ox != NULL ) *ox = nx;
	if ( oy != NULL ) *oy = ny;
}

void gui_font_display( GuiFont *font )
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
