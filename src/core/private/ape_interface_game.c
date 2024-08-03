// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "game/game_public.h"
#include "world/world.h"

#include "client/ape_client.h"
#include "client/renderer/renderer.h"

#include "server/server.h"

#include "yin/core_fs.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static void world_command( unsigned int argc, char **argv )
{
	PLPath path;
	PlSetupPath( path, true, "worlds/%s.wld.n", argv[ 1 ] );

	ape_spawn_world_( path );
}

static void print_world_name( const char *path, void * )
{
	const char *name = ( name = strrchr( path, '/' ) ) != nullptr ? name + 1 : path;
	ape_print_( "%s\n", name );
}

static void list_worlds_command( uint, char ** )
{
	PlScanDirectory( "worlds", "n", print_world_name, false, nullptr );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

const ApeGameInterfaceImport *ape_gameInterface;

void ape_initialize_game_( void )
{
	ape_print_( "Initializing game...\n" );

	PlRegisterConsoleCommand( "world", "Load in and spawn the specified world.", 1, world_command );
	PlRegisterConsoleCommand( "list_worlds", "List all of the available worlds.", 0, list_worlds_command );

	ape_gameInterface = ape_game_get_interface();
	if ( ape_gameInterface == nullptr )
	{
		ape_error_( true, "Failed to get game interface!\n" );
	}
	else if ( ape_gameInterface->version != APE_GAME_INTERFACE_VERSION )
	{
		ape_error_( true, "Unsupported game interface version (%u != %u)!\n", ape_gameInterface->version, APE_GAME_INTERFACE_VERSION );
	}
	else if ( *ape_gameInterface->identifier == '\0' )
	{
		ape_error_( true, "No identifier provided for game interface!\n" );
	}

	if ( !game_initialize() )
	{
		ape_error_( true, "Failed to initialize game!\n" );
	}

	ape_print_( "Game initialized!\n" );
}

void ape_shutdown_game_( void )
{
	const ApeGameInterfaceImport *interface = ape_game_get_interface();
	if ( interface == nullptr )
	{
		return;
	}

	interface->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_SHUTDOWN, nullptr );
}

void ape_tick_game_server_( void )
{
	ape_draw_debug_clear_();

	ApeWorld *world = ss_game_get_current_world();
	if ( world != nullptr )
	{
		ape_calc_world_node_bounds( &world->base );
	}

	ape_build_camera_visibility_lists_();

	const ApeGameInterfaceImport *game = ape_game_get_interface();
	game->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_TICK_SERVER, NULL );
}

void ape_spawn_world_( const char *worldPath )
{
	ApeWorld *world = ape_world_load( worldPath );
	if ( world == nullptr )
	{
		ape_warning_( "Failed to load world, aborting game spawn!\n" );
		return;
	}

	game_spawn_world( world );

	ape_world_spawn_entities_( world );

	ape_server_start( "localhost", 0 );
	ape_initiate_client_connection_( "localhost", ape_server_get_port_() );
}
