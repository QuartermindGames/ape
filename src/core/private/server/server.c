/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "ape_private.h"

#include <yin/core_game.h>

#include "server.h"

#define SERVER_CLIENT_TIMEOUT 1024

static NetSocket *hostSocket = NULL;

typedef enum ServerClientState
{
	SERVER_CLIENT_STATE_ACCEPTED,   /* connection has been established */
	SERVER_CLIENT_STATE_VALIDATING, /* pending validation */
	SERVER_CLIENT_STATE_VALIDATED,  /* connection validated */
} ServerClientState;

typedef struct ApeServerClient
{
	NetSocket *netSocket;
	PLLinkedListNode *node;

	ServerClientState state;

	char receiveBuffer[ PROTOCOL_MESSAGESIZE ];
	size_t receivedBytes;

	unsigned int lastMessageTick;
} ApeServerClient;
static PLLinkedList *connectedClients = NULL;

static void DropClientCallback( void *userData, bool *breakEarly )
{
	apeDropServerClient( ( ApeServerClient * ) userData );
}

bool apeStartServer( const char *ip, unsigned short port )
{
	PRINT( "Opening socket: %s:" PL_FMT_uint16 "\n", ip, port );

	hostSocket = Net_OpenSocket( ip, port, true );
	if ( hostSocket == NULL )
	{
		PRINT_WARNING( "Failed to open server socket!\n" );
		return false;
	}

	PRINT( "==============================================\n" );
	PRINT( "YIN %s SERVER ACTIVE, LISTENING FOR CLIENTS\n", ENGINE_VERSION_STR );
	PRINT( "==============================================\n" );

	return true;
}

void apeInitializeServer( void )
{
	connectedClients = PlCreateLinkedList();
	if ( connectedClients == NULL )
	{
		PRINT_ERROR( "Failed to create connected clients list: %s\n", PlGetError() );
	}
}

void apeShutdownServer( void )
{
	/* drop all connected clients */
	PlIterateLinkedList( connectedClients, DropClientCallback, true );
}

void apeDropServerClient( ApeServerClient *serverClient )
{
	PRINT( "Dropping client...\n" );

	Net_CloseSocket( serverClient->netSocket );
	PlDestroyLinkedListNode( serverClient->node );
	PlFree( serverClient );
}

static void ProcessClientMessage( ApeServerClient *client, const void *buf )
{
}

static void TickServerClient( void *userData, bool *breakEarly )
{
	ApeServerClient *serverClient = ( ApeServerClient * ) userData;

	ssize_t r = Net_Receive( serverClient->netSocket,
	                         serverClient->receiveBuffer + serverClient->receivedBytes,
	                         sizeof( serverClient->receiveBuffer ) - serverClient->receivedBytes );
	if ( r == -1 )
	{
		apeDropServerClient( serverClient );
		return;
	}
	else if ( r > 0 )
	{
		serverClient->lastMessageTick = apeGetNumTicks();
	}

	serverClient->receivedBytes += r;

	if ( serverClient->receivedBytes >= sizeof( ApeProtocolMessageHeader ) )
	{
		const ApeProtocolMessageHeader *messageHeader = ( ApeProtocolMessageHeader * ) serverClient->receiveBuffer;

		uint32_t l = messageHeader->length;
		if ( serverClient->receivedBytes >= l )
		{
			/* process message */
			ProcessClientMessage( serverClient, serverClient->receiveBuffer );

			memmove( serverClient->receiveBuffer, serverClient->receiveBuffer + l, serverClient->receivedBytes - l );
			serverClient->receivedBytes -= l;
		}
		else if ( messageHeader->length > PROTOCOL_MESSAGESIZE )
		{
			/* boom */
			PRINT_WARNING( "Client sent a message of an invalid length: %u/%u\n", messageHeader->length, PROTOCOL_MESSAGESIZE );
			apeDropServerClient( serverClient );
		}
	}
}

void apeTickServer( void )
{
	if ( hostSocket != NULL )
	{ /* check if a new connection is being established */
		NetSocket *connectedSocket = Net_Accept( hostSocket );
		if ( connectedSocket != NULL )
		{
			ApeServerClient *serverClient = PlMAllocA( sizeof( ApeServerClient ) );
			serverClient->netSocket       = connectedSocket;
			serverClient->node            = PlInsertLinkedListNode( connectedClients, serverClient );
			/* validation still needs to be performed */
			PRINT( "Client connected, awaiting validation...\n" );
		}

		PlIterateLinkedList( connectedClients, TickServerClient, true );
	}

	apeTickGame();
}

/**
 * Return port of local server instance.
 */
unsigned short apeGetServerPort( void )
{
	assert( hostSocket != NULL );
	if ( hostSocket == NULL )
		return 0;

	return Net_GetLocalPort( hostSocket );
}
