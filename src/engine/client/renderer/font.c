/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "font.h"
#include "renderer.h"

static BitmapFont *defaultFont;

static PLGMesh *renderMesh;

static void Font_AddBitmapCharacterToPass( BitmapFont *font, float x, float y, float scale, PLColour colour, uint8_t character )
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

	unsigned int vX = PlgAddMeshVertex( renderMesh, PLVector3( x, y, 0 ), pl_vecOrigin3, colour, PLVector2( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( renderMesh, PLVector3( x, y + ( ( float ) font->ch * scale ), 0 ), pl_vecOrigin3, colour, PLVector2( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( renderMesh, PLVector3( x + ( ( float ) font->cw * scale ), y, 0 ), pl_vecOrigin3, colour, PLVector2( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( renderMesh, PLVector3( x + ( ( float ) font->cw * scale ), y + ( ( float ) font->ch * scale ), 0 ), pl_vecOrigin3, colour, PLVector2( tx + tw, ty + th ) );

	PlgAddMeshTriangle( renderMesh, vX, vY, vZ );
	PlgAddMeshTriangle( renderMesh, vZ, vY, vW );
}

/**
 * Draw a single bitmap character at the specified coordinates.
 */
void Font_DrawBitmapCharacter( BitmapFont *font, float x, float y, float scale, PLColour colour, char character )
{
	if ( scale <= 0 )
	{
		return;
	}

	int w, h;
	globalSystem.GetCurrentDisplaySize( &w, &h );

	float dw = ( float ) w;
	float dh = ( float ) h;
	if ( x > dw || y > dh )
	{
		return;
	}

	/* setup our render pass */

	PlgClearMesh( renderMesh );

	Font_AddBitmapCharacterToPass( font, x, y, scale, colour, character );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	RM_DrawMesh( font->material, renderMesh );

	PlPopMatrix();
}

void Font_DrawBitmapString( BitmapFont *font, float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow )
{
	if ( scale == 0.0f )
	{
		return;
	}

	size_t numChars = strlen( msg );
	if ( numChars == 0 )
	{
		return;
	}

	PLGShaderProgram *program = PlgGetCurrentShaderProgram();
	if ( program == NULL )
	{
		return;
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgClearMesh( renderMesh );

	float n_x;
	float n_y;
	if ( shadow )
	{
		n_x = x + 1;
		n_y = y + 1;
		for ( size_t i = 0; i < numChars; ++i )
		{
			Font_AddBitmapCharacterToPass( font, n_x, n_y, scale, PL_COLOUR_BLACK, ( uint8_t ) msg[ i ] );
			if ( msg[ i ] == '\n' )
			{
				n_y += ( font->ch * scale );
				n_x = x;
			}
			else
			{
				n_x += ( font->cw * scale );
			}
		}

		RM_DrawMesh( font->material, renderMesh );

		PlgClearMesh( renderMesh );
	}

	n_x = x;
	n_y = y;
	for ( size_t i = 0; i < numChars; ++i )
	{
		Font_AddBitmapCharacterToPass( font, n_x, n_y, scale, colour, ( uint8_t ) msg[ i ] );
		if ( msg[ i ] == '\n' )
		{
			n_y += ( font->ch * scale );
			n_x = x;
		}
		else
		{
			n_x += ( font->cw * scale );
		}
	}

	RM_DrawMesh( font->material, renderMesh );

	PlPopMatrix();
}

void Font_Initialize( void )
{
	renderMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 512, 256 );
	if ( renderMesh == NULL )
	{
		PrintError( "Failed to create font mesh, %s, aborting!\n", PlGetError() );
	}

	defaultFont = Font_LoadBitmap( "materials/engine/default_font.mat", 256, 48, 8, 12, 0, 128 );
}

void Font_Shutdown( void )
{
	Font_Destroy( defaultFont );
	defaultFont = NULL;
}

BitmapFont *Font_LoadBitmap( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end )
{
	BitmapFont *font = globalSystem.MAlloc( sizeof( BitmapFont ), true );
	font->material   = RM_CacheMaterial( materialPath, CACHE_GROUP_STATIC, false );
	if ( font->material == NULL )
	{
		PrintError( "Failed to load font material \"%s\"!\n", materialPath );
	}

	font->w     = w;
	font->h     = h;
	font->cw    = cw;
	font->ch    = ch;
	font->start = start;
	font->end   = end;

	strncpy( font->path, materialPath, sizeof( font->path ) );

	return font;
}

void Font_Destroy( BitmapFont *font )
{
	if ( font == NULL )
		return;

	RM_ReleaseMaterial( font->material );
	globalSystem.Free( font );
}

BitmapFont *Font_GetDefault( void )
{
	return defaultFont;
}
