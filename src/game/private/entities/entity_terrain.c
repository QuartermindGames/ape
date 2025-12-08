// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Simple terrain system, utilising our existing brushes for rendering/collisions/etc.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "entity_terrain.h"

#include "qmos/public/qm_os_linked_list.h"
#include "qmos/public/qm_os_string.h"

/*
 * Wishlist
	- Do all of the terrain work in the shader instead...
	gave up on this for now because of stencil shadows, collisions and such, but there are solutions!
	- If we don't do above, at least offload the work of generating the terrain into another thread
	- LODS!! We can do this, albeit a little gross but different copies of each chunk for different levels
	the only thing that sucks is connecting the brushes together...
	- Or do away with this entirely and implement a proper terrain system, rather than dumb hacks :p
	- Proc terrain generation (qm/pl has perlin-noise functions)
 */

static constexpr char GAME_TERRAIN_CLASS_NAME[] = "terrain";

static constexpr char TERRAIN_DEFAULT_HEIGHTMAP[] = "heightmap_test";
static constexpr char TERRAIN_DEFAULT_MATERIAL[]  = "materials/terrain/terrain_default.mat.n";

static constexpr unsigned int TERRAIN_HEIGHTMAP_MAX_NAME    = 128;
static constexpr char         TERRAIN_HEIGHTMAP_BASE_PATH[] = "materials/terrain/heightmaps";

typedef struct GameTerrainEntity
{
	float minHeight;// lowest point
	float maxHeight;// heighest point

	ApeIntegerProperty numChunks;
	ApeIntegerProperty numTiles;
	ApeFloatProperty   tileSize;

	ApeStringProperty heightmapName[ TERRAIN_HEIGHTMAP_MAX_NAME ];
	ApeFloatProperty  heightmapMultiplier;

	char     oldHeightmapName[ TERRAIN_HEIGHTMAP_MAX_NAME ];
	uint8_t *heightmap;
	uint16_t heightmapWidth;
	uint16_t heightmapHeight;

	ApeBrush   **brushes;
	unsigned int numBrushes;
} GameTerrainEntity;

#define GAME_TERRAIN_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_TERRAIN_CLASS_NAME, GameTerrainEntity )

/*
 * Returns the total chunks in the terrain.
 */
static unsigned int terrain_get_total_chunks( const GameTerrainEntity *terrain )
{
	return terrain->numChunks * terrain->numChunks;
}

/*
 * Get the width of a chunk.
 * If you want the *total* size you'll need to multiply the result by itself.
 */
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

/*
 * Get the width of the terrain.
 * If you want the *total* size you'll need to multiply the result by itself.
 */
static float terrain_get_size( const GameTerrainEntity *terrain )
{
	return terrain_get_chunk_size( terrain ) * terrain->numChunks;
}

static void terrain_load_heightmap( GameTerrainEntity *terrain )
{
	if ( strcmp( terrain->heightmapName, terrain->oldHeightmapName ) == 0 )
	{
		// heightmap hasn't changed
		return;
	}

	char *path = qm_os_string_alloc( nullptr, "%s/%s.png", TERRAIN_HEIGHTMAP_BASE_PATH, terrain->heightmapName );
	if ( path == nullptr )
	{
		game_warning_( "Failed to setup path for loading heightmap (%s/%s)!\n", TERRAIN_HEIGHTMAP_BASE_PATH, terrain->heightmapName );
		return;
	}

	PLImage *image = PlLoadImage( path );
	if ( image == nullptr )
	{
		game_warning_( "Failed to load heightmap (%s)\n", path );
		qm_os_memory_free( path );
		return;
	}

	// check if the destination needs to be resized
	size_t size    = image->width * image->height;
	size_t oldSize = terrain->heightmapWidth * terrain->heightmapHeight;
	if ( terrain->heightmap != nullptr && size != oldSize )
	{
		qm_os_memory_free( terrain->heightmap );
		terrain->heightmap = nullptr;
	}

	if ( terrain->heightmap == nullptr )
	{
		terrain->heightmap = QM_OS_MEMORY_NEW_( uint8_t, size );
	}

	if ( terrain->heightmap != nullptr )
	{
		// urgh, horrible nested warning spaghetti
		if ( PlConvertPixelFormat( image, PL_IMAGEFORMAT_R8 ) )
		{
			if ( PlGetImageDataSize( image ) == size )
			{
				const uint8_t *src = PlGetImageData( image, 0, 0 );
				if ( src != nullptr )
				{
					memcpy( terrain->heightmap, src, size );
				}
				else
				{
					game_warning_( "Failed to get image data for heightmap: %s\n", PlGetError() );
				}
			}
			else
			{
				game_warning_( "Unexpected image size for heightmap (!= %u)!\n", size );
			}
		}
		else
		{
			game_warning_( "Failed to convert pixel format: %s\n", PlGetError() );
			// don't need to worry for free for heightmap store, as we'll do that later
		}
	}

	unsigned int w = PlGetImageWidth( image );
	unsigned int h = PlGetImageHeight( image );

	PlDestroyImage( image );

	if ( terrain->heightmap == nullptr )
	{
		game_warning_( "Failed to load heightmap (%s)!\n", path );
		qm_os_memory_free( path );
		return;
	}

	terrain->heightmapWidth  = w;
	terrain->heightmapHeight = h;

	strcpy( terrain->oldHeightmapName, terrain->heightmapName );

	qm_os_memory_free( path );
}

static ApeBrush *build_terrain_chunk( ApeEntity *self, GameTerrainEntity *terrain, const QmMathVector2f chunkBasePos )
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
		float y = chunkBasePos.y + terrain->tileSize * row;
		for ( unsigned int col = 0; col < numChunkRowVertices; ++col )
		{
			float x = chunkBasePos.x + terrain->tileSize * col;

			unsigned int vertexIndex = row * numChunkRowVertices + col;
			assert( vertexIndex < brush->numVertices );
			brush->vertices[ vertexIndex ].x = x;
			brush->vertices[ vertexIndex ].z = y;

			// now apply the height
			if ( terrain->heightmap != nullptr )
			{
				float terrainSize = terrain_get_size( terrain );

				unsigned int px = ( float ) terrain->heightmapWidth / terrainSize * ( x + terrainSize * 0.5f );
				if ( px >= terrain->heightmapWidth )
				{
					px = terrain->heightmapWidth - 1;
				}

				unsigned int py = ( float ) terrain->heightmapHeight / terrainSize * ( y + terrainSize * 0.5f );
				if ( py >= terrain->heightmapHeight )
				{
					py = terrain->heightmapHeight - 1;
				}

				unsigned int index               = py * terrain->heightmapWidth + px;
				const float  height              = QM_MATH_BTOF( terrain->heightmap[ index ] ) * terrain->heightmapMultiplier;
				brush->vertices[ vertexIndex ].y = height;

				game_print_( "x: %f y: %f px: %u py: %u height: %f\n", x, y, px, py, height );

				if ( height > terrain->maxHeight )
				{
					terrain->maxHeight = height;
				}
				if ( height < terrain->minHeight )
				{
					terrain->minHeight = height;
				}
			}
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
	terrain_load_heightmap( terrain );

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

	QmMathVector3f mins = {};
	QmMathVector3f maxs = {};

	float terrainSize = terrain_get_size( terrain );

	terrain->numBrushes = terrain_get_total_chunks( terrain );
	terrain->brushes    = QM_OS_MEMORY_NEW_( ApeBrush *, terrain->numBrushes );
	for ( unsigned int row = 0, chunk = 0; row < terrain->numChunks; ++row )
	{
		for ( unsigned int col = 0; col < terrain->numChunks; ++col, ++chunk )
		{
			float          chunkSize  = terrain_get_chunk_size( terrain );
			QmMathVector2f chunkPos   = qm_math_vector2f( chunkSize * row - terrainSize * 0.5f, chunkSize * col - terrainSize * 0.5f );
			terrain->brushes[ chunk ] = build_terrain_chunk( self, terrain, chunkPos );

			PLCollisionAABB bounds = ape_world_node_get_local_bounds( APE_WORLD_NODE( terrain->brushes[ chunk ] ) );
			if ( bounds.mins.x < mins.x ) { mins.x = bounds.mins.x; }
			if ( bounds.mins.y < mins.y ) { mins.y = bounds.mins.y; }
			if ( bounds.mins.z < mins.z ) { mins.z = bounds.mins.z; }
			if ( bounds.maxs.x > maxs.x ) { maxs.x = bounds.maxs.x; }
			if ( bounds.maxs.y > maxs.y ) { maxs.y = bounds.maxs.y; }
			if ( bounds.maxs.z > maxs.z ) { maxs.z = bounds.maxs.z; }
		}
	}

	// ensure that our bounds match the total bounds of our chunks
	//TODO: there is supposed to be some logic to handle this automatically...
	ape_world_node_set_local_bounds( APE_WORLD_NODE( self ), &mins, &maxs );

	// URGH
	QmOsLinkedList *faces = qm_os_linked_list_create();
	for ( unsigned int i = 0; i < terrain->numBrushes; ++i )
	{
		ApeBrush *brush = terrain->brushes[ i ];
		assert( brush != nullptr );

		for ( unsigned int j = 0; j < brush->numFaces; ++j )
		{
			qm_os_linked_list_push_back( faces, &brush->faces[ j ] );
		}
	}

	ape_brush_smooth_faces( faces );

	qm_os_memory_free( faces );
}

static void *terrain_create( ApeEntity *self, AcmBranch *properties )
{
	GameTerrainEntity *terrain = QM_OS_MEMORY_NEW( GameTerrainEntity );

	terrain->numChunks = 4;
	terrain->numTiles  = 8;
	terrain->tileSize  = 64.0f;

	terrain->heightmapMultiplier = 500.0f;
	snprintf( terrain->heightmapName, sizeof( terrain->heightmapName ), "%s", TERRAIN_DEFAULT_HEIGHTMAP );

	build_terrain( self, terrain );
	return terrain;
}

static void terrain_destroy( ApeEntity *self )
{
	GameTerrainEntity *terrain = GAME_TERRAIN_ENTITY( self );
	qm_os_memory_free( terrain->brushes );// children will get destroyed automatically
	qm_os_memory_free( terrain->heightmap );
	qm_os_memory_free( terrain );
}

static void terrain_spawn( ApeEntity *self )
{
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
        APE_PROPERTY_BASIC( "Heightmap Multiplier", "", GameTerrainEntity, heightmapMultiplier, FLOAT ),
        APE_PROPERTY_STRING( "Heightmap Name", "Name of the heightmap to use.", GameTerrainEntity, heightmapName ),
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
