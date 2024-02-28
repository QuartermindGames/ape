// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_hashtable.h>
#include <plcore/pl_array_vector.h>

#include "../../ape_private.h"

#include "gui_private.h"
#include "common_format_fnt.h"
#include "client/renderer/renderer_material.h"
#include "client/renderer/renderer.h"

/****************************************
 * GUI BITMAP FONT API
 ****************************************/

typedef struct GuiFont
{
	PLGTexture *texture;

	uint32_t numGlyphs;
	ComFontGlyph *glyphs;
	PLHashTable *glyphTable;

	float lineSpacing;
	float tabWidth;

	PLGMesh *mesh;
} GuiFont;

static PLVectorArray *cachedFonts;
static PLHashTable *cachedFontsTable;
static GuiFont *defaultFonts[ GUI_MAX_FONT_DEFAULTS ];

float guiGetFontLineSpacing( const GuiFont *font ) { return font->lineSpacing; }

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

GuiFont *guiDeserializeFont( PLFile *file )
{
	uint32_t magic = PL_READUINT32( file, false, NULL );
	assert( magic == COM_FORMAT_FONT_MAGIC );
	if ( magic != COM_FORMAT_FONT_MAGIC )
	{
		GUI_WARNING( "Invalid font file!\n" );
		return NULL;
	}

	uint16_t version = PL_READUINT16( file, false, NULL );
	assert( version <= COM_FORMAT_FONT_VERSION );
	if ( version > COM_FORMAT_FONT_VERSION )
	{
		GUI_WARNING( "Unsupported font version (%u)!\n", version );
		return NULL;
	}

	uint32_t numGlyphs = PL_READUINT32( file, false, NULL );
	assert( numGlyphs != 0 );
	if ( numGlyphs == 0 )
	{
		GUI_WARNING( "Empty font file!\n" );
		return NULL;
	}

	GuiFont *font = PL_NEW( GuiFont );
	font->glyphTable = PlCreateHashTable();
	font->glyphs = PL_NEW_( ComFontGlyph, numGlyphs );
	for ( uint32_t i = 0; i < numGlyphs; ++i )
	{
		font->glyphs[ i ].codepoint = PL_READUINT32( file, false, NULL );
		font->glyphs[ i ].x = PL_READUINT16( file, false, NULL );
		font->glyphs[ i ].y = PL_READUINT16( file, false, NULL );
		font->glyphs[ i ].w = PL_READUINT16( file, false, NULL );
		font->glyphs[ i ].h = PL_READUINT16( file, false, NULL );
		PlInsertHashTableNode( font->glyphTable, &font->glyphs[ i ].codepoint, sizeof( uint32_t ), &font->glyphs[ i ] );

		// for now, just determine line spacing and tab width based on the w/h of a space...
		if ( font->glyphs[ i ].codepoint == ' ' )
		{
			font->lineSpacing = font->glyphs[ i ].h;
			font->tabWidth = font->glyphs[ i ].w;
		}
	}

	uint16_t bitmapW = PL_READUINT16( file, false, NULL );
	uint16_t bitmapH = PL_READUINT16( file, false, NULL );
	assert( bitmapW != 0 && bitmapH != 0 );
	if ( bitmapW == 0 || bitmapH == 0 )
	{
		guiDestroyFont( font );
		GUI_WARNING( "Invalid bitmap size for font!\n" );
		return NULL;
	}

	size_t bitmapSize = bitmapW * bitmapH;
	PLImage *bitmapImage = PlCreateImage( NULL, bitmapW, bitmapH, 0, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_R8 );
	if ( PlReadFile( file, PlGetImageData( bitmapImage, 0, 0 ), sizeof( uint8_t ), bitmapSize ) != bitmapSize )
	{
		guiDestroyFont( font );
		GUI_WARNING( "Failed to load entirity of bitmap image from font!\n" );
		return NULL;
	}

	font->texture = PlgCreateTexture();
	if ( !PlgUploadTextureImage( font->texture, bitmapImage ) )
	{
		guiDestroyFont( font );
		GUI_WARNING( "Failed to upload texture data for font!\n" );
		return NULL;
	}

	PlDestroyImage( bitmapImage );

	font->mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 32, 32 );

	return font;
}

GuiFont *guiLoadFontFile( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		GUI_WARNING( "Failed to load font: %s\n", PlGetError() );
		return NULL;
	}

	GuiFont *font = guiDeserializeFont( file );

	PlCloseFile( file );

	return font;
}

bool guiInitializeFonts_( void )
{
	cachedFonts = PlCreateVectorArray( GUI_MAX_FONT_DEFAULTS );
	cachedFontsTable = PlCreateHashTable();

	static const char *fontPaths[ GUI_MAX_FONT_DEFAULTS ] = {
	        [GUI_FONT_DEFAULT_LARGE] = "guis/fonts/Monospace 12.fnt",
	        [GUI_FONT_DEFAULT_MEDIUM] = "guis/fonts/Monospace 12.fnt",
	        [GUI_FONT_DEFAULT_SMALL] = "guis/fonts/Noto Mono 10.fnt",
	        [GUI_FONT_DEFAULT_TINY] = "guis/fonts/Noto Mono 8.fnt",
	};
	for ( uint32_t i = 0; i < GUI_MAX_FONT_DEFAULTS; ++i )
	{
		defaultFonts[ i ] = guiLoadFontFile( fontPaths[ i ] );
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
	assert( glyph != NULL );
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

void guiGetStringPixelSize( const GuiFont *font, float scale, const char *string, size_t length, float *dw, float *dh )
{
	float w = 0;
	float h = 0;
	for ( size_t i = 0; i < length; ++i )
	{
		if ( string[ i ] == '\0' )
		{
			break;
		}
		else if ( string[ i ] == '\n' )
		{
			h += ( font->lineSpacing * scale );
			continue;
		}
		else if ( string[ i ] == '\t' )
		{
			w += ( font->lineSpacing * scale ) * 4.0f;
			continue;
		}

		uint32_t c = ( uint32_t ) string[ i ];

		const ComFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &c, sizeof( uint32_t ) );
		if ( glyph == NULL )
		{
			continue;
		}

		w += ( ( float ) glyph->w ) * scale;
		if ( ( ( float ) glyph->h ) > h )
		{
			h += ( ( float ) glyph->h );
		}
	}

	if ( dw != NULL ) *dw = w;
	if ( dh != NULL ) *dh = h;
}

void guiDrawFontGlyph( const GuiFont *font, float x, float y, float scale, const PLColour *colour, const ComFontGlyph *glyph )
{
	float tw = ( float ) glyph->w / ( float ) font->texture->w;
	float th = ( float ) glyph->h / ( float ) font->texture->h;
	float tx = ( float ) glyph->x / ( float ) font->texture->w;
	float ty = ( float ) glyph->y / ( float ) font->texture->h;

	unsigned int vX = PlgAddMeshVertex( font->mesh, &PLVector3( x, y, 0 ), &pl_vecOrigin3, colour, &PLVector2( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( font->mesh, &PLVector3( x, y + ( ( float ) glyph->h * scale ), 0 ), &pl_vecOrigin3, colour, &PLVector2( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( font->mesh, &PLVector3( x + ( ( float ) glyph->w * scale ), y, 0 ), &pl_vecOrigin3, colour, &PLVector2( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( font->mesh, &PLVector3( x + ( ( float ) glyph->w * scale ), y + ( ( float ) glyph->h * scale ), 0 ), &pl_vecOrigin3, colour, &PLVector2( tx + tw, ty + th ) );

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

	guiDrawFontGlyph( font, x, y, scale, colour, glyph );
}

void gui_font_draw_string( const GuiFont *font, float x, float y, float *ox, float *oy, float scale, const PLColour *colour, const char *string, size_t length, bool shadow )
{
	float nx = x;
	float ny = y;
	for ( size_t i = 0; i < length; ++i )
	{
		if ( string[ i ] == '\0' )
		{
			break;
		}

		if ( string[ i ] == '\n' )
		{
			ny += ( font->lineSpacing * scale );
			nx = x;
			continue;
		}
		else if ( string[ i ] == '\t' )
		{
			nx += ( font->lineSpacing * scale ) * 4.0f;
			continue;
		}

		uint32_t c = ( uint32_t ) string[ i ];

		const ComFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &c, sizeof( uint32_t ) );
		if ( glyph == NULL )
		{
			continue;
		}

		if ( shadow )
		{
			guiDrawFontGlyph( font, nx + 2, ny + 2, scale, &PLColourRGB( 0, 0, 0 ), glyph );
		}

		guiDrawFontGlyph( font, nx, ny, scale, colour, glyph );

		nx += ( ( float ) glyph->w ) * scale;
	}

	if ( ox != NULL ) *ox = nx;
	if ( oy != NULL ) *oy = ny;
}

void gui_font_display( GuiFont *font )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_FONT ] );
	PlgSetShaderUniformValue( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_FONT ], "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );

	PlgSetBlendMode( PLG_BLEND_DEFAULT );

	PlgSetTexture( font->texture, 0 );

	PlgUploadMesh( font->mesh );
	PlgDrawMesh( font->mesh );

	ape_rendererPerformance_.numTriangles += font->mesh->num_triangles;
	ape_rendererPerformance_.numBatches++;

	PlPopMatrix();

	PlgClearMesh( font->mesh );

	PlgSetShaderProgram( NULL );
	PlgSetBlendMode( PLG_BLEND_DISABLE );
}
