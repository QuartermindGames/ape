// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "renderer_font.h"
#include "renderer.h"

static BitmapFont *defaultFont, *defaultFontSmall;

void Font_AddBitmapCharacterToPass( const BitmapFont *font, float x, float y, float scale, PLColour colour, uint8_t character )
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

void Font_AddBitmapStringToPass( const BitmapFont *font, float x, float y, float scale, PLColour colour, const char *msg, size_t length, bool shadow )
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
			Font_AddBitmapCharacterToPass( font, n_x + 1, n_y + 1, scale, PLColourRGB( 0, 0, 0 ), ( uint8_t ) msg[ i ] );

		Font_AddBitmapCharacterToPass( font, n_x, n_y, scale, colour, ( uint8_t ) msg[ i ] );

		n_x += ( font->cw * scale );
	}
}

/**
 * Draw a single bitmap character at the specified coordinates.
 */
void Font_DrawBitmapCharacter( BitmapFont *font, float x, float y, float scale, PLColour colour, char character )
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

	Font_BeginDraw( font );

	Font_AddBitmapCharacterToPass( font, x, y, scale, colour, character );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	ogeMaterial_DrawMesh( font->material, font->mesh, NULL, 0 );

	PlPopMatrix();
}

void Font_DrawBitmapString( BitmapFont *font, float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow )
{
	if ( scale == 0.0f )
		return;

	size_t numChars = strlen( msg );
	if ( numChars == 0 )
		return;

	Font_BeginDraw( font );

	if ( shadow )
		Font_AddBitmapStringToPass( font, x + 1, y + 1, scale, PL_COLOUR_BLACK, msg, numChars, false );

	Font_AddBitmapStringToPass( font, x, y, scale, colour, msg, numChars, false );

	Font_Draw( font );
}

void Font_BeginDraw( BitmapFont *font )
{
	PlgClearMesh( font->mesh );
}

void Font_Draw( BitmapFont *font )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	ogeMaterial_DrawMesh( font->material, font->mesh, NULL, 0 );

	PlPopMatrix();
}

void YR_Font_Initialize( void )
{
	defaultFont		 = Font_CacheBitmap( "materials/ui/fonts/default.mat.n", 256, 48, 8, 12, 0, 128 );
	defaultFontSmall = Font_CacheBitmap( "materials/ui/fonts/default_small.mat.n", 128, 24, 4, 6, 0, 128 );

	if ( defaultFont == NULL || defaultFontSmall == NULL )
		PRINT_ERROR( "Failed to load default fonts!\n" );
}

void Font_Shutdown( void )
{
	Font_ReleaseBitmap( defaultFont );
	defaultFont = NULL;
}

static void Font_CB_DestroyBitmap( void *userData )
{
	BitmapFont *font = userData;
	assert( font != NULL );

	ogeMaterial_Release( font->material );

	PlgDestroyMesh( font->mesh );

	PlFree( font );
}

BitmapFont *Font_CacheBitmap( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end )
{
	BitmapFont *font = ogeMM_GetCachedData( materialPath, MEM_CACHE_FONT );
	if ( font != NULL )
	{
		ogeMemoryManager_AddReference( &font->mem );
		return font;
	}

	PLGMesh *mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 4096, 4096 );
	if ( mesh == NULL )
	{
		PRINT_WARNING( "Failed to create font mesh, %s, aborting!\n", PlGetError() );
		return NULL;
	}

	OgeMaterial *material = YnCore_Material_Cache( materialPath, 0, false, false );
	if ( material == NULL )
	{
		PlgDestroyMesh( mesh );
		PRINT_WARNING( "Failed to load font material \"%s\"!\n", materialPath );
		return NULL;
	}

	font		   = PlMAlloc( sizeof( BitmapFont ), true );
	font->material = material;
	font->mesh	   = mesh;
	font->w		   = w;
	font->h		   = h;
	font->cw	   = cw;
	font->ch	   = ch;
	font->start	   = start;
	font->end	   = end;

	strncpy( font->path, materialPath, sizeof( font->path ) );

	ogeMM_AddToCache( materialPath, MEM_CACHE_FONT, font );

	ogeMemoryManager_SetupReference( "bitmapFont", MEM_CACHE_FONT, &font->mem, Font_CB_DestroyBitmap, font );
	ogeMemoryManager_AddReference( &font->mem );

	return font;
}

void Font_ReleaseBitmap( BitmapFont *font )
{
	ogeMemoryManager_ReleaseReference( &font->mem );
}

BitmapFont *Font_GetDefault( void ) { return defaultFont; }
BitmapFont *Font_GetDefaultSmall( void ) { return defaultFontSmall; }
