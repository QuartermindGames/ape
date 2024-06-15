// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Terrain brush implementation.
// Author:  Mark E. Sowden

#include <float.h>

#include "pm_game.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct PMTerrainBrush
{
	unsigned int width;
	unsigned int height;
	unsigned char *heightmap;

	float maxHeight;
	float minHeight;

	ApeMaterial *material;
} PMTerrainBrush;

#define SELF( X ) APE_SELF_CAST( PMTerrainBrush, X )

static bool load_raw_terrain_file( PMTerrainBrush *brush, const char *path, unsigned int width, unsigned int height )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == nullptr )
	{
		Game_Warning( "Failed to open terrain file: %s\n", PlGetError() );
		return false;
	}

	size_t size = PlGetFileSize( file );
	brush->heightmap = PL_NEW_( unsigned char, size );
	if ( PlReadFile( file, brush->heightmap, sizeof( unsigned char ), size ) == size )
	{
		// find the heighest and lowest points
		brush->minHeight = FLT_MAX;
		brush->maxHeight = FLT_MIN;
		for ( unsigned int i = 0; i < size; ++i )
		{
			if ( brush->heightmap[ i ] > ( int ) brush->maxHeight )
			{
				brush->maxHeight = brush->heightmap[ i ];
			}
			if ( brush->heightmap[ i ] < ( int ) brush->minHeight )
			{
				brush->minHeight = brush->heightmap[ i ];
			}
		}
		assert( brush->minHeight <= brush->maxHeight );
	}
	else
	{
		Game_Warning( "Failed to read in terrain: %s\n", PlGetError() );
	}

	PlCloseFile( file );

	return true;
}

static void *create_terrain()
{
	PMTerrainBrush *terrain = PL_NEW( PMTerrainBrush );
	return terrain;
}

static void destroy_terrain( void *self )
{
	PL_DELETE( SELF( self )->heightmap );

	ape_material_release( SELF( self )->material );

	PL_DELETE( self );
}

static void draw_terrain( ApeBrush *self )
{
	printf( "farts\n" );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeBrushClass pm_terrainBrushClass = {
        .name = "pm_terrainBrushClass",
        .editorName = "Terrain",
        .editorDescription = "A flat-shaded primitive terrain system for PM.",

        .iconSmall = "resources/terrain.gif",

        .createFunction = create_terrain,
        .destroyFunction = destroy_terrain,
        .drawFunction = draw_terrain,
};

float pm_brush_terrain_get_max_height( const PMTerrainBrush *self )
{
	return self->maxHeight;
}

float pm_brush_terrain_get_min_height( const PMTerrainBrush *self )
{
	return self->minHeight;
}
