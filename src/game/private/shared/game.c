// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"
#include "integrations/integrations.h"

#include "menu/menu.h"

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

/////////////////////////////////////////////////////////////////////////////////////

extern ApeEntityClassDefinition game_pathEntityClass_;
extern ApeEntityClassDefinition game_playerSpawnEntityClass_;
extern ApeEntityClassDefinition game_triggerEntityClass_;
extern ApeEntityClassDefinition game_ropeEntityClass_;
extern ApeEntityClassDefinition game_portalEntityClass_;
extern ApeEntityClassDefinition game_terrainEntityClass_;
#if defined( GAME_SS1 )
extern ApeEntityClassDefinition ss1_airshipEntityClass;
extern ApeEntityClassDefinition ss1_pawnEntityClass;
extern ApeEntityClassDefinition ss1_playerEntityClass;
#endif
#if defined( GAME_QM2 )
extern ApeEntityClassDefinition game_qm2_creatureEntityClass_;
#endif

extern ApeEntityComponentDefinition game_collisionComponent_;
extern ApeEntityComponentDefinition game_healthComponent_;
extern ApeEntityComponentDefinition game_movementComponent_;
extern ApeEntityComponentDefinition game_inventoryComponent_;

static void register_standard_entity_components()
{
	ape_register_entity_class( &game_pathEntityClass_ );
	ape_register_entity_class( &game_playerSpawnEntityClass_ );
	ape_register_entity_class( &game_triggerEntityClass_ );
	ape_register_entity_class( &game_ropeEntityClass_ );
	ape_register_entity_class( &game_portalEntityClass_ );
	ape_register_entity_class( &game_terrainEntityClass_ );
#if defined( GAME_SS1 )
	ape_register_entity_class( &ss1_airshipEntityClass );
	ape_register_entity_class( &ss1_pawnEntityClass );
	ape_register_entity_class( &ss1_playerEntityClass );
	ape_register_entity_class( &game_pathEntityClass_ );
#endif
#if defined( GAME_QM2 )
	ape_register_entity_class( &game_qm2_creatureEntityClass_ );
#endif

	ape_register_entity_component( &game_collisionComponent_ );
	ape_register_entity_component( &game_healthComponent_ );
	ape_register_entity_component( &game_movementComponent_ );
	ape_register_entity_component( &game_inventoryComponent_ );
}

static void navigate_world_tree( const ApeWorldNode *node, const unsigned int depth )
{
	for ( unsigned int i = 0; i < depth; ++i )
	{
		game_print_( "\t" );
	}
	game_print_( "%s (%s)\n", node->classType->identifier, *node->name == '\0' ? "none" : node->name );

	const ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		navigate_world_tree( child, depth + 1 );
	}
}

static void print_world_tree_command( unsigned int argc, char **argv )
{
	ApeWorld *world = game_get_current_world();
	if ( world == nullptr )
	{
		game_print_( "No world loaded!\n" );
		return;
	}

	navigate_world_tree( APE_WORLD_NODE( world ), 0 );
}

void game_server_initialize_();
void game_client_initialize_();
void game_language_initialize_();
bool game_initialize( void )
{
	gameConfig = com_get_config( "game_shared" );

	PlRegisterConsoleVariable( "game.timeModifier", "Time modifier, useful for emulating slow-motion.", "1.0", PL_VAR_F32, &gameTimeModifier, nullptr, false );

	PlRegisterConsoleCommand( "game_print_world_tree", "Prints out the current world tree structure.", 0, print_world_tree_command );

	globalGameLog        = PlAddLogLevel( "game", PL_COLOUR_WHITE, acm_get_bool( gameConfig, "log", true ) );
	globalGameWarningLog = PlAddLogLevel( "game.warning", PL_COLOUR_YELLOW, acm_get_bool( gameConfig, "logWarning", true ) );
	globalGameErrorLog   = PlAddLogLevel( "game.error", PL_COLOUR_RED, acm_get_bool( gameConfig, "logError", true ) );

	globalGameDebugLog = PlAddLogLevel( "game.debug", PL_COLOUR_WHITE_SMOKE,
#if !defined( NDEBUG )
	                                    true
#else
	                                    false
#endif
	);

	game_language_initialize_();
	game_server_initialize_();
	game_client_initialize_();

	register_standard_entity_components();

	static constexpr int64_t DISCORD_CLIENT_ID = 822170320169074719;
	game_integrations_discord_initialize_( DISCORD_CLIENT_ID );
	game_integrations_discord_update_activity_( G_STR_( "Idling" ), nullptr, "qm1-logo", "Temp" );

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

double game_get_delta_mod_( double delta )
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

	// a world is being loaded, skip the splash screens!
	game_menu_splash_cleanup_();

	if ( ape_gameInterface->spawnWorld != nullptr )
	{
		ape_gameInterface->spawnWorld( room );
	}
}

const char *game_get_identifier()
{
	return ape_gameInterface->identifier;
}

AcmBranch *game_get_config()
{
	return gameConfig;
}
