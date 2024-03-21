// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Server implementation.

#include "ape_private.h"

#include <yin/core_game.h>

#include "server.h"

#define SERVER_CLIENT_TIMEOUT 1024

static SSAclNetSocket *hostSocket = NULL;

typedef enum ServerClientState
{
	SERVER_CLIENT_STATE_ACCEPTED,   /* connection has been established */
	SERVER_CLIENT_STATE_VALIDATING, /* pending validation */
	SERVER_CLIENT_STATE_VALIDATED,  /* connection validated */
} ServerClientState;

typedef struct ApeServerClient
{
	SSAclNetSocket *netSocket;
	PLLinkedListNode *node;

	ServerClientState state;

	char receiveBuffer[ PROTOCOL_MESSAGESIZE ];
	size_t receivedBytes;

	unsigned int lastMessageTick;
} ApeServerClient;
static PLLinkedList *connectedClients = NULL;

static void drop_client_callback( void *userData, bool *breakEarly )
{
	ape_server_drop_client_( ( ApeServerClient * ) userData );
}

bool ape_server_start( const char *ip, unsigned short port )
{
	PRINT( "Opening socket: %s:" PL_FMT_uint16 "\n", ip, port );

	hostSocket = ape_net_open_socket_( ip, port, true );
	if ( hostSocket == NULL )
	{
		PRINT_WARNING( "Failed to open server socket!\n" );
		return false;
	}

	PRINT( "APE %s server active, listening for clients...\n", ENGINE_VERSION_STR );

	return true;
}

void ape_initialize_server_( void )
{
	connectedClients = PlCreateLinkedList();
	if ( connectedClients == NULL )
	{
		PRINT_ERROR( "Failed to create connected clients list: %s\n", PlGetError() );
	}
}

void ape_shutdown_server_( void )
{
	/* drop all connected clients */
	PlIterateLinkedList( connectedClients, drop_client_callback, true );
}

void ape_server_drop_client_( ApeServerClient *serverClient )
{
	PRINT( "Dropping client...\n" );

	ape_net_close_socket_( serverClient->netSocket );
	PlDestroyLinkedListNode( serverClient->node );
	PlFree( serverClient );
}

static void process_client_message( ApeServerClient *client, const void *buf )
{
}

static void tick_server_client( void *userData, bool *breakEarly )
{
	ApeServerClient *serverClient = ( ApeServerClient * ) userData;

	ssize_t r = ss_acl_net_receive_( serverClient->netSocket,
	                                 serverClient->receiveBuffer + serverClient->receivedBytes,
	                                 sizeof( serverClient->receiveBuffer ) - serverClient->receivedBytes );
	if ( r == -1 )
	{
		ape_server_drop_client_( serverClient );
		return;
	}
	else if ( r > 0 )
	{
		serverClient->lastMessageTick = ape_get_num_ticks();
	}

	serverClient->receivedBytes += r;

	if ( serverClient->receivedBytes >= sizeof( ApeProtocolMessageHeader ) )
	{
		const ApeProtocolMessageHeader *messageHeader = ( ApeProtocolMessageHeader * ) serverClient->receiveBuffer;

		uint32_t l = messageHeader->length;
		if ( serverClient->receivedBytes >= l )
		{
			/* process message */
			process_client_message( serverClient, serverClient->receiveBuffer );

			memmove( serverClient->receiveBuffer, serverClient->receiveBuffer + l, serverClient->receivedBytes - l );
			serverClient->receivedBytes -= l;
		}
		else if ( messageHeader->length > PROTOCOL_MESSAGESIZE )
		{
			/* boom */
			PRINT_WARNING( "Client sent a message of an invalid length: %u/%u\n", messageHeader->length, PROTOCOL_MESSAGESIZE );
			ape_server_drop_client_( serverClient );
		}
	}
}

void ape_server_tick_( void )
{
	COM_PROFILE_FUNCTION_START();

	if ( hostSocket != NULL )
	{ /* check if a new connection is being established */
		SSAclNetSocket *connectedSocket = ss_acl_net_accept_( hostSocket );
		if ( connectedSocket != NULL )
		{
			ApeServerClient *serverClient = PlMAllocA( sizeof( ApeServerClient ) );
			serverClient->netSocket = connectedSocket;
			serverClient->node = PlInsertLinkedListNode( connectedClients, serverClient );
			/* validation still needs to be performed */
			PRINT( "Client connected, awaiting validation...\n" );
		}

		PlIterateLinkedList( connectedClients, tick_server_client, true );
	}

	ape_tick_game_();

	COM_PROFILE_FUNCTION_END();
}

/**
 * Return port of local server instance.
 */
unsigned short ape_server_get_port_( void )
{
	assert( hostSocket != NULL );
	if ( hostSocket == NULL )
	{
		return 0;
	}

	return ss_acl_net_get_local_port_( hostSocket );
}
