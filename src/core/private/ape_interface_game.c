// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "game/game_interface.h"
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

/////////////////////////////////////////////////////////////////////////////////////
// Public

const ApeGameInterfaceImport *game_modeInterface;

void ape_initialize_game_( void )
{
	PRINT( "Initializing Game...\n" );

	PlRegisterConsoleCommand( "world", "Load in and spawn the specified world.", 1, world_command );

	ss_game_initialize();

	game_modeInterface = ape_game_get_interface();
	if ( game_modeInterface == nullptr )
	{
		ape_error_( true, "Failed to get game interface!\n" );
	}

	if ( game_modeInterface->version != APE_GAME_INTERFACE_VERSION )
	{
		ape_error_( true, "Unsupported game interface version!\n" );
	}

	if ( !game_modeInterface->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_INITIALIZE, nullptr ) )
	{
		ape_error_( true, "Failed to initialize game sub-system!\n" );
	}

	PRINT( "Game initialized!\n" );
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

void ape_tick_game_( void )
{
	ss_game_tick();
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
