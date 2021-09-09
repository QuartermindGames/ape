/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "game_interface.h"
#include "actor.h"
#include "world.h"

#include "client/sgui.h"
#include "client/renderer/renderer.h"

/* game specific implementation goes here! */

typedef enum InputTarget
{
	INPUT_TARGET_MENU, /* menu mode */
	INPUT_TARGET_GAME, /* game mode */
} InputTarget;
static InputTarget inputTarget = INPUT_TARGET_MENU;
static MenuState   menuState   = MENU_STATE_START;

static World *currentWorld = NULL;

typedef enum GameState
{
	GAME_STATE_PAUSED,
	GAME_STATE_ACTIVE,
} GameState;
GameState gameState = GAME_STATE_PAUSED;

typedef enum GameConnectionType
{
	GAME_CONNECTION_LOCAL,
	GAME_CONNECTION_LAN,
	GAME_CONNECTION_NET,
} GameConnectionType;
static GameConnectionType gameConnectionType = GAME_CONNECTION_LOCAL;

MenuState Game_GetMenuState( void )
{
	return menuState;
}

static Actor *playerActor = NULL;

Actor *Game_GetPlayer( void )
{
	return playerActor;
}

static int gameRestartCountdown = 0; /* timer before map respawn */

void Game_Tick( void )
{
	Actor *player = Act_GetByTag( "player", NULL );
	if ( player == NULL )
	{
		return;
	}

	if ( player->health <= 0 )
	{
		if ( gameRestartCountdown <= 0 )
		{
			PlParseConsoleString( "world worlds/menu.node" );
			return;
		}

		gameRestartCountdown--;
	}
}

void Game_Display( void )
{
	R_DrawPerspective( R_GetGlobalCamera() );
}

void SG_PrecacheData( void );
void SG_DestroyCachedData( void );

void Game_Disconnect( void )
{
	if ( currentWorld != NULL )
	{
		W_DestroyWorld( currentWorld );
		currentWorld = NULL;
	}

	Act_DestroyActors();

	Sch_KillTask( "actor_tick" );

	/* reset the camera state */
	Camera *camera = R_GetGlobalCamera();
	camera->followMode = CAMERA_MODE_EYE;
	camera->parentActor = NULL;
	camera->internal->position = pl_vecOrigin3;
	camera->internal->angles = pl_vecOrigin3;

	SG_DestroyCachedData();
}

void Game_SetupWorldProperties( World *world )
{
	NLNode *prop;
	if ( ( prop = W_GetWorldProperty( world, "music" ) ) != NULL )
	{
		PLPath musicPath;
		if ( NL_GetStr( prop, musicPath, sizeof( PLPath ) ) == NL_ERROR_SUCCESS )
		{

		}
	}

	gameRestartCountdown = 200;
}

void Game_SpawnWorld( const char *worldPath )
{
	Game_Disconnect();

	World *world = W_LoadWorld( worldPath );
	if ( world == NULL )
	{
		PrintWarn( "Failed to load world, aborting game spawn!\n" );
		return;
	}

	currentWorld = world;

	/* todo: HACK, if it's the menu, force menu mode!! */
	const char *fileName = PlGetFileName( worldPath );
	if ( strncmp( "menu", fileName, strlen( fileName ) - 5 ) == 0 )
		menuState = MENU_STATE_START;
	else
		menuState = MENU_STATE_HUD;

	gameState	= GAME_STATE_ACTIVE;
	inputTarget = INPUT_TARGET_GAME;

	SG_PrecacheData();

	Sch_PushTask( "actor_tick", Act_TickActors, NULL, 0.0 );

	Game_SetupWorldProperties( world );
}

static void Game_Cmd_Disconnect( unsigned int argc, char **argv )
{
	Game_Disconnect();
}

World *Game_GetCurrentWorld( void )
{
	return currentWorld;
}

static void Cmd_SpawnWorld( unsigned int argc, char **argv )
{
	if ( argc <= 1 )
	{
		PrintWarn( "Invalid argument, please specify world!\n" );
		return;
	}

	Game_SpawnWorld( argv[ 1 ] );
}

static PLLibrary *dllGamePtr = NULL;

void Game_Initialize( void )
{
	Print( "Initializing game\n" );

	dllGamePtr = PlLoadLibrary( "./game", true );
	if ( dllGamePtr == NULL )
		PrintError( "Failed to load game module, aborting!\nPL: %s\n", PlGetError() );

	DllGameInterface GetDllInterface = ( DllGameInterface ) PlGetLibraryProcedure( dllGamePtr, INTERFACE_PROCEDURE );
	if ( GetDllInterface == NULL )
		PrintError( "Failed to fetch \"" INTERFACE_PROCEDURE "\" from game module, aborting!\nPL: %s\n", PlGetError() );

	/* initialize the interface */
	//globalGame = GetDllInterface( GAME_INTERFACE_VERSION, &globalSystem, &globalEngine );

	Act_Initialize();

	PlRegisterConsoleCommand( "world", Cmd_SpawnWorld, "Load in and spawn the specified world." );
	PlRegisterConsoleCommand( "disconnect", Game_Cmd_Disconnect, "Disconnect from the current game." );

#if 0
	Actor *test = Act_SpawnActorById( "point.sg.asteroid", NULL );
	if ( test == NULL )
		PrintError( "Failed to spawn test actor!\n" );

	PLVector3 pos = PLVector3( 64.0f, 0.0f, 0.0f ) ;
	Act_SetPosition( test, &pos );

	Sch_PushTask( "actor_tick", Act_TickActors, NULL, 0.0 );
#endif
}

void Game_Shutdown( void )
{
	Game_Disconnect();

	Act_Shutdown();
}

/* ======================================================================
 * Game Difficulty Management
 * ====================================================================*/

static GameDifficulty gameDifficulty = GAME_DIFFICULTY_NORMAL;
void				  Game_SetDifficultyMode( const GameDifficulty difficulty ) { gameDifficulty = difficulty; }
GameDifficulty		  Game_GetDifficultyMode( void ) { return gameDifficulty; }
