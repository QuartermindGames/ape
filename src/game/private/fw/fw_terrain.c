// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plmodel/plm.h>

#include "fw_terrain.h"

#define TERRAIN_HM_WIDTH  256
#define TERRAIN_HM_HEIGHT 256
#define TERRAIN_HM_SIZE   ( TERRAIN_HM_WIDTH * TERRAIN_HM_HEIGHT )

static unsigned char terrainHeightmap[ TERRAIN_HM_SIZE ];
static unsigned char terrainMinHeight, terrainMaxHeight;

#if 0// originally was going to have multiple meshes - let's go with one mesh instead :)
#	define TERRAIN_CHUNK_NUM_X 8
#	define TERRAIN_CHUNK_NUM_Y 8
#	define TERRAIN_NUM_CHUNKS  ( TERRAIN_CHUNK_NUM_X * TERRAIN_CHUNK_NUM_Y )
#endif

static OgeMaterial *terrainMaterial = NULL;
static PLMModel *terrainModel          = NULL;

#define TERRAIN_TILE_SPACING 64

void FW_Terrain_Initialize( void )
{
	terrainModel = PlmLoadModel( "models/terrain.ply" );
	if ( terrainModel == NULL )
	{
		Game_Error( "Failed to load terrain: %s\n", PlGetError() );
		return;
	}
}

void FW_Terrain_Shutdown( void )
{
	PlmDestroyModel( terrainModel );

	ogeMaterial_Release( terrainMaterial );
}

/****************************************/
/* Overview represents the overhead view of the world
 * that players can use as a point of reference - basically
 * a minimap... */

static PLGTexture *overview = NULL;
PLGTexture *FW_Terrain_GetOverview( void ) { return overview; }

static void UpdateOverview( void )
{
}

/****************************************/
/****************************************/

/**
 * Loads the terrain into our heightmap buffer.
 */
static bool ParseTerrainFile( PLFile *file )
{
	const char *path = PlGetFilePath( file );

	// validate the file size and ensure it's what we're expecting
	size_t fileSize = PlGetFileSize( file );
	assert( TERRAIN_HM_SIZE == fileSize );
	if ( TERRAIN_HM_SIZE != fileSize )
	{
		Game_Warning( "Invalid terrain size (%d != %d)!\n", TERRAIN_HM_SIZE, fileSize );
		return false;
	}

	if ( PlReadFile( file, terrainHeightmap, sizeof( char ), TERRAIN_HM_SIZE ) != TERRAIN_HM_SIZE )
	{
		Game_Warning( "Failed to read in terrain: %s\n", PlGetError() );
		return false;
	}

	// find the heighest and lowest points
	terrainMinHeight = UINT8_MAX;
	terrainMaxHeight = 0;
	for ( unsigned int i = 0; i < TERRAIN_HM_SIZE; ++i )
	{
		if ( terrainHeightmap[ i ] > terrainMaxHeight )
			terrainMaxHeight = terrainHeightmap[ i ];
		if ( terrainHeightmap[ i ] < terrainMinHeight )
			terrainMinHeight = terrainHeightmap[ i ];
	}

	UpdateOverview();

	return true;
}

bool FW_Terrain_Load( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		Game_Warning( "Failed to load terrain (%s): %s\n", path, PlGetError() );
		return false;
	}

	bool status = ParseTerrainFile( file );
	if ( !status )
		Game_Warning( "Failed to load terrain (%s)!\n", path );

	PlCloseFile( file );

	return status;
}

float FW_Terrain_GetHeight( float x, float y )
{
	return 0;
}

float FW_Terrain_GetMaxHeight( void )
{
	return terrainMaxHeight;
}

float FW_Terrain_GetMinHeight( void )
{
	return terrainMinHeight;
}
