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
#include "common/common_tbl.h"
#include "yin/core_fs.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef enum InputTarget
{
	INPUT_TARGET_MENU, /* menu mode */
	INPUT_TARGET_GAME, /* game mode */
} InputTarget;
static InputTarget inputTarget = INPUT_TARGET_MENU;

static ApeWorld *currentWorld = NULL;

static void spawn_level_command( unsigned int argc, char **argv )
{
	PLPath path;
	snprintf( path, sizeof( path ), "%s", argv[ 1 ] );
	ss_acl_spawn_world_( path );
}

static void cache_clutter( void )
{
	size_t scriptSize;
	char *scriptBuf = ss_acl_fs_load_file_buffer( "clutter.tbl", &scriptSize );
	if ( scriptBuf != NULL )
	{
		const char *p = scriptBuf;
		assert( com_tbl_validate_type( &p, "Clutter" ) );
		PL_DELETE( scriptBuf );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

const GameModeInterface *game_modeInterface;

GameState acl_gameState_;

void ss_acl_initialize_game_( void )
{
	PRINT( "Initializing Game...\n" );

	globalGameLog = PlAddLogLevel( "game", PL_COLOUR_WHITE, true );
	globalGameDebugLog = PlAddLogLevel( "game/debug", PL_COLOUR_WHITE_SMOKE,
#if !defined( NDEBUG )
	                                    true
#else
	                                    false
#endif
	);
	globalGameWarningLog = PlAddLogLevel( "game/warning", PL_COLOUR_YELLOW, true );
	globalGameErrorLog = PlAddLogLevel( "game/error", PL_COLOUR_RED, true );

	PlRegisterConsoleCommand( "level", "Load in and spawn the specified level.", 1, spawn_level_command );

	PL_ZERO_( acl_gameState_ );

	cache_clutter();

	game_modeInterface = gameGetModeInterface();
	if ( game_modeInterface == NULL )
	{
		PRINT_ERROR( "Failed to get game interface!\n" );
	}

	if ( !game_modeInterface->requestCallbackMethod( GAMEMODE_REQUEST_INITIALIZE, NULL ) )
	{
		PRINT_ERROR( "Failed to initialize game sub-system!\n" );
	}

	PRINT( "Game initialized!\n" );
}

void ss_acl_shutdown_game_( void )
{
	game_modeInterface->requestCallbackMethod( GAMEMODE_REQUEST_SHUTDOWN, NULL );
	game_modeInterface = NULL;
}

void ss_acl_tick_game_( void )
{
	COM_PROFILE_FUNCTION_START();

	game_modeInterface->requestCallbackMethod( GAMEMODE_REQUEST_TICK, NULL );

	COM_PROFILE_FUNCTION_END();
}

void ss_acl_disconnect_game_( void )
{
	if ( currentWorld != NULL )
	{
		if ( currentWorld->isDirty )
		{
			/* todo: throw a message letting the user know their changes
			 *  might be lost! */
		}

		acl_level_destroy( currentWorld );
		currentWorld = NULL;
	}

	game_modeInterface->requestCallbackMethod( GAMEMODE_REQUEST_DISCONNECT, NULL );
}

void ss_acl_spawn_world_( const char *worldPath )
{
	if ( currentWorld != NULL && strcmp( currentWorld->path, worldPath ) == 0 )
	{
		PRINT_WARNING( "World already loaded!\n" );
		return;
	}

	ss_acl_disconnect_game_();

	ApeWorld *world = acl_level_load( worldPath );
	if ( world == NULL )
	{
		PRINT_WARNING( "Failed to load world, aborting game spawn!\n" );
		return;
	}

	currentWorld = world;

	game_modeInterface->requestCallbackMethod( GAMEMODE_REQUEST_SPAWN_LEVEL, world );

	acl_level_spawn_entities_( world );

	ss_acl_start_server_( "localhost", 0 );
	ss_acl_initiate_client_connection_( "localhost", ss_acl_server_get_port_() );
}

ApeWorld *acl_level_get_current( void )
{
	return currentWorld;
}
