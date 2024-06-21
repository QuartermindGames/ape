// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape/ape_public_game.h"

PL_EXTERN_C

typedef struct ApeWorld ApeWorld;

bool game_initialize( void );

/// Fetches the currently active world. Only one world can be active at a time.
/// \return Handle to the currently active world.
struct ApeWorld *ss_game_get_current_world( void );

void game_spawn_world( ApeWorld *world );

const char *game_get_identifier();

/////////////////////////////////////////////////////////////////////////////////////

typedef enum GameConnectionType
{
	GAME_CONNECTION_NONE,  /* not connected */
	GAME_CONNECTION_LOCAL, /* localhost */
	GAME_CONNECTION_LAN,
	GAME_CONNECTION_NET,
} GameConnectionType;
GameConnectionType gameGetConnectionType( void );

////////////////////////////////////////////////////////////////////

typedef enum GameMaterialSurfaceType
{
	GAME_MATERIAL_SURFACE_TYPE_NONE,
	GAME_MATERIAL_SURFACE_TYPE_ROCK,
	GAME_MATERIAL_SURFACE_TYPE_METAL,
	GAME_MATERIAL_SURFACE_TYPE_FLESH,
	GAME_MATERIAL_SURFACE_TYPE_WATER,
	GAME_MATERIAL_SURFACE_TYPE_LAVA,
	GAME_MATERIAL_SURFACE_TYPE_SOLID,
	GAME_MATERIAL_SURFACE_TYPE_GLASS,
	GAME_MATERIAL_SURFACE_TYPE_SAND,
	GAME_MATERIAL_SURFACE_TYPE_ICE,
	GAME_MATERIAL_SURFACE_TYPE_WOOD,
	GAME_MATERIAL_SURFACE_TYPE_GRASS,

	GAME_MAX_MATERIAL_SURFACE_TYPES
} GameMaterialSurfaceType;

typedef struct GameMaterialSurface
{
	char description[ 32 ];
	char **aliases;
	uint8_t numAliases;
} GameMaterialSurface;

PL_EXTERN_C_END
