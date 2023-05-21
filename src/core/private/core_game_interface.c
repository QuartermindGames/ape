// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "game_interface.h"
#include "world.h"

#include <yin/node.h>
#include <yin/core_game.h>

#include "legacy/actor.h"

#include "client/client.h"
#include "client/renderer/renderer.h"

#include "server/server.h"

/****************************************
 * PRIVATE
 ****************************************/

typedef enum InputTarget
{
	INPUT_TARGET_MENU, /* menu mode */
	INPUT_TARGET_GAME, /* game mode */
} InputTarget;
static InputTarget inputTarget = INPUT_TARGET_MENU;
static MenuState   menuState   = MENU_STATE_START;

static OgeWorld *currentWorld = NULL;

static void SpawnWorldCommand( unsigned int argc, char **argv )
{
	PLPath path;
	snprintf( path, sizeof( path ), "worlds/%s/%s." YN_CORE_WORLD_EXTENSION, argv[ 1 ], argv[ 1 ] );
	Game_SpawnWorld( path );
}

/****************************************
 * PUBLIC
 ****************************************/

GameState gameState;

void ogeInitializeGame( void )
{
	PRINT( "Initializing Game...\n" );

	globalGameLog      = PlAddLogLevel( "game", PL_COLOUR_WHITE, true );
	globalGameDebugLog = PlAddLogLevel( "game/debug", PL_COLOUR_WHITE_SMOKE,
#if !defined( NDEBUG )
	                                    true
#else
	                                    false
#endif
	);
	globalGameWarningLog = PlAddLogLevel( "game/warning", PL_COLOUR_YELLOW, true );
	globalGameErrorLog   = PlAddLogLevel( "game/error", PL_COLOUR_RED, true );

	PlRegisterConsoleCommand( "world", "Load in and spawn the specified world.", 1, SpawnWorldCommand );

	PL_ZERO_( gameState );

	YnCore_EntityManager_Initialize();

	const YNCoreEntityComponentCallbackTable *EntityComponent_Transform_GetCallbackTable( void );
	YnCore_EntityManager_RegisterComponent( "transform", EntityComponent_Transform_GetCallbackTable() );
	const YNCoreEntityComponentCallbackTable *EntityComponent_Mesh_GetCallbackTable( void );
	YnCore_EntityManager_RegisterComponent( "mesh", EntityComponent_Mesh_GetCallbackTable() );

	gameModeInterface = Game_GetModeInterface();
	if ( gameModeInterface == NULL )
	{
		PRINT_ERROR( "Failed to get game interface!\n" );
	}

	gameModeInterface->Initialize();

	// has to come last, otherwise we won't find the components!
	YnCore_EntityManager_RegisterEntityPrefabs();

	PRINT( "Game initialized!\n" );
}

void ogeShutdownGame( void )
{
	gameModeInterface->Shutdown();
	gameModeInterface = NULL;

	YnCore_EntityManager_Shutdown();
}

MenuState Game_GetMenuState( void )
{
	return menuState;
}

void Game_Tick( void )
{
	YnCore_EntityManager_Tick();

	gameModeInterface->RequestCallbackMethod( GAMEMODE_REQUEST_TICK, NULL );
}

void Game_Disconnect( void )
{
	if ( currentWorld != NULL )
	{
		if ( currentWorld->isDirty )
		{
			/* todo: throw a message letting the user know their changes
			 *  might be lost! */
		}

		YnCore_World_Destroy( currentWorld );
		currentWorld = NULL;
	}

	gameModeInterface->RequestCallbackMethod( GAMEMODE_REQUEST_DISCONNECT, NULL );
}

void Game_SetupWorldProperties( OgeWorld *world )
{
	YNNodeBranch *prop;
	if ( ( prop = YnCore_World_GetProperty( world, "music" ) ) != NULL )
	{
		PLPath musicPath;
		if ( YnNode_GetStr( prop, musicPath, sizeof( PLPath ) ) == YN_NODE_ERROR_SUCCESS )
		{
		}
	}
}

void Game_SpawnWorld( const char *worldPath )
{
	if ( currentWorld != NULL && strcmp( currentWorld->path, worldPath ) == 0 )
	{
		PRINT_WARNING( "World already loaded!\n" );
		return;
	}

	Game_Disconnect();

	OgeWorld *world = YnCore_World_Load( worldPath );
	if ( world == NULL )
	{
		PRINT_WARNING( "Failed to load world, aborting game spawn!\n" );
		return;
	}

	currentWorld = world;

	/* HACK, if it's the menu, force menu mode!! */
	const char *fileName = PlGetFileName( worldPath );
	if ( strncmp( "menu", fileName, strlen( fileName ) - 5 ) == 0 )
	{
		menuState = MENU_STATE_START;
	}
	else
	{
		menuState = MENU_STATE_HUD;
	}

	//gameState	= GAME_STATE_ACTIVE;
	inputTarget = INPUT_TARGET_GAME;

	gameModeInterface->RequestCallbackMethod( GAMEMODE_REQUEST_SPAWNWORLD, world );

	YnCore_World_SpawnEntities( world );

	Server_Start( "localhost", 0 );

	YnCore_Client_InitiateConnection( "localhost", Server_GetPort() );
}

OgeWorld *Game_GetCurrentWorld( void )
{
	return currentWorld;
}
