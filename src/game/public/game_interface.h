// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

// todo: these shouldn't be in here!!
extern int globalGameLog;
extern int globalGameDebugLog;
extern int globalGameWarningLog;
extern int globalGameErrorLog;

typedef enum GameModeRequest
{
	GAMEMODE_REQUEST_TICK,
	GAMEMODE_REQUEST_HANDLEINPUT,
	GAMEMODE_REQUEST_SPAWNWORLD,
	GAMEMODE_REQUEST_DISCONNECT,
} GameModeRequest;

typedef struct GameModeInterface
{
	void ( *Initialize )( void );
	void ( *Shutdown )( void );

	void ( *NewGame )( const char *path );
	void ( *SaveGame )( const char *path );
	void ( *RestoreGame )( const char *path );

	void ( *Precache )( void );

	void ( *Draw )( void );
	void ( *DrawMenu )( const struct ApeViewport *viewport );

	// This is basically a replacement for the above - just slightly less fussy
	bool ( *RequestCallbackMethod )( GameModeRequest gameModeRequest, void *user );
} GameModeInterface;
const GameModeInterface *Game_GetModeInterface( void );
extern const GameModeInterface *gameModeInterface;

typedef enum GameDifficulty
{
	GAME_DIFFICULTY_NORMAL,
	GAME_DIFFICULTY_EASY,
	GAME_DIFFICULTY_HARD,

	GAME_MAX_DIFFICULTY_MODES
} GameDifficulty;
void Game_SetDifficultyMode( GameDifficulty difficulty );
GameDifficulty Game_GetDifficultyMode( void );

typedef enum GameConnectionType
{
	GAME_CONNECTION_NONE,  /* not connected */
	GAME_CONNECTION_LOCAL, /* localhost */
	GAME_CONNECTION_LAN,
	GAME_CONNECTION_NET,
} GameConnectionType;
GameConnectionType Game_GetConnectionType( void );

void Game_PlayerConnected( const char *name, unsigned int id );
void Game_PlayerDisconnected( unsigned int id );

typedef enum MenuState
{
	MENU_STATE_START, /* draw start screen */
	MENU_STATE_HUD,   /* hud/overlay mode */
} MenuState;
MenuState Game_GetMenuState( void );

typedef struct Actor Actor;

void Game_SpawnWorld( const char *worldPath );
struct ApeWorld *Game_GetCurrentWorld( void );


