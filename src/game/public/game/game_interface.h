// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

// todo: these shouldn't be in here!!
extern int globalGameLog;
extern int globalGameDebugLog;
extern int globalGameWarningLog;
extern int globalGameErrorLog;

typedef enum GameModeRequest
{
	GAMEMODE_REQUEST_INITIALIZE,// called on engine initialisation
	GAMEMODE_REQUEST_SHUTDOWN,  // called when shutting down engine

	GAME_MODE_REQUEST_DRAW,
	GAME_MODE_REQUEST_DRAW_UI,

	GAMEMODE_REQUEST_TICK,// called after entity tick
	GAMEMODE_REQUEST_HANDLEINPUT,
	GAMEMODE_REQUEST_SPAWNWORLD,// called before entities are spawned in and
	                            // before starting and connecting to server
	GAMEMODE_REQUEST_DISCONNECT,
} GameModeRequest;

typedef struct GameModeInterface
{
	void ( *Initialize )( void );
	void ( *Shutdown )( void );

	void ( *Draw )( void );

	// This is basically a replacement for the above - just slightly less fussy
	bool ( *requestCallbackMethod )( GameModeRequest gameModeRequest, void *user );
} GameModeInterface;
const GameModeInterface *gameGetModeInterface( void );

typedef enum GameDifficulty
{
	GAME_DIFFICULTY_NORMAL,
	GAME_DIFFICULTY_EASY,
	GAME_DIFFICULTY_HARD,

	GAME_MAX_DIFFICULTY_MODES
} GameDifficulty;
void gameSetDifficultyMode( GameDifficulty difficulty );
GameDifficulty gameGetDifficultyMode( void );

typedef enum GameConnectionType
{
	GAME_CONNECTION_NONE,  /* not connected */
	GAME_CONNECTION_LOCAL, /* localhost */
	GAME_CONNECTION_LAN,
	GAME_CONNECTION_NET,
} GameConnectionType;
GameConnectionType gameGetConnectionType( void );

void gamePlayerConnected( const char *name, unsigned int id );
void gamePlayerDisconnected( unsigned int id );

typedef enum MenuState
{
	MENU_STATE_START, /* draw start screen */
	MENU_STATE_HUD,   /* hud/overlay mode */
} MenuState;
MenuState gameGetMenuState( void );

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
