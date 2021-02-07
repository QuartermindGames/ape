/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "font.h"
#include "image.h"
#include "renderer.h"

typedef struct BitmapFont {
    Material *material;
    int w, h, cw, ch;
    char path[ PL_SYSTEM_MAX_PATH ];
} BitmapFont;
static BitmapFont *defaultFont;

static PLMesh *renderMesh;

static void Font_AddBitmapCharacterToPass( BitmapFont *font, float x, float y, float scale, PLColour colour, uint8_t character ) {
	int row = character / (font->w / font->cw);
	int col = character % (font->w / font->cw);

	int cX = col * font->cw;
	int cY = row * font->ch;

	/* figure out the correct coords we need in the font sheet */
	float tw = ( float ) font->cw / ( float ) font->w;
	float th = ( float ) font->ch / ( float ) font->h;
	float tx = ( float ) cX / ( float ) font->w;
	float ty = ( float ) cY / ( float ) font->h;

	unsigned int vX = plAddMeshVertex( renderMesh, PLVector3( x, y, 0 ), pl_vecOrigin3, colour, PLVector2( tx, ty ) );
	unsigned int vY = plAddMeshVertex( renderMesh, PLVector3( x, y + ( ( float ) font->ch * scale ), 0 ), pl_vecOrigin3, colour, PLVector2( tx, ty + th ) );
	unsigned int vZ = plAddMeshVertex( renderMesh, PLVector3( x + ( ( float ) font->cw * scale ), y, 0 ), pl_vecOrigin3, colour, PLVector2( tx + tw, ty ) );
	unsigned int vW = plAddMeshVertex( renderMesh, PLVector3( x + ( ( float ) font->cw * scale ), y + ( ( float ) font->ch * scale ), 0 ), pl_vecOrigin3, colour, PLVector2( tx + tw, ty + th ) );

	plAddMeshTriangle( renderMesh, vX, vY, vZ );
	plAddMeshTriangle( renderMesh, vZ, vY, vW );
}

/**
 * Draw a single bitmap character at the specified coordinates.
 */
void Font_DrawBitmapCharacter( BitmapFont *font, float x, float y, float scale, PLColour colour, char character ) {
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

	plClearMesh( renderMesh );

	Font_AddBitmapCharacterToPass( font, x, y, scale, colour, character );

	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();

	plLoadIdentityMatrix();

	RM_DrawMesh( font->material, renderMesh );

	plPopMatrix();
}

void Font_DrawBitmapString( BitmapFont *font, float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow ) {
	if ( scale == 0.0f ) {
		return;
	}

	size_t numChars = strlen( msg );
	if ( numChars == 0 ) {
		return;
	}

	PLShaderProgram *program = plGetCurrentShaderProgram();
	if ( program == NULL ) {
		return;
	}

	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();

	plLoadIdentityMatrix();

	plClearMesh( renderMesh );

	float n_x;
	float n_y;
	if ( shadow ) {
		n_x = x + 1;
		n_y = y + 1;
		for ( size_t i = 0; i < numChars; ++i ) {
			Font_AddBitmapCharacterToPass( font, n_x, n_y, scale, PL_COLOUR_BLACK, ( uint8_t ) msg[ i ] );
			if ( msg[ i ] == '\n' ) {
				n_y += font->ch;
				n_x = x;
			} else {
				n_x += font->cw;
			}
		}

		RM_DrawMesh( font->material, renderMesh );

		plClearMesh( renderMesh );
	}

	n_x = x;
	n_y = y;
	for ( size_t i = 0; i < numChars; ++i ) {
		Font_AddBitmapCharacterToPass( font, n_x, n_y, scale, colour, ( uint8_t ) msg[ i ] );
		if ( msg[ i ] == '\n' ) {
			n_y += font->ch;
			n_x = x;
		} else {
			n_x += font->cw;
		}
	}

	RM_DrawMesh( font->material, renderMesh );

	plPopMatrix();
}

void Font_Initialize( void ) {
	renderMesh = plCreateMesh( PL_MESH_TRIANGLES, PL_DRAW_DYNAMIC, 512, 256 );
	if ( renderMesh == NULL ) {
		PrintError( "Failed to create font mesh, %s, aborting!\n", plGetError() );
	}

	defaultFont = Font_LoadBitmap( "materials/engine/default_font.mat", 256, 48, 8, 12 );
}

void Font_Shutdown( void ) {
}

BitmapFont *Font_LoadBitmap( const char *materialPath, int w, int h, int cw, int ch ) {
    BitmapFont *font = AllocMemory( sizeof( BitmapFont ), true );
    font->material = RM_CacheMaterial( materialPath, CACHE_GROUP_STATIC, false );
	if ( font->material == NULL ) {
		PrintError( "Failed to load font material \"%s\"!\n", materialPath );
	}

	font->w = w;
	font->h = h;
	font->cw = cw;
	font->ch = ch;

	strncpy( font->path, materialPath, sizeof( font->path ) );

	return font;
}

void Font_Destroy( BitmapFont *font ) {
	if ( font == NULL ) {
		return;
	}

	RM_DestroyMaterial( font->material, false );
	free( font );
}

BitmapFont *Font_GetDefault( void ) {
	return defaultFont;
}
