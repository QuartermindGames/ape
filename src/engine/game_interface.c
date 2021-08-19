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

typedef enum GameConnectionType {
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

void Game_Tick( void )
{
	if ( gameState == GAME_STATE_PAUSED )
	{
		if ( globalSystem.GetKeyState( 'z' ) )
			/* if any key was hit here, just switch to the game */
			gameState = GAME_STATE_ACTIVE;

		return;
	}

	static unsigned int spawnDelay = 0;
	if ( globalSystem.GetKeyState( 'z' ) && spawnDelay < Engine_GetNumTicks() )
	{
		Actor *dummyPlayer = Act_SpawnActor( ACTOR_PLAYER, NULL );
		Act_SetPosition( dummyPlayer, &pl_vecOrigin3 );

		spawnDelay = Engine_GetNumTicks() + 50;
	}
}

void Game_Display( void )
{
	R_DrawPerspective( R_GetGlobalCamera() );
}

void Game_SpawnWorld( const char *worldPath )
{
	World *world = W_LoadWorld( worldPath );
	if ( world == NULL )
	{
		PrintWarn( "Failed to load world, aborting game spawn!\n" );
		return;
	}

	if ( currentWorld != NULL )
		W_DestroyWorld( currentWorld );

	currentWorld = world;

	/* spawn the player in */
	playerActor = Act_SpawnActor( ACTOR_PLAYER, NULL );
	Act_SetPosition( playerActor, &PLVector3( 0, 32, 0 ) );

	Camera *camera		= R_GetGlobalCamera();
	camera->parentActor = playerActor;

	gameState	= GAME_STATE_ACTIVE;
	menuState	= MENU_STATE_HUD;
	inputTarget = INPUT_TARGET_GAME;
}

void Game_Disconnect( void )
{
	if ( currentWorld != NULL )
	{
		W_DestroyWorld( currentWorld );
		currentWorld = NULL;
	}

	Act_DestroyActors();
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

	Actor *test = Act_SpawnActorById( "point.sg.asteroid", NULL );
	if ( test == NULL )
		PrintError( "Failed to spawn test actor!\n" );

	PLVector3 pos = PLVector3( 64.0f, 0.0f, 0.0f ) ;
	Act_SetPosition( test, &pos );

	Sch_PushTask( "actor_tick", Act_TickActors, NULL, 0.0 );
}

void Game_Shutdown( void )
{
	Game_Disconnect();

	Act_Shutdown();
}
