// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_hashtable.h>
#include <plcore/pl_array_vector.h>

#include "../../core_private.h"

#include "gui_private.h"
#include "common_format_fnt.h"
#include "client/renderer/renderer_material.h"
#include "client/renderer/renderer.h"

/****************************************
 * GUI BITMAP FONT API
 ****************************************/

typedef struct GUIFont
{
	PLGTexture *texture;

	uint32_t numGlyphs;
	OSWFontGlyph *glyphs;
	PLHashTable *glyphTable;

	float lineSpacing;
	float tabWidth;

	PLGMesh *mesh;
} GUIFont;

static PLVectorArray *cachedFonts;
static PLHashTable *cachedFontsTable;
static GUIFont *defaultFonts[ GUI_MAX_FONT_DEFAULTS ];

float GUI_Font_GetLineSpacing( const GUIFont *font ) { return font->lineSpacing; }

GUIFont *GUI_Font_GetDefault( GUIFontDefaultType defaultType )
{
	return defaultFonts[ defaultType ];
}

void GUI_Font_Destroy( GUIFont *font )
{
	PlDestroyHashTable( font->glyphTable );
	PlgDestroyTexture( font->texture );
	PlgDestroyMesh( font->mesh );
	PL_DELETE( font->glyphs );
	PL_DELETE( font );
}

GUIFont *GUI_Font_Deserialize( PLFile *file )
{
	uint32_t magic = PL_READUINT32( file, false, NULL );
	assert( magic == OSW_FONT_MAGIC );
	if ( magic != OSW_FONT_MAGIC )
	{
		GUI_Warning( "Invalid font file!\n" );
		return NULL;
	}

	uint16_t version = PL_READUINT16( file, false, NULL );
	assert( version <= OSW_FONT_VERSION );
	if ( version > OSW_FONT_VERSION )
	{
		GUI_Warning( "Unsupported font version (%u)!\n", version );
		return NULL;
	}

	uint32_t numGlyphs = PL_READUINT32( file, false, NULL );
	assert( numGlyphs != 0 );
	if ( numGlyphs == 0 )
	{
		GUI_Warning( "Empty font file!\n" );
		return NULL;
	}

	GUIFont *font    = PL_NEW( GUIFont );
	font->glyphTable = PlCreateHashTable();
	font->glyphs     = PL_NEW_( OSWFontGlyph, numGlyphs );
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
		GUI_Font_Destroy( font );
		GUI_Warning( "Invalid bitmap size for font!\n" );
		return NULL;
	}

	size_t bitmapSize    = bitmapW * bitmapH;
	PLImage *bitmapImage = PlCreateImage( NULL, bitmapW, bitmapH, 0, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_R8 );
	if ( PlReadFile( file, PlGetImageData( bitmapImage, 0, 0 ), sizeof( uint8_t ), bitmapSize ) != bitmapSize )
	{
		GUI_Font_Destroy( font );
		GUI_Warning( "Failed to load entirity of bitmap image from font!\n" );
		return NULL;
	}

	font->texture = PlgCreateTexture();
	if ( !PlgUploadTextureImage( font->texture, bitmapImage ) )
	{
		GUI_Font_Destroy( font );
		GUI_Warning( "Failed to upload texture data for font!\n" );
		return NULL;
	}

	PlDestroyImage( bitmapImage );

	font->mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 32, 32 );

	return font;
}

GUIFont *GUI_Font_LoadFile( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		GUI_Warning( "Failed to load font: %s\n", PlGetError() );
		return NULL;
	}

	GUIFont *font = GUI_Font_Deserialize( file );

	PlCloseFile( file );

	return font;
}

bool GUI_Font_Initialize( void )
{
	cachedFonts      = PlCreateVectorArray( GUI_MAX_FONT_DEFAULTS );
	cachedFontsTable = PlCreateHashTable();

	static const char *fontPaths[ GUI_MAX_FONT_DEFAULTS ] = {
	        [GUI_FONT_DEFAULT_LARGE]  = "guis/fonts/Monospace 12.fnt",
	        [GUI_FONT_DEFAULT_MEDIUM] = "guis/fonts/Monospace 12.fnt",
	        [GUI_FONT_DEFAULT_SMALL]  = "guis/fonts/Liberation Mono 9.fnt",
	        [GUI_FONT_DEFAULT_TINY]   = "guis/fonts/CozetteVector 9.fnt",
	};
	for ( uint32_t i = 0; i < GUI_MAX_FONT_DEFAULTS; ++i )
	{
		defaultFonts[ i ] = GUI_Font_LoadFile( fontPaths[ i ] );
		assert( defaultFonts[ i ] != NULL );
		if ( defaultFonts[ i ] == NULL )
		{
			GUI_Error( "Failed to load default font (%s)!\n", fontPaths[ i ] );
			return false;
		}
	}

	return true;
}

void GUI_Font_GetCharacterPixelSize( const GUIFont *font, float scale, uint32_t character, float *dw, float *dh )
{
	const OSWFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &character, sizeof( uint32_t ) );
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

float GUI_Font_GetCharacterPixelWidth( const GUIFont *font, float scale, uint32_t character )
{
	float w;
	GUI_Font_GetCharacterPixelSize( font, scale, character, &w, NULL );
	return w;
}

void GUI_Font_GetStringPixelSize( const GUIFont *font, float scale, const char *string, size_t length, float *dw, float *dh )
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

		const OSWFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &c, sizeof( uint32_t ) );
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

void GUI_Font_DrawGlyph( const GUIFont *font, float x, float y, float scale, const PLColour *colour, const OSWFontGlyph *glyph )
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

void GUI_Font_DrawCharacter( const GUIFont *font, float x, float y, float scale, const PLColour *colour, uint32_t character )
{
	OSWFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &character, sizeof( uint32_t ) );
	if ( glyph == NULL )
	{
		return;
	}

	GUI_Font_DrawGlyph( font, x, y, scale, colour, glyph );
}

void GUI_Font_DrawString( const GUIFont *font, float x, float y, float *ox, float *oy, float scale, const PLColour *colour, const char *string, size_t length, bool shadow )
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

		const OSWFontGlyph *glyph = PlLookupHashTableUserData( font->glyphTable, &c, sizeof( uint32_t ) );
		if ( glyph == NULL )
		{
			continue;
		}

		if ( shadow )
		{
			GUI_Font_DrawGlyph( font, nx + 2, ny + 2, scale, &PLColourRGB( 0, 0, 0 ), glyph );
		}

		GUI_Font_DrawGlyph( font, nx, ny, scale, colour, glyph );

		nx += ( ( float ) glyph->w ) * scale;
	}

	if ( ox != NULL ) *ox = nx;
	if ( oy != NULL ) *oy = ny;
}

void GUI_Font_Display( GUIFont *font )
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

	ape_RendererPerformance_.numTriangles += font->mesh->num_triangles;
	ape_RendererPerformance_.numBatches++;

	PlPopMatrix();

	PlgClearMesh( font->mesh );

	PlgSetShaderProgram( NULL );
	PlgSetBlendMode( PLG_BLEND_DISABLE );
}
