// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "game/game_interface.h"
#include "world/world.h"

#include <yin/node.h>
#include <yin/core_game.h>

#include "legacy/actor.h"

#include "client/ape_client.h"
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
static MenuState menuState     = MENU_STATE_START;

static ApeWorld *currentWorld = NULL;

static void SpawnWorldCommand( unsigned int argc, char **argv )
{
	PLPath path;
	snprintf( path, sizeof( path ), "%s", argv[ 1 ] );
	apeSpawnWorld( path );
}

/****************************************
 * PUBLIC
 ****************************************/

const GameModeInterface *game_modeInterface;

GameState oge_gameState_;

void apeInitializeGame( void )
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

	PL_ZERO_( oge_gameState_ );

	ogeEntityManager_Initialize();

	const ApeEntityComponentCallbackTable *EntityComponent_Transform_GetCallbackTable( void );
	ogeEntityManager_RegisterComponent( "transform", EntityComponent_Transform_GetCallbackTable() );
	const ApeEntityComponentCallbackTable *EntityComponent_Mesh_GetCallbackTable( void );
	ogeEntityManager_RegisterComponent( "mesh", EntityComponent_Mesh_GetCallbackTable() );

	game_modeInterface = gameGetModeInterface();
	if ( game_modeInterface == NULL )
	{
		PRINT_ERROR( "Failed to get game interface!\n" );
	}

	if ( !game_modeInterface->RequestCallbackMethod( GAMEMODE_REQUEST_INITIALIZE, NULL ) )
	{
		PRINT_ERROR( "Failed to initialize game sub-system!\n" );
	}

	// has to come last, otherwise we won't find the components!
	apeRegisterEntityPrefabs();

	PRINT( "Game initialized!\n" );
}

void apeShutdownGame( void )
{
	game_modeInterface->RequestCallbackMethod( GAMEMODE_REQUEST_SHUTDOWN, NULL );
	game_modeInterface = NULL;

	apeShutdownEntityManager();
}

MenuState gameGetMenuState( void )
{
	return menuState;
}

void apeTickGame( void )
{
	apeTickEntityManager();

	game_modeInterface->RequestCallbackMethod( GAMEMODE_REQUEST_TICK, NULL );
}

void apeDisconnectGame( void )
{
	if ( currentWorld != NULL )
	{
		if ( currentWorld->isDirty )
		{
			/* todo: throw a message letting the user know their changes
			 *  might be lost! */
		}

		apeDestroyWorld( currentWorld );
		currentWorld = NULL;
	}

	game_modeInterface->RequestCallbackMethod( GAMEMODE_REQUEST_DISCONNECT, NULL );
}

void Game_SetupWorldProperties( ApeWorld *world )
{
	NdBranch *prop;
	if ( ( prop = apeGetWorldProperty( world, "music" ) ) != NULL )
	{
		PLPath musicPath;
		if ( ndGetStr( prop, musicPath, sizeof( PLPath ) ) == ND_ERROR_SUCCESS )
		{
		}
	}
}

void apeSpawnWorld( const char *worldPath )
{
	if ( currentWorld != NULL && strcmp( currentWorld->path, worldPath ) == 0 )
	{
		PRINT_WARNING( "World already loaded!\n" );
		return;
	}

	apeDisconnectGame();

	ApeWorld *world = apeLoadWorld( worldPath );
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

	game_modeInterface->RequestCallbackMethod( GAMEMODE_REQUEST_SPAWNWORLD, world );

	apeSpawnWorldEntities( world );

	apeStartServer( "localhost", 0 );

	apeInitiateClientConnection_( "localhost", apeGetServerPort() );
}

ApeWorld *apeGetCurrentWorld( void )
{
	return currentWorld;
}
