// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"
#include "game_server.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static PLHashTable *serverClients;//GameServerClient

/////////////////////////////////////////////////////////////////////////////////////
// Public

void game_server_initialize_()
{
	serverClients = PlCreateHashTable();
}

void game_server_client_connected_( ApeServerClientHandle *clientHandle )
{
	GameServerClient *serverClient = PL_NEW( GameServerClient );
	PlInsertHashTableNode( serverClients, clientHandle, sizeof( ApeServerClientHandle * ), serverClient );
	serverClient->internalHandle = clientHandle;
}

void game_server_client_disconnected_( ApeServerClientHandle *clientHandle )
{
	GameServerClient *serverClient = PlLookupHashTableUserData( serverClients, clientHandle, sizeof( ApeServerClientHandle * ) );
	assert( serverClient != nullptr );
	PlDestroyHashTableNode( serverClient->hashTableNode );
	PL_DELETE( serverClient );
}

void game_server_process_message_( ApeServerClientHandle *clientHandle, const void *buf, size_t bufSize )
{
	const GameNetMessageHeader *header = ( const GameNetMessageHeader * ) buf;
	switch ( header->type )
	{
		case GAME_NET_MESSAGE_SAY:
		{
			PLHashTableNode *hashNode = PlGetFirstHashTableNode( serverClients );
			while ( hashNode != nullptr )
			{
				GameServerClient *otherClient = PlGetHashTableNodeUserData( hashNode );
				hashNode                      = PlGetNextHashTableNode( hashNode );
				if ( otherClient->internalHandle == clientHandle )
				{
					continue;
				}

				ape_server_send( otherClient->internalHandle, ( const void *[] ){ buf }, ( size_t[] ){ bufSize }, 1 );
			}
			break;
		}
	}
}

bool game_server_send_message_( ApeServerClientHandle *clientHandle, GameNetMessageType type, const void *buf, size_t bufSize )
{
	const void *items[] = {
	        &( GameNetMessageHeader ){ .type = type },
	        buf,
	};
	size_t sizes[] = {
	        sizeof( GameNetMessageHeader ),
	        bufSize,
	};

	return ape_server_send( clientHandle, items, sizes, PL_ARRAY_ELEMENTS( items ) );
}
