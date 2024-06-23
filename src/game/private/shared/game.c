// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeWorld *currentWorld = NULL;

/////////////////////////////////////////////////////////////////////////////////////
// Public

static int globalGameLog;
static int globalGameDebugLog;
static int globalGameWarningLog;
static int globalGameErrorLog;

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

void game_error_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( globalGameErrorLog, buf );
}

void game_server_initialize_();
void game_client_initialize_();
bool game_initialize( void )
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

	game_server_initialize_();
	game_client_initialize_();

	if ( !ape_gameInterface->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_INITIALIZE, nullptr ) )
	{
		game_error_( "Failed to initialize game sub-system!\n" );
		return false;
	}

	return true;
}

ApeWorld *ss_game_get_current_world( void )
{
	return currentWorld;
}

void game_spawn_world( ApeWorld *world )
{
	if ( currentWorld != NULL )
	{
		ape_world_node_destroy( ape_world_get_world_node( currentWorld ) );
		currentWorld = NULL;
	}

	currentWorld = world;
}

const char *game_get_identifier()
{
	return ape_gameInterface->identifier;
}

extern ApeEntityClassDefinition game_entityLightClass;

void ss_game_register_standard_entity_components_( void )
{
	ape_register_entity_class( &game_entityLightClass );
}
