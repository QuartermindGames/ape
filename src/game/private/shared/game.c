// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeWorld *currentWorld;

static AcmBranch *gameConfig;

static int globalGameLog;
static int globalGameDebugLog;
static int globalGameWarningLog;
static int globalGameErrorLog;

/////////////////////////////////////////////////////////////////////////////////////
// Public

void game_print_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( globalGameLog, buf );
}

#if !defined( NDEBUG )
void game_debug_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( globalGameDebugLog, buf );
}
#endif

void game_warning_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( globalGameWarningLog, buf );
}

#if !defined( NDEBUG )
#	include <signal.h>
#endif

void game_error_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( globalGameErrorLog, buf );

#if !defined( NDEBUG )
	raise( SIGINT );
#endif
}

void game_server_initialize_();
void game_client_initialize_();
void game_language_initialize_();
bool game_initialize( void )
{
	gameConfig = com_get_config( "game_shared" );

	globalGameLog        = PlAddLogLevel( "game", PL_COLOUR_WHITE, acm_get_bool( gameConfig, "log", true ) );
	globalGameWarningLog = PlAddLogLevel( "game/warning", PL_COLOUR_YELLOW, acm_get_bool( gameConfig, "logWarning", true ) );
	globalGameErrorLog   = PlAddLogLevel( "game/error", PL_COLOUR_RED, acm_get_bool( gameConfig, "logError", true ) );

	globalGameDebugLog = PlAddLogLevel( "game/debug", PL_COLOUR_WHITE_SMOKE,
#if !defined( NDEBUG )
	                                    true
#else
	                                    false
#endif
	);

	game_language_initialize_();
	game_server_initialize_();
	game_client_initialize_();

	if ( !ape_gameInterface->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_INITIALIZE, nullptr ) )
	{
		game_error_( "Failed to initialize game sub-system!\n" );
		return false;
	}

	return true;
}

void game_language_shutdown_();
void game_shutdown()
{
	game_language_shutdown_();
}

ApeWorld *ss_game_get_current_world( void )
{
	return currentWorld;
}

void game_spawn_world( ApeWorld *world, ApeRoom *room )
{
	if ( currentWorld != NULL )
	{
		ape_world_node_destroy( ( ApeWorldNode * ) currentWorld );
		currentWorld = nullptr;
	}

	currentWorld = world;

	if ( ape_gameInterface->spawnWorld != nullptr )
	{
		ape_gameInterface->spawnWorld( world, room );
	}
}

const char *game_get_identifier()
{
	return ape_gameInterface->identifier;
}

extern ApeEntityClassDefinition game_triggerEntityClass;

void game_register_standard_entity_components_( void )
{
	ape_register_entity_class( &game_triggerEntityClass );
}

AcmBranch *game_get_config()
{
	return gameConfig;
}
