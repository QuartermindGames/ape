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

static float gameTimeModifier = 1.0f;

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

	PlRegisterConsoleVariable( "game.timeModifier", "Time modifier, useful for emulating slow-motion.", "1.0", PL_VAR_F32, &gameTimeModifier, nullptr, false );

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

double game_get_time_delta_( double delta )
{
	return delta * gameTimeModifier;
}

ApeWorld *game_get_current_world( void )
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
		ape_gameInterface->spawnWorld( room );
	}
}

const char *game_get_identifier()
{
	return ape_gameInterface->identifier;
}

extern ApeEntityClassDefinition game_playerSpawnEntityClass_;
extern ApeEntityClassDefinition game_triggerEntityClass_;
extern ApeEntityClassDefinition game_ropeEntityClass_;
extern ApeEntityClassDefinition game_portalEntityClass_;

extern ApeEntityComponentDefinition game_collisionComponent_;
extern ApeEntityComponentDefinition game_healthComponent_;
extern ApeEntityComponentDefinition game_movementComponent_;
extern ApeEntityComponentDefinition game_inventoryComponent_;

void game_register_standard_entity_components_( void )
{
	ape_register_entity_class( &game_playerSpawnEntityClass_ );
	ape_register_entity_class( &game_triggerEntityClass_ );
	ape_register_entity_class( &game_ropeEntityClass_ );
	ape_register_entity_class( &game_portalEntityClass_ );

	ape_register_entity_component( &game_collisionComponent_ );
	ape_register_entity_component( &game_healthComponent_ );
	ape_register_entity_component( &game_movementComponent_ );
	ape_register_entity_component( &game_inventoryComponent_ );
}

AcmBranch *game_get_config()
{
	return gameConfig;
}
