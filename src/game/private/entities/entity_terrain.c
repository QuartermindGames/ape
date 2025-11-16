// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Simple terrain system, utilising our existing brushes for rendering/collisions/etc.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "entity_terrain.h"

static constexpr char GAME_TERRAIN_CLASS_NAME[] = "terrain";

// chunks
static constexpr unsigned int GAME_TERRAIN_NUM_CHUNKS_W = 4;
static constexpr unsigned int GAME_TERRAIN_NUM_CHUNKS   = GAME_TERRAIN_NUM_CHUNKS_W * GAME_TERRAIN_NUM_CHUNKS_W;

// chunk tiles
static constexpr unsigned int GAME_TERRAIN_CHUNK_NUM_TILES_W      = 8;
static constexpr unsigned int GAME_TERRAIN_CHUNK_NUM_TILES        = GAME_TERRAIN_CHUNK_NUM_TILES_W * GAME_TERRAIN_CHUNK_NUM_TILES_W;
static constexpr unsigned int GAME_TERRAIN_CHUNK_NUM_VERTICES     = ( GAME_TERRAIN_CHUNK_NUM_TILES_W + 1 ) * ( GAME_TERRAIN_CHUNK_NUM_TILES_W + 1 );
static constexpr unsigned int GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES = GAME_TERRAIN_CHUNK_NUM_TILES_W + 1;

static constexpr float GAME_TERRAIN_CHUNK_SIZE_W = 32.0f;

// total number of tiles
static constexpr unsigned int GAME_TERRAIN_NUM_TILES = GAME_TERRAIN_CHUNK_NUM_TILES * GAME_TERRAIN_NUM_CHUNKS;

typedef struct GameTerrainTile
{
	short height;
} GameTerrainTile;

typedef struct GameTerrainEntity
{
	float minHeight;// lowest point
	float maxHeight;// heighest point

	ApeBrush *geometry[ GAME_TERRAIN_NUM_CHUNKS ];
} GameTerrainEntity;

#define GAME_TERRAIN_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_TERRAIN_CLASS_NAME, GameTerrainEntity )

//TEMP
static constexpr char TERRAIN_TEST_HEIGHTMAP[] = "heightmaps/test.png";
static constexpr char TERRAIN_TEST_MATERIAL[]  = "materials/terrain/terrain_test.mat.n";

static void build_terrain( ApeEntity *self, GameTerrainEntity *terrain )
{
	for ( unsigned int i = 0; i < 1 /*GAME_TERRAIN_NUM_CHUNKS*/; ++i )
	{
		ApeBrush *brush = ape_brush_create( APE_WORLD_NODE( self ), "terrain", &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO );
		if ( brush == nullptr )
		{
			game_warning_( "Failed to attach brush for terrain!\n" );
			continue;
		}

		brush->type = APE_WORLD_BRUSH_TYPE_SOLID;

		// ensure brush isn't saved
		APE_WORLD_NODE( brush )->flags |= APE_WORLD_NODE_FLAG_DISCARD;

		brush->numVertices = GAME_TERRAIN_CHUNK_NUM_VERTICES;
		brush->vertices    = QM_OS_MEMORY_NEW_( QmMathVector3f, brush->numVertices );
		for ( unsigned int row = 0; row < GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES; ++row )
		{
			float y = GAME_TERRAIN_CHUNK_SIZE_W * row;
			for ( unsigned int col = 0; col < GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES; ++col )
			{
				float x = GAME_TERRAIN_CHUNK_SIZE_W * col;

				unsigned int vertexIndex = row * GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES + col;
				assert( vertexIndex < brush->numVertices );
				brush->vertices[ vertexIndex ].x = x;
				brush->vertices[ vertexIndex ].z = y;
			}
		}

		brush->numFaces = GAME_TERRAIN_CHUNK_NUM_TILES;
		brush->faces    = QM_OS_MEMORY_NEW_( ApeBrushFace, brush->numFaces );
		for ( unsigned int j = 0; j < brush->numFaces; ++j )
		{
			ApeBrushFace *face = &brush->faces[ j ];
			face->parent       = brush;
			face->numVertices  = 4;
			face->material     = ape_material_get_default( APE_MATERIAL_DEFAULT_FALLBACK );

			unsigned int row = j / ( GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES - 1 );
			unsigned int col = j % ( GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES - 1 );

			unsigned int x = row * GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES + col;
			assert( x < brush->numVertices );

			unsigned int y = row * GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES + ( col + 1 );
			assert( y < brush->numVertices );

			unsigned int z = ( row + 1 ) * GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES + ( col + 1 );
			assert( z < brush->numVertices );

			unsigned int w = ( row + 1 ) * GAME_TERRAIN_CHUNK_NUM_ROW_VERTICES + col;
			assert( w < brush->numVertices );

			face->vertices[ 0 ].posIndex = x;
			face->vertices[ 1 ].posIndex = y;
			face->vertices[ 2 ].posIndex = z;
			face->vertices[ 3 ].posIndex = w;

			face->edgeLoopOrder[ 0 ] = 3;
			face->edgeLoopOrder[ 1 ] = 2;
			face->edgeLoopOrder[ 2 ] = 1;
			face->edgeLoopOrder[ 3 ] = 0;

			ape_brush_face_compute_normal( face );
			ape_brush_face_compute_bounds( face );

			//ape_brush_face_apply_material_coordinates( face, &QM_MATH_VECTOR2F( 1.0f, 1.0f ), &QM_MATH_VECTOR2F( 0.0f, 0.0f ), &QM_MATH_VECTOR3F( 0.0f, 0.0f, 0.0f ), false );
		}

		ape_brush_compute_bounds( brush );

		terrain->geometry[ i ] = brush;
	}
}

static void *create_terrain( ApeEntity *self, AcmBranch *properties )
{
	GameTerrainEntity *terrain = QM_OS_MEMORY_NEW( GameTerrainEntity );
	build_terrain( self, terrain );
	return terrain;
}

static void destroy_terrain( ApeEntity *self )
{
	GameTerrainEntity *terrain = GAME_TERRAIN_ENTITY( self );

	qm_os_memory_free( terrain );
}

static void spawn_terrain( ApeEntity *self )
{
	return;

	GameTerrainEntity *terrain = GAME_TERRAIN_ENTITY( self );
	assert( terrain != nullptr );

	PLImage *image = nullptr;

	// before we can do anything, we need to load in the heightmap image
	if ( ( image = PlLoadImage( TERRAIN_TEST_HEIGHTMAP ) ) == nullptr )
	{
		game_warning_( "Failed to load heightmap (%s): %s\n", TERRAIN_TEST_HEIGHTMAP, PlGetError() );
		goto ABORT;
	}

	return;

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

        .editorSpritePath = "materials/editor/icons/icon_terrain.mat.n",
};
