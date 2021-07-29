/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"
#include "font.h"
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

	unsigned int vX = PlgAddMeshVertex( font->mesh, PLVector3( x, y, 0 ), pl_vecOrigin3, colour, PLVector2( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( font->mesh, PLVector3( x, y + ( ( float ) font->ch * scale ), 0 ), pl_vecOrigin3, colour, PLVector2( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( font->mesh, PLVector3( x + ( ( float ) font->cw * scale ), y, 0 ), pl_vecOrigin3, colour, PLVector2( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( font->mesh, PLVector3( x + ( ( float ) font->cw * scale ), y + ( ( float ) font->ch * scale ), 0 ), pl_vecOrigin3, colour, PLVector2( tx + tw, ty + th ) );

	PlgAddMeshTriangle( font->mesh, vX, vY, vZ );
	PlgAddMeshTriangle( font->mesh, vZ, vY, vW );
}

void Font_AddBitmapStringToPass( const BitmapFont *font, float x, float y, float scale, PLColour colour, const char *msg, size_t length )
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

	int w = globalSystem.viewport->w;
	int h = globalSystem.viewport->h;

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

	RM_DrawMesh( font->material, font->mesh );

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
		Font_AddBitmapStringToPass( font, x + 1, y + 1, scale, PL_COLOUR_BLACK, msg, numChars );

	Font_AddBitmapStringToPass( font, x, y, scale, colour, msg, numChars );

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

	RM_DrawMesh( font->material, font->mesh );

	PlPopMatrix();
}

void Font_Initialize( void )
{
	defaultFont		 = Font_CacheBitmap( "materials/ui/fonts/default.mat", 256, 48, 8, 12, 0, 128 );
	defaultFontSmall = Font_CacheBitmap( "materials/ui/fonts/default_small.mat", 128, 24, 4, 6, 0, 128 );

	if ( defaultFont == NULL || defaultFontSmall == NULL )
		PrintError( "Failed to load default fonts!\n" );
}

void Font_Shutdown( void )
{
	Font_ReleaseBitmap( defaultFont );
	defaultFont = NULL;
}

static void Font_CB_DestroyBitmap( void *userData )
{
	BitmapFont *font = userData;
	u_assert( font != NULL );

	RM_ReleaseMaterial( font->material );

	PlgDestroyMesh( font->mesh );

	globalSystem.Free( font );
}

BitmapFont *Font_CacheBitmap( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end )
{
	BitmapFont *font = MEM_GetCachedData( materialPath, MEM_CACHE_FONT );
	if ( font != NULL )
	{
		MEM_AddReference( &font->mem );
		return font;
	}

	PLGMesh *mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 4096, 4096 );
	if ( mesh == NULL )
	{
		PrintWarn( "Failed to create font mesh, %s, aborting!\n", PlGetError() );
		return NULL;
	}

	Material *material = RM_CacheMaterial( materialPath, 0, false );
	if ( material == NULL )
	{
		PlgDestroyMesh( mesh );
		PrintWarn( "Failed to load font material \"%s\"!\n", materialPath );
		return NULL;
	}

	font		   = globalSystem.MAlloc( sizeof( BitmapFont ), true );
	font->material = material;
	font->mesh	   = mesh;
	font->w		   = w;
	font->h		   = h;
	font->cw	   = cw;
	font->ch	   = ch;
	font->start	   = start;
	font->end	   = end;

	strncpy( font->path, materialPath, sizeof( font->path ) );

	MEM_SetupReferenceInstance( "bitmapFont", &font->mem, Font_CB_DestroyBitmap, font );
	MEM_AddReference( &font->mem );

	MEM_CacheData( materialPath, MEM_CACHE_FONT, font );

	return font;
}

void Font_ReleaseBitmap( BitmapFont *font )
{
	MEM_ReleaseReference( &font->mem );
}

BitmapFont *Font_GetDefault( void ) { return defaultFont; }
BitmapFont *Font_GetDefaultSmall( void ) { return defaultFontSmall; }
