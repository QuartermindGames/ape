// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "ape_private.h"
#include "game/game_public.h"
#include "world/world.h"

#include "client/client.h"
#include "renderer/renderer.h"

#include "server/server.h"

#include "yin/core_fs.h"

const ApeGameInterfaceImport *ape_gameInterface;

void ape_initialize_game_( void )
{
	ape_console_print_( "Initializing game...\n" );

	ape_gameInterface = ape_game_get_interface();
	if ( ape_gameInterface == nullptr )
	{
		ape_console_error_( true, "Failed to get game interface!\n" );
	}
	else if ( ape_gameInterface->version != APE_GAME_INTERFACE_VERSION )
	{
		ape_console_error_( true, "Unsupported game interface version (%u != %u)!\n", ape_gameInterface->version, APE_GAME_INTERFACE_VERSION );
	}
	else if ( *ape_gameInterface->identifier == '\0' )
	{
		ape_console_error_( true, "No identifier provided for game interface!\n" );
	}

	if ( ape_gameInterface->initialize != nullptr && !ape_gameInterface->initialize() )
	{
		ape_console_error_( true, "Failed to initialize game!\n" );
	}

	ape_console_print_( "Game initialized!\n" );
}

void ape_shutdown_game_( void )
{
	if ( ape_gameInterface->shutdown != nullptr )
	{
		ape_gameInterface->shutdown();
	}
}

static void sync_world_nodes( ApeWorldNode *worldNode )
{
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
