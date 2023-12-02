// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

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
	ss_acl_spawn_world_( argv[ 1 ] );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

const SSGameModeInterface *game_modeInterface;

void ss_acl_initialize_game_( void )
{
	PRINT( "Initializing Game...\n" );

	PlRegisterConsoleCommand( "world", "Load in and spawn the specified world.", 1, world_command );

	ss_game_initialize();

	game_modeInterface = ss_game_mode_get_interface();
	if ( game_modeInterface == NULL )
		PRINT_ERROR( "Failed to get game interface!\n" );

	if ( !game_modeInterface->requestCallbackMethod( GAMEMODE_REQUEST_INITIALIZE, NULL ) )
		PRINT_ERROR( "Failed to initialize game sub-system!\n" );

	PRINT( "Game initialized!\n" );
}

void ss_acl_shutdown_game_( void )
{
	const SSGameModeInterface *interface = ss_game_mode_get_interface();
	assert( interface != NULL );
	if ( interface == NULL )
		return;

	interface->requestCallbackMethod( GAMEMODE_REQUEST_SHUTDOWN, NULL );
}

void ss_acl_tick_game_( void )
{
	ss_game_tick();
}

void ss_acl_spawn_world_( const char *worldPath )
{
	ApeWorld *world = ss_acl_level_load( worldPath );
	if ( world == NULL )
	{
		PRINT_WARNING( "Failed to load world, aborting game spawn!\n" );
		return;
	}

	ss_game_spawn_world( world );

	ss_acl_world_spawn_entities_( world );

	ss_acl_start_server_( "localhost", 0 );
	ss_acl_initiate_client_connection_( "localhost", ss_acl_server_get_port_() );
}
