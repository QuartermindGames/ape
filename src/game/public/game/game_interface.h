// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

typedef struct ApeWorld ApeWorld;

typedef enum SSGameModeRequest
{
	GAMEMODE_REQUEST_INITIALIZE,// called on engine initialisation
	GAMEMODE_REQUEST_SHUTDOWN,  // called when shutting down engine

	GAME_MODE_REQUEST_DRAW,
	GAME_MODE_REQUEST_DRAW_UI,

	GAMEMODE_REQUEST_TICK,// called after entity tick
	GAMEMODE_REQUEST_HANDLE_INPUT,
	SS_GAME_MODE_REQUEST_SPAWN_WORLD,// called before entities are spawned in and
	                                 // before starting and connecting to server
	GAMEMODE_REQUEST_DISCONNECT,
} SSGameModeRequest;

typedef struct SSGameModeInterface
{
	// This is basically a replacement for the above - just slightly less fussy
	bool ( *requestCallbackMethod )( SSGameModeRequest gameModeRequest, void *user );
} SSGameModeInterface;
const SSGameModeInterface *ss_game_mode_get_interface( void );

void ss_game_initialize( void );

/// Fetches the currently active world. Only one world can be active at a time.
/// \return Handle to the currently active world.
struct ApeWorld *ss_game_get_current_world( void );

void ss_game_spawn_world( ApeWorld *world );

void ss_game_tick( void );
void ss_game_disconnect( void );

typedef enum GameConnectionType
{
	GAME_CONNECTION_NONE,  /* not connected */
	GAME_CONNECTION_LOCAL, /* localhost */
	GAME_CONNECTION_LAN,
	GAME_CONNECTION_NET,
} GameConnectionType;
GameConnectionType gameGetConnectionType( void );

typedef struct Actor Actor;

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
