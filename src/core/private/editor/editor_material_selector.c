// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "../ape_private.h"

#include "editor.h"

static ApeMaterial **materials;
static unsigned int numMaterials, maxMaterials;

#define MATERIAL_DEFAULT_WIDTH 128

#define MATERIAL_STORE_INC 256

static void CacheMaterialPreviewCallback( const char *path, void *user ) {
	ApeMaterial *material = apeCacheMaterial( path, APE_CACHE_EDITOR, false, true );
	if ( material == NULL )
		return;

	materials[ numMaterials++ ] = material;
	if ( numMaterials >= maxMaterials ) {
		maxMaterials += MATERIAL_STORE_INC;
		materials = PlReAllocA( materials, sizeof( ApeMaterial * ) * maxMaterials );
	}
}

static int CompareMaterials( const void *a, const void *b ) {
	const char *strA = apeGetMaterialPath( ( ApeMaterial * ) a );
	const char *strB = apeGetMaterialPath( ( ApeMaterial * ) b );
	return strcmp( strA, strB );
}

void edInitializeMaterialSelector_( void ) {
	numMaterials = 0;
	maxMaterials = MATERIAL_STORE_INC;
	materials = PL_NEW_( ApeMaterial *, maxMaterials );

	// Cache all the materials in a preview state
	PlScanDirectory( "materials/world/", "n", CacheMaterialPreviewCallback, true, NULL );
	PRINT( "Found %u world materials\n", numMaterials );

	qsort( materials, numMaterials, sizeof( ApeMaterial * ), CompareMaterials );
	for ( unsigned int i = 0; i < numMaterials; ++i ) {
		PRINT( "\t%s\n", apeGetMaterialPath( materials[ i ] ) );
	}
}

void edShutdownMaterialSelector_( void ) {
	for ( unsigned int i = 0; i < numMaterials; ++i ) {
		apeReleaseMaterial( materials[ i ] );
	}

	PL_DELETE( materials );
}

/**
 * Draw the material selection panel.
 */
void Editor_MaterialSelector_Draw( const ApeViewport *viewport ) {
	static const unsigned int mw = MATERIAL_DEFAULT_WIDTH;
	static const unsigned int mh = MATERIAL_DEFAULT_WIDTH;
	static const unsigned int sp = MATERIAL_DEFAULT_WIDTH / 8;

	unsigned int x = sp;
	unsigned int y = sp;

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	PlgSetShaderProgram( apeGetDefaultShaderProgram( APE_SHADER_DEFAULT_VERTEX ) );

	int vw, vh;
	apeGetViewportSize( viewport, &vw, &vh );

	PlgDrawRectangle( 0, 0, ( float ) vw, ( float ) vh, PL_COLOUR_DARK_SLATE_GRAY );

	PlgSetShaderProgram( apeGetDefaultShaderProgram( APE_SHADER_DEFAULT ) );

	ApeBitmapFont *font = apeGetDefaultSmallBitmapFont();
	for ( unsigned int i = 0; i < numMaterials; ++i ) {
		PLGTexture *texture = apeGetMaterialPreviewTexture( materials[ i ] );
		PlgDrawTexturedRectangle( ( float ) x, ( float ) y, ( float ) mw, ( float ) mh, texture );

		char buf[ 8 ];
		snprintf( buf, sizeof( buf ), "%ux%u", texture->w, texture->h );
		apeDrawBitmapString( font, ( float ) ( x + sp ), ( float ) ( y + sp ), 1.0f, 1.0f, PL_COLOUR_WHITE, buf, true );
		apeDrawBitmapString( font, ( float ) x, ( float ) ( y + mw + 2 ), 1.0f, 1.0f, PL_COLOUR_WHITE, apeGetMaterialPath( materials[ i ] ), true );

		x += mw + sp;
		if ( x + mw >= vw ) {
			x = sp;
			y += sp + mh;
		}

		if ( y >= vh ) {
			break;
		}
	}

	PlPopMatrix();
}

void Editor_MaterialSelector_Tick( void ) {
}
