// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Simple terrain system, utilising our existing brushes for rendering/collisions/etc.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "entity_terrain.h"

static void *create_terrain( ApeEntity *self, AcmBranch *properties )
{
	return QM_OS_MEMORY_NEW( GameTerrainEntity );
}

static void destroy_terrain( ApeEntity *self )
{
	GameTerrainEntity *terrain = GAME_TERRAIN_ENTITY( self );

	qm_os_memory_free( terrain );
}

static void spawn_terrain( ApeEntity *self )
{
	GameTerrainEntity *terrain = GAME_TERRAIN_ENTITY( self );
	assert( terrain != nullptr );

	//TEMP
	static constexpr char TERRAIN_TEST_HEIGHTMAP[] = "heightmaps/test.png";
	static constexpr char TERRAIN_TEST_MATERIAL[]  = "materials/terrain/terrain_test.mat.n";

	ApeMaterial *material = ape_material_cache( TERRAIN_TEST_MATERIAL, APE_CACHE_GROUP_WORLD, false );
	if ( material == nullptr )
	{
		game_warning_( "Failed to load terrain material (%s)\n", TERRAIN_TEST_MATERIAL );
		goto ABORT;
	}

	// before we can do anything, we need to load in the heightmap image
	PLImage *image = PlLoadImage( TERRAIN_TEST_HEIGHTMAP );
	if ( image == nullptr )
	{
		game_warning_( "Failed to load heightmap (%s): %s\n", TERRAIN_TEST_HEIGHTMAP, PlGetError() );
		goto ABORT;
	}

	ApeBrush *brush = ape_brush_create( APE_WORLD_NODE( self ), "terrain", &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO );
	if ( brush == nullptr )
	{
		game_warning_( "Failed to attach brush for terrain!\n" );
		goto ABORT;
	}

	brush->numFaces = GAME_TERRAIN_NUM_TILES;
	brush->faces    = QM_OS_MEMORY_NEW_( ApeBrushFace, brush->numFaces );

	for ( unsigned int i = 0; i < brush->numFaces; ++i )
	{
		ApeBrushFace *face = &brush->faces[ i ];
		face->parent       = brush;
		face->numVertices  = 4;
		face->material     = material;
	}

ABORT:
	if ( image != nullptr )
	{
		PlDestroyImage( image );
	}

	ape_world_node_destroy( APE_WORLD_NODE( self ) );
}

ApeEntityClassDefinition game_terrainEntityClass_ = {
        .name        = GAME_TERRAIN_CLASS_NAME,
        .description = "Simple brush-based terrain.",

        .createFunction  = create_terrain,
        .destroyFunction = destroy_terrain,
        .spawnFunction   = spawn_terrain,
};
