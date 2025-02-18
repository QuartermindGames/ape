// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"
#include "game_server.h"

static PLHashTable *serverClientsLookup;//GameServerClient

static GameServerClient serverClients[ GAME_MAX_CLIENTS ];
static unsigned int     numServerClients;

static GamePlayer   players[ GAME_MAX_PLAYERS ];
static unsigned int numPlayers;

void game_server_initialize_()
{
	serverClientsLookup = PlCreateHashTable();

	PL_ZERO_( serverClients );
}

bool game_server_client_validate_( PL_UNUSED ApeServerClientHandle *clientHandle )
{
	if ( numServerClients >= GAME_MAX_CLIENTS )
	{
		game_warning_( "Max clients limit hit, rejecting new client!\n" );
		return false;
	}

	// if it's over the player limit, they'll get put in some sort of
	// lobby-state, so we skip that check here...

	return true;
}

void game_server_client_connected_( ApeServerClientHandle *clientHandle )
{
	GameServerClient *serverClient = &serverClients[ numServerClients ];
	serverClient->slot             = numServerClients;
	PlInsertHashTableNode( serverClientsLookup, clientHandle, sizeof( ApeServerClientHandle * ), serverClient );
	serverClient->internalHandle = clientHandle;
	numServerClients++;
}

void game_server_client_disconnected_( ApeServerClientHandle *clientHandle )
{
	GameServerClient *serverClient = PlLookupHashTableUserData( serverClientsLookup, clientHandle, sizeof( ApeServerClientHandle * ) );
	assert( serverClient != nullptr );
	PlDestroyHashTableNode( serverClient->hashTableNode );
	serverClient->slot = 0;
}

void game_server_process_message_( ApeServerClientHandle *clientHandle, const void *buf, size_t bufSize )
{
	const GameNetMessageHeader *header = buf;
	switch ( header->type )
	{
		default:
			game_warning_( "Unhandled client message (%u)!\n", header->type );
			break;
		case GAME_NET_MESSAGE_SAY:
		{
			GameServerClient *otherClient;
			COM_ITERATE_HASHED_LIST( otherClient, serverClientsLookup, i )
			{
				ape_server_send( otherClient->internalHandle, ( const void *[] ) { buf }, ( size_t[] ) { bufSize }, 1 );
			}
			break;
		}
	}
}

bool game_server_send_message_( ApeServerClientHandle *clientHandle, GameNetMessageType type, const void *buf, size_t bufSize )
{
	const void *items[] = {
	        &( GameNetMessageHeader ) { .type = type },
	        buf,
	};
	size_t sizes[] = {
	        sizeof( GameNetMessageHeader ),
	        bufSize,
	};

	return ape_server_send( clientHandle, items, sizes, PL_ARRAY_ELEMENTS( items ) );
}

GameServerClient *game_server_get_host_client_()
{
	if ( ape_is_dedicated() )
	{
		return nullptr;
	}

	// this assumes the first slot is the host
	// if it's not dedicated... which might be dumb
	return game_server_get_client_( 0 );
}

GamePlayer *game_server_get_host_player_()
{
	GameServerClient *client = game_server_get_host_client_();
	if ( client == nullptr )
	{
		return nullptr;
	}

	return client->playerSlot;
}

GameServerClient *game_server_get_client_( unsigned int slot )
{
	assert( slot < GAME_MAX_CLIENTS );
	return &serverClients[ slot ];
}

unsigned int game_server_get_num_clients_()
{
	return numServerClients;
}

unsigned int game_server_get_num_players_()
{
	return numPlayers;
}

void game_server_broadcast_message_( GameNetMessageType type, const void *buf, size_t bufSize )
{
	const void *items[] = {
	        &( GameNetMessageHeader ) { .type = type },
	        buf,
	};
	size_t sizes[] = {
	        sizeof( GameNetMessageHeader ),
	        bufSize,
	};

	PLHashTableNode *hashNode = PlGetFirstHashTableNode( serverClientsLookup );
	while ( hashNode != nullptr )
	{
		GameServerClient *client = PlGetHashTableNodeUserData( hashNode );
		hashNode                 = PlGetNextHashTableNode( hashNode );
		if ( !ape_server_send( client->internalHandle, items, sizes, PL_ARRAY_ELEMENTS( items ) ) )
		{
			game_warning_( "Failed to send message to client %p!\n", client->internalHandle );
		}
	}
}

void game_server_print_( ApeServerClientHandle *clientHandle, const char *message )
{
	size_t messageSize = strlen( message );
	if ( messageSize > GAME_NET_MAX_SAY_MESSAGE )
	{
		game_warning_( "Invalid say message length (%u > %u)!\n", messageSize, GAME_NET_MAX_SAY_MESSAGE );
		return;
	}

	game_print_( "%s", message );

	if ( clientHandle == nullptr )
	{
		game_server_broadcast_message_( GAME_NET_MESSAGE_ANNOUNCE, message, strlen( message ) );
		return;
	}

	game_server_send_message_( clientHandle, GAME_NET_MESSAGE_ANNOUNCE, message, strlen( message ) );
}
