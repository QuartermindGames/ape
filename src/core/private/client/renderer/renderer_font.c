// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "renderer_font.h"
#include "renderer.h"

static ApeBitmapFont *defaultFont, *defaultFontSmall;

static void DestroyBitmapFont( void *userData )
{
	ApeBitmapFont *font = userData;
	assert( font != NULL );

	ape_material_release( font->material );

	PlgDestroyMesh( font->mesh );

	PlFree( font );
}

void ss_arl_bitmap_font_batch_character( const ApeBitmapFont *font, float x, float y, float scale, PLColour colour, uint8_t character )
{
	int row = ( character - font->start ) / ( font->w / font->cw );
	int col = ( character - font->start ) % ( font->w / font->cw );

	int cX = col * font->cw;
	int cY = row * font->ch;

	/* figure out the correct coords we need in the font sheet */
	float tw = ( float ) font->cw / ( float ) font->w;
	float th = ( float ) font->ch / ( float ) font->h;
	float tx = ( float ) cX / ( float ) font->w;
	float ty = ( float ) cY / ( float ) font->h;

	unsigned int vX = PlgAddMeshVertex( font->mesh, &PLVector3( x, y, 0 ), &pl_vecOrigin3, &colour, &PLVector2( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( font->mesh, &PLVector3( x, y + ( ( float ) font->ch * scale ), 0 ), &pl_vecOrigin3, &colour, &PLVector2( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( font->mesh, &PLVector3( x + ( ( float ) font->cw * scale ), y, 0 ), &pl_vecOrigin3, &colour, &PLVector2( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( font->mesh, &PLVector3( x + ( ( float ) font->cw * scale ), y + ( ( float ) font->ch * scale ), 0 ), &pl_vecOrigin3, &colour, &PLVector2( tx + tw, ty + th ) );

	PlgAddMeshTriangle( font->mesh, vX, vY, vZ );
	PlgAddMeshTriangle( font->mesh, vZ, vY, vW );
}

void ape_bitmap_font_batch_string( const ApeBitmapFont *font, float x, float y, float scale, PLColour colour, const char *msg, size_t length, bool shadow )
{
	if ( length == 0 )
		return;

	float n_x = x;
	float n_y = y;
	for ( size_t i = 0; i < length; ++i )
	{
		if ( msg[ i ] == '\n' )
		{
			n_y += ( font->ch * scale );
			n_x = x;
			continue;
		}
		else if ( msg[ i ] == '\t' )
		{
			n_x += ( font->cw * scale ) * 4.0f;
			continue;
		}

		if ( shadow )
			ss_arl_bitmap_font_batch_character( font, n_x + 1, n_y + 1, scale, PLColourRGB( 0, 0, 0 ), ( uint8_t ) msg[ i ] );

		ss_arl_bitmap_font_batch_character( font, n_x, n_y, scale, colour, ( uint8_t ) msg[ i ] );

		n_x += ( font->cw * scale );
	}
}

/**
 * Draw a single bitmap character at the specified coordinates.
 */
void ss_arl_bitmap_font_draw_character( ApeBitmapFont *font, float x, float y, float scale, PLColour colour, char character )
{
	if ( scale <= 0 )
		return;

	int w, h;
	PlgGetViewport( NULL, NULL, &w, &h );

	float dw = ( float ) w;
	float dh = ( float ) h;
	if ( x > dw || y > dh )
		return;

	/* setup our render pass */

	ape_bitmap_font_begin_draw( font );

	ss_arl_bitmap_font_batch_character( font, x, y, scale, colour, character );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	ape_material_draw( font->material, font->mesh, NULL, 0 );

	PlPopMatrix();
}

void ape_bitmap_font_draw_string( ApeBitmapFont *font, float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow )
{
	if ( scale == 0.0f )
		return;

	size_t numChars = strlen( msg );
	if ( numChars == 0 )
		return;

	ape_bitmap_font_begin_draw( font );

	if ( shadow )
		ape_bitmap_font_batch_string( font, x + 1, y + 1, scale, PL_COLOUR_BLACK, msg, numChars, false );

	ape_bitmap_font_batch_string( font, x, y, scale, colour, msg, numChars, false );
	ape_bitmap_font_draw( font );
}

void ape_bitmap_font_begin_draw( ApeBitmapFont *font )
{
	PlgClearMesh( font->mesh );
}

void ape_bitmap_font_draw( ApeBitmapFont *font )
{
	//PlMatrixMode( PL_MODELVIEW_MATRIX );
	//PlPushMatrix();

	//PlLoadIdentityMatrix();

	ape_material_draw( font->material, font->mesh, NULL, 0 );

	//PlPopMatrix();
}

void ape_initialize_bitmap_fonts_( void )
{
	defaultFont = ss_arl_bitmap_font_cache( "materials/ui/fonts/default.mat.n", 256, 48, 8, 12, 0, 128 );
	defaultFontSmall = ss_arl_bitmap_font_cache( "materials/ui/fonts/default_small.mat.n", 128, 24, 4, 6, 0, 128 );

	if ( defaultFont == NULL || defaultFontSmall == NULL )
		PRINT_ERROR( "Failed to load default fonts!\n" );
}

void ape_shutdown_bitmap_fonts_( void )
{
	ss_arl_bitmap_font_release( defaultFont );
	defaultFont = NULL;
}

ApeBitmapFont *ss_arl_bitmap_font_cache( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end )
{
	ApeBitmapFont *font = ape_cache_get_data_( materialPath, APE_CACHE_POOL_FONTS );
	if ( font != NULL )
	{
		ape_mm_add_reference( &font->mem );
		return font;
	}

	PLGMesh *mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 4096, 4096 );
	if ( mesh == NULL )
	{
		PRINT_WARNING( "Failed to create font mesh, %s, aborting!\n", PlGetError() );
		return NULL;
	}

	ApeMaterial *material = ape_material_cache( materialPath, 0, false, false );
	if ( material == NULL )
	{
		PlgDestroyMesh( mesh );
		PRINT_WARNING( "Failed to load font material \"%s\"!\n", materialPath );
		return NULL;
	}

	font = PlMAlloc( sizeof( ApeBitmapFont ), true );
	font->material = material;
	font->mesh = mesh;
	font->w = w;
	font->h = h;
	font->cw = cw;
	font->ch = ch;
	font->start = start;
	font->end = end;

	strncpy( font->path, materialPath, sizeof( font->path ) );

	ape_cache_add_to_pool_( materialPath, APE_CACHE_POOL_FONTS, font );

	ape_mm_setup_reference( "bitmapFont", APE_CACHE_POOL_FONTS, &font->mem, DestroyBitmapFont, font );
	ape_mm_add_reference( &font->mem );

	return font;
}

void ss_arl_bitmap_font_release( ApeBitmapFont *font )
{
	ape_mm_release( &font->mem );
}

ApeBitmapFont *ss_arl_get_default_bitmap_font( void ) { return defaultFont; }
ApeBitmapFont *ape_get_default_small_bitmap_font( void ) { return defaultFontSmall; }
