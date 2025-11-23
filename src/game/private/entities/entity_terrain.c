// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Simple terrain system, utilising our existing brushes for rendering/collisions/etc.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "entity_terrain.h"

static constexpr char GAME_TERRAIN_CLASS_NAME[] = "terrain";

//TEMP
static constexpr char TERRAIN_DEFAULT_HEIGHTMAP[] = "heightmaps/test.png";
static constexpr char TERRAIN_DEFAULT_MATERIAL[]  = "materials/terrain/terrain_default.mat.n";

typedef struct GameTerrainTile
{
	short height;
} GameTerrainTile;

typedef struct GameTerrainEntity
{
	float minHeight;// lowest point
	float maxHeight;// heighest point

	ApeIntegerProperty numChunks;
	ApeIntegerProperty numTiles;
	ApeFloatProperty   tileSize;

	ApeBrush   **brushes;
	unsigned int numBrushes;
} GameTerrainEntity;

#define GAME_TERRAIN_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_TERRAIN_CLASS_NAME, GameTerrainEntity )

static unsigned int terrain_get_total_chunks( const GameTerrainEntity *terrain )
{
	return terrain->numChunks * terrain->numChunks;
}

static float terrain_get_chunk_size( const GameTerrainEntity *terrain )
{
	return terrain->tileSize * terrain->numTiles;
}

static unsigned int terrain_get_num_chunk_tiles( const GameTerrainEntity *terrain )
{
	return terrain->numTiles * terrain->numTiles;
}

static unsigned int terrain_get_num_chunk_vertices_row( const GameTerrainEntity *terrain )
{
	return terrain->numTiles + 1;
}

static unsigned int terrain_get_num_chunk_vertices( const GameTerrainEntity *terrain )
{
	return ( terrain->numTiles + 1 ) * ( terrain->numTiles + 1 );
}

static ApeBrush *build_terrain_chunk( ApeEntity *self, GameTerrainEntity *terrain, const QmMathVector2f basePos )
{
	ApeBrush *brush = ape_brush_create( APE_WORLD_NODE( self ), "terrain", &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO );
	if ( brush == nullptr )
	{
		game_warning_( "Failed to attach brush for terrain!\n" );
		return nullptr;
	}

	brush->type = APE_WORLD_BRUSH_TYPE_SOLID;

	// ensure brush isn't saved
	APE_WORLD_NODE( brush )->flags |= APE_WORLD_NODE_FLAG_DISCARD;

	unsigned int numChunkRowVertices = terrain_get_num_chunk_vertices_row( terrain );

	brush->numVertices = terrain_get_num_chunk_vertices( terrain );
	brush->vertices    = QM_OS_MEMORY_NEW_( QmMathVector3f, brush->numVertices );
	for ( unsigned int row = 0; row < numChunkRowVertices; ++row )
	{
		float y = basePos.y + terrain->tileSize * row;
		for ( unsigned int col = 0; col < numChunkRowVertices; ++col )
		{
			float x = basePos.x + terrain->tileSize * col;

			unsigned int vertexIndex = row * numChunkRowVertices + col;
			assert( vertexIndex < brush->numVertices );
			brush->vertices[ vertexIndex ].x = x;
			brush->vertices[ vertexIndex ].z = y;

			// for testing...
			brush->vertices[ vertexIndex ].y = sinf( x * y ) * 10.0f;
		}
	}

	brush->numFaces = terrain_get_num_chunk_tiles( terrain );
	brush->faces    = QM_OS_MEMORY_NEW_( ApeBrushFace, brush->numFaces );
	for ( unsigned int j = 0; j < brush->numFaces; ++j )
	{
		ApeBrushFace *face = &brush->faces[ j ];
		face->parent       = brush;
		face->numVertices  = 4;
		face->material     = ape_material_cache( TERRAIN_DEFAULT_MATERIAL, APE_CACHE_GROUP_WORLD, true );

		unsigned int row = j / ( numChunkRowVertices - 1 );
		unsigned int col = j % ( numChunkRowVertices - 1 );

		unsigned int x = row * numChunkRowVertices + col;
		assert( x < brush->numVertices );

		unsigned int y = row * numChunkRowVertices + ( col + 1 );
		assert( y < brush->numVertices );

		unsigned int z = ( row + 1 ) * numChunkRowVertices + ( col + 1 );
		assert( z < brush->numVertices );

		unsigned int w = ( row + 1 ) * numChunkRowVertices + col;
		assert( w < brush->numVertices );

		face->vertices[ 0 ].posIndex = x;
		face->vertices[ 1 ].posIndex = y;
		face->vertices[ 2 ].posIndex = z;
		face->vertices[ 3 ].posIndex = w;

		face->edgeLoopOrder[ 0 ] = 3;
		face->edgeLoopOrder[ 1 ] = 2;
		face->edgeLoopOrder[ 2 ] = 1;
		face->edgeLoopOrder[ 3 ] = 0;

		ape_brush_face_compute_bounds( face );
		ape_brush_face_compute_normal( face );

		ape_brush_face_apply_material_coordinates( face, &QM_MATH_VECTOR2F( 1.0f, 1.0f ), &QM_MATH_VECTOR2F( 0.0f, 0.0f ), &QM_MATH_VECTOR3F( 0.0f, 0.0f, 0.0f ), false );
	}

	ape_brush_compute_bounds( brush );

	return brush;
}

static void build_terrain( ApeEntity *self, GameTerrainEntity *terrain )
{
	if ( terrain->brushes != nullptr )
	{
		for ( unsigned int i = 0; i < terrain->numBrushes; ++i )
		{
			if ( terrain->brushes[ i ] == nullptr )
			{
				continue;
			}

			ape_world_node_destroy( APE_WORLD_NODE( terrain->brushes[ i ] ) );
			terrain->brushes[ i ] = nullptr;
		}

		qm_os_memory_free( terrain->brushes );
	}

	terrain->numBrushes = terrain_get_total_chunks( terrain );
	terrain->brushes    = QM_OS_MEMORY_NEW_( ApeBrush *, terrain->numBrushes );
	for ( unsigned int row = 0, chunk = 0; row < terrain->numChunks; ++row )
	{
		for ( unsigned int col = 0; col < terrain->numChunks; ++col )
		{
			float chunkSize             = terrain_get_chunk_size( terrain );
			terrain->brushes[ chunk++ ] = build_terrain_chunk( self, terrain, qm_math_vector2f( chunkSize * row, chunkSize * col ) );
		}
	}
}

static void *terrain_create( ApeEntity *self, AcmBranch *properties )
{
	GameTerrainEntity *terrain = QM_OS_MEMORY_NEW( GameTerrainEntity );

	terrain->numChunks = 4;
	terrain->numTiles  = 8;
	terrain->tileSize  = 64.0f;

	build_terrain( self, terrain );
	return terrain;
}

static void terrain_destroy( ApeEntity *self )
{
	GameTerrainEntity *terrain = GAME_TERRAIN_ENTITY( self );
	qm_os_memory_free( terrain->brushes );// children will get destroyed automatically
	qm_os_memory_free( terrain );
}

static void terrain_spawn( ApeEntity *self )
{
	return;

	GameTerrainEntity *terrain = GAME_TERRAIN_ENTITY( self );
	assert( terrain != nullptr );

	PLImage *image = nullptr;

	// before we can do anything, we need to load in the heightmap image
	if ( ( image = PlLoadImage( TERRAIN_DEFAULT_HEIGHTMAP ) ) == nullptr )
	{
		game_warning_( "Failed to load heightmap (%s): %s\n", TERRAIN_DEFAULT_HEIGHTMAP, PlGetError() );
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

static void terrain_on_update_property( ApeEntity *self, const ApeProperty *property )
{
	GameTerrainEntity *terrain = GAME_TERRAIN_ENTITY( self );
	assert( terrain != nullptr );

	terrain->numChunks = QM_MATH_CLAMP( 1, terrain->numChunks, 8 );
	terrain->numTiles  = QM_MATH_CLAMP( 1, terrain->numTiles, 8 );
	terrain->tileSize  = QM_MATH_CLAMP( 1.0f, terrain->tileSize, 2048.0f );

	build_terrain( self, terrain );
}

static void terrain_deserialize( ApeEntity *self, [[maybe_unused]] AcmBranch *root )
{
	GameTerrainEntity *terrain = GAME_TERRAIN_ENTITY( self );
	assert( terrain != nullptr );

	build_terrain( self, terrain );
}

ApeProperty properties[] = {
        APE_PROPERTY_BASIC( "Num Chunks", "Number of chunks per row and col.", GameTerrainEntity, numChunks, INTEGER ),
        APE_PROPERTY_BASIC( "Num Tiles", "Number of tiles per row and col.", GameTerrainEntity, numTiles, INTEGER ),
        APE_PROPERTY_BASIC( "Tile Size", "Size of each tile, width and height.", GameTerrainEntity, tileSize, FLOAT ),
};

ApeEntityClassDefinition game_terrainEntityClass_ = {
        .name        = GAME_TERRAIN_CLASS_NAME,
        .description = "Simple brush-based terrain.",

        .createFunction      = terrain_create,
        .destroyFunction     = terrain_destroy,
        .spawnFunction       = terrain_spawn,
        .deserializeFunction = terrain_deserialize,

        .onUpdateProperty = terrain_on_update_property,

        .properties    = properties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( properties ),

        .editorSpritePath = "materials/editor/icons/icon_terrain.mat.n",
};
