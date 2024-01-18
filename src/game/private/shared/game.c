// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"
#include "yin/core_fs.h"
#include "common/common_tbl.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeWorld *currentWorld = NULL;

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

int globalGameLog;
int globalGameDebugLog;
int globalGameWarningLog;
int globalGameErrorLog;

void ss_game_initialize( void )
{
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

	cache_clutter();
}

ApeWorld *ss_game_get_current_world( void )
{
	return currentWorld;
}

void ss_game_spawn_world( ApeWorld *world )
{
	if ( currentWorld != NULL )
	{
		acl_level_destroy( currentWorld );
		currentWorld = NULL;
	}

	currentWorld = world;
	game_modeInterface->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD, world );
}

void ss_game_tick( void )
{
	COM_PROFILE_FUNCTION_START();

	game_modeInterface->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_TICK, NULL );

	COM_PROFILE_FUNCTION_END();
}

void ss_game_disconnect( void )
{
	game_modeInterface->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_DISCONNECT, NULL );
}

void ss_game_register_standard_entity_components_( void )
{
}

static GameConnectionType gameConnectionType = GAME_CONNECTION_LOCAL;

void Game_SetConnection( const GameConnectionType connectionType )
{
	if ( gameConnectionType != GAME_CONNECTION_NONE )
	{
	}

	gameConnectionType = connectionType;
}

GameConnectionType gameGetConnectionType( void )
{
	return gameConnectionType;
}
