/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "font.h"
#include "image.h"

static PLTexture *fontTextureSheet;
static PLMesh *renderMesh;

#define FONT_SHEET_W    256
#define FONT_SHEET_H    48
#define FONT_CHAR_W     8
#define FONT_CHAR_H     12

static void Font_AddBitmapCharacterToPass( float x, float y, float scale, PLColour colour, uint8_t character ) {
	int row = character / (FONT_SHEET_W / FONT_CHAR_W);
	int col = character % (FONT_SHEET_W / FONT_CHAR_W);

	int cX = col * FONT_CHAR_W;
	int cY = row * FONT_CHAR_H;

	/* figure out the correct coords we need in the font sheet */
	float tw = ( float ) FONT_CHAR_W / ( float ) FONT_SHEET_W;
	float th = ( float ) FONT_CHAR_H / ( float ) FONT_SHEET_H;
	float tx = ( float ) cX / ( float ) FONT_SHEET_W;
	float ty = ( float ) cY / ( float ) FONT_SHEET_H;

	unsigned int vX = plAddMeshVertex( renderMesh, PLVector3( x, y, 0 ), pl_vecOrigin3, colour, PLVector2( tx, ty ) );
	unsigned int vY = plAddMeshVertex( renderMesh, PLVector3( x, y + ( ( float ) FONT_CHAR_H * scale ), 0 ), pl_vecOrigin3, colour, PLVector2( tx, ty + th ) );
	unsigned int vZ = plAddMeshVertex( renderMesh, PLVector3( x + ( ( float ) FONT_CHAR_W * scale ), y, 0 ), pl_vecOrigin3, colour, PLVector2( tx + tw, ty ) );
	unsigned int vW = plAddMeshVertex( renderMesh, PLVector3( x + ( ( float ) FONT_CHAR_W * scale ), y + ( ( float ) FONT_CHAR_H * scale ), 0 ), pl_vecOrigin3, colour, PLVector2( tx + tw, ty + th ) );

	plAddMeshTriangle( renderMesh, vX, vY, vZ );
	plAddMeshTriangle( renderMesh, vZ, vY, vW );
}

/**
 * Draw a single bitmap character at the specified coordinates.
 */
void Font_DrawBitmapCharacter( float x, float y, float scale, PLColour colour, char character ) {
	if ( scale <= 0 ) {
		return;
	}

	SysWindow *window = Engine_GetMainWindow();
	if ( window == NULL ) {
		return;
	}

	/* todo: get the active viewport size, not the window size! */
	int w, h;
	g_system.GetWindowSize( window, &w, &h );

	float dw = ( float ) w;
	float dh = ( float ) h;
	if ( x > dw || y > dh ) {
		return;
	}

	/* setup our render pass */

	plSetTexture( fontTextureSheet, 0 );

	plClearMesh( renderMesh );

	Font_AddBitmapCharacterToPass( x, y, scale, colour, character );

	plSetNamedShaderUniformMatrix4( NULL, "pl_model", plMatrix4Identity(), false );

	plUploadMesh( renderMesh );
	plDrawMesh( renderMesh );
}

void Font_DrawBitmapString( float x, float y, float spacing, float scale, PLColour colour, const char *msg ) {
	if ( scale == 0.0f ) {
		return;
	}

	size_t numChars = strlen( msg );
	if ( numChars == 0 ) {
		return;
	}

	plClearMesh( renderMesh );

	plSetTexture( fontTextureSheet, 0 );

	float n_x = x;
	float n_y = y;
	for ( size_t i = 0; i < numChars; ++i ) {
		Font_AddBitmapCharacterToPass( n_x, n_y, scale, colour, ( uint8_t ) msg[ i ] );

		if ( msg[ i ] == '\n' ) {
			n_y += FONT_CHAR_H;
			n_x = x;
		} else {
			n_x += FONT_CHAR_W;
		}
	}

	plSetNamedShaderUniformMatrix4( NULL, "pl_model", plMatrix4Identity(), false );
	plUploadMesh( renderMesh );
	plDrawMesh( renderMesh );
}

static PLTexture *Font_LoadBitmap( const char *path ) {
	/* load in the image */
	PLImage *image = Image_LoadPackedImage( path );

	/* turn pink to alpha */
	plReplaceImageColour( image, PLColour( 255, 0, 255, 255 ), PLColour( 0, 0, 0, 0 ) );

	/* upload the texture to the GPU */

	PLTexture *texture = plCreateTexture();
	if ( texture == NULL ) {
		PrintError( "Failed to create texture for font, %s!\nPL: %s\n", path, plGetError() );
	}

	texture->filter = PL_TEXTURE_FILTER_LINEAR;

	plUploadTextureImage( texture, image );
	plDestroyImage( image );

	return texture;
}

void Font_Initialize( void ) {
	renderMesh = plCreateMesh( PL_MESH_TRIANGLES, PL_DRAW_DYNAMIC, 512, 256 );
	if ( renderMesh == NULL ) {
		PrintError( "Failed to create font mesh, %s, aborting!\n", plGetError() );
	}

	fontTextureSheet = Font_LoadBitmap( "Engine:DefaultFont.gfx" );
}

void Font_Shutdown( void ) {
	plDestroyTexture( fontTextureSheet );
}
