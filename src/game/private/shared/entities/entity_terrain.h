// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

static constexpr char GAME_TERRAIN_CLASS_NAME[] = "terrain";

// chunks
static constexpr unsigned int GAME_TERRAIN_NUM_CHUNKS_W = 16;
static constexpr unsigned int GAME_TERRAIN_NUM_CHUNKS   = GAME_TERRAIN_NUM_CHUNKS_W * GAME_TERRAIN_NUM_CHUNKS_W;

// chunk tiles
static constexpr unsigned int GAME_TERRAIN_NUM_CHUNK_TILES_W = 8;
static constexpr unsigned int GAME_TERRAIN_NUM_CHUNK_TILES   = GAME_TERRAIN_NUM_CHUNK_TILES_W * GAME_TERRAIN_NUM_CHUNK_TILES_W;

// total number of tiles
static constexpr unsigned int GAME_TERRAIN_NUM_TILES = GAME_TERRAIN_NUM_CHUNK_TILES * GAME_TERRAIN_NUM_CHUNKS;

typedef struct GameTerrainTile
{
	short height;
} GameTerrainTile;

typedef struct GameTerrainEntity
{
	float minHeight;// lowest point
	float maxHeight;// heighest point

	ApeBrush *geometry;
} GameTerrainEntity;

#define GAME_TERRAIN_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_TERRAIN_CLASS_NAME, GameTerrainEntity )
