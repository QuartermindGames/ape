// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "game/game_public.h"
#include "world/world.h"

#include "client/ape_client.h"
#include "renderer/renderer.h"

#include "server/server.h"

#include "yin/core_fs.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static void load_room_command( PL_UNUSED unsigned int argc, char **argv )
{
	PLPath path;
	PlSetupPath( path, true, "rooms/%s." APE_WORLD_ROOM_EXTENSION, argv[ 1 ] );

	ape_spawn_world_( path );
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
	ape_print_( "%s\n", name );
}

static void list_rooms_command( unsigned int, char ** )
{
	PlScanDirectory( "rooms", "n", print_world_name, true, nullptr );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

const ApeGameInterfaceImport *ape_gameInterface;

void ape_initialize_game_( void )
{
	ape_print_( "Initializing game...\n" );

	PlRegisterConsoleCommand( "game_load_room", "Load in and spawn the specified room.", 1, load_room_command );
	PlRegisterConsoleCommand( "game_list_rooms", "List all of the available worlds.", 0, list_rooms_command );

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

static void sync_world_nodes( ApeWorldNode *worldNode )
{
	COM_PROFILE_FUNCTION_START();

	ApeWorldNode *childNode;
	COM_ITERATE_LINKED_LIST( childNode, worldNode->children, i )
	{
		if ( childNode->needsSyncOnTick )
		{
			unsigned int length;

			// serialize all the base crap here first


			// now serialize all the class-specific crap
			if ( childNode->classType->netSerializeFunction != nullptr )
			{
				void *ptr = childNode->classType->netSerializeFunction( childNode, &length );
			}
		}

		sync_world_nodes( childNode );
	}

	COM_PROFILE_FUNCTION_END();
}

static bool tick_room_decals( ApeWorldNode *node, void *user )
{
	assert( ape_world_node_is_valid( node, APE_WORLD_NODE_TYPE_ROOM ) );

	ApeRoom *room = ( ApeRoom * ) node;
	if ( room->decalManager == nullptr )
	{
		return true;
	}

	ape_decal_manager_tick_( room->decalManager, *( double * ) user );

	return true;
}

void ape_tick_game_server_( double delta )
{
	COM_PROFILE_FUNCTION_START();

	ApeWorld *world = game_get_current_world();
	if ( world != nullptr )
	{
		ape_world_node_compute_bounds_( &world->base );
		ape_world_tick_entities_( world, delta );

		// and now we need to tick the decals for each room,
		// given each has its own decal manager
		ape_world_node_visit_children( APE_WORLD_NODE( world ), APE_WORLD_NODE_TYPE_ROOM, false, tick_room_decals, &delta );

		// send any updates from the server to the clients
		sync_world_nodes( APE_WORLD_NODE( world ) );
	}

	if ( ape_gameInterface->serverTick != nullptr )
	{
		ape_gameInterface->serverTick( delta );
	}

	COM_PROFILE_FUNCTION_END();
}

void ape_spawn_world_( const char *path )
{
	ApeWorld *world = ape_world_create();
	assert( world != nullptr );

	ApeWorldNode *roomNode = ape_world_node_load( APE_WORLD_NODE( world ), path );
	if ( roomNode == nullptr )
	{
		ape_world_node_destroy( APE_WORLD_NODE( world ) );
		return;
	}

	game_spawn_world( world, ( ApeRoom * ) roomNode );

	ape_world_spawn_entities_( world );

	ape_server_start( "localhost", 0 );
	ape_initiate_client_connection_( "localhost", ape_server_get_port_() );
}
