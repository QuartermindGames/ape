// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

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

static void register_standard_entity_components()
{
	extern ApeEntityClassDefinition game_pathEntityClass_;
	extern ApeEntityClassDefinition game_playerSpawnEntityClass_;
	extern ApeEntityClassDefinition game_triggerEntityClass_;
	extern ApeEntityClassDefinition game_ropeEntityClass_;
	extern ApeEntityClassDefinition game_portalEntityClass_;
	extern ApeEntityClassDefinition game_terrainEntityClass_;

	ape_register_entity_class( &game_pathEntityClass_ );
	ape_register_entity_class( &game_playerSpawnEntityClass_ );
	ape_register_entity_class( &game_triggerEntityClass_ );
	ape_register_entity_class( &game_ropeEntityClass_ );
	ape_register_entity_class( &game_portalEntityClass_ );
	ape_register_entity_class( &game_terrainEntityClass_ );

	extern ApeEntityComponentDefinition game_cameraComponent_;
	extern ApeEntityComponentDefinition game_collisionComponent_;
	extern ApeEntityComponentDefinition game_healthComponent_;
	extern ApeEntityComponentDefinition game_movementComponent_;
	extern ApeEntityComponentDefinition game_inventoryComponent_;

	ape_register_entity_component( &game_cameraComponent_ );
	ape_register_entity_component( &game_collisionComponent_ );
	ape_register_entity_component( &game_healthComponent_ );
	ape_register_entity_component( &game_movementComponent_ );
	ape_register_entity_component( &game_inventoryComponent_ );
}

static void load_room_command( [[maybe_unused]] unsigned int argc, char **argv )
{
	ApeWorld *world = ape_world_create();
	assert( world != nullptr );

	ApeWorldNode *roomNode = ape_world_node_load( APE_WORLD_NODE( world ), argv[ 1 ] );
	if ( roomNode == nullptr )
	{
		ape_world_node_destroy( APE_WORLD_NODE( world ) );
		return;
	}

	// a world is being loaded via command, skip the splash screens and menu!
	game_menu_set_active( nullptr );
	game_menu_splash_cleanup_();

	game_spawn_world( world, ( ApeRoom * ) roomNode );
}

static void print_world_name( const char *path, void * )
{
	// verify it's a valid world
	if ( strcmp( &path[ strlen( path ) - 6 ], "." APE_WORLD_ROOM_EXTENSION ) != 0 )
	{
		return;
	}

	//TODO: just print the name of the world itself?

	const char *name = ( name = strrchr( path, '/' ) ) != nullptr ? name + 1 : path;
	game_print_( "%s\n", name );
}

static void list_rooms_command( unsigned int, char ** )
{
	PlScanDirectory( "rooms", "n", print_world_name, true, nullptr );
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
bool game_initialize_( void )
{
	gameConfig = com_get_config( "game_shared" );

	ape_console_var_register( "game.timeModifier", "Time modifier, useful for emulating slow-motion.", "1.0", PL_VAR_F32, &gameTimeModifier, nullptr, APE_CONSOLE_VAR_FLAG_CHEAT );
	ape_console_var_register( "game.mode",
	                          "Set the specific game mode for the server. Shouldn't be changed after the server is already spawned!",
	                          "0", PL_VAR_I32,
	                          nullptr, nullptr, 0 );

	PlRegisterConsoleCommand( "game_load_room", "Load in and spawn the specified room.", 1, load_room_command );
	PlRegisterConsoleCommand( "game_list_rooms", "List all of the available worlds.", 0, list_rooms_command );
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

	return true;
}

void game_language_shutdown_();
void game_shutdown_()
{
	game_language_shutdown_();
	game_integrations_discord_shutdown_();
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

	if ( ape_gameInterface->spawnWorld != nullptr )
	{
		ape_gameInterface->spawnWorld( room );
	}

	ape_world_spawn_entities( world );

	ape_server_start( "localhost", 0 );
	ape_client_connect( "localhost", ape_server_get_port() );
}

const char *game_get_identifier()
{
	return ape_gameInterface->identifier;
}

AcmBranch *game_get_config()
{
	return gameConfig;
}
