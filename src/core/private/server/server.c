// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Server implementation.

#include "ape_private.h"

#include "ape/ape_public_game.h"

#include "server.h"

#define SERVER_CLIENT_TIMEOUT 1024

static ApeNetSocket *hostSocket;

static char serverName[ PL_VAR_VALUE_LENGTH ];
static char serverPass[ PL_VAR_VALUE_LENGTH ];

typedef struct ApeServerClient
{
	char name[ APE_PROTOCOL_MAX_CLIENT_NAME ];

	ApeNetSocket     *netSocket;
	PLLinkedListNode *node;

	ApeServerClientState state;

	ApeProtocolMessage message;

	unsigned int lastMessageTick;
} ApeServerClient;
static PLLinkedList *connectedClients;

static void drop_client_callback( void *userData, PL_UNUSED bool *breakEarly )
{
	ape_server_drop_client_( userData );
}

void ape_server_register_console_variables_()
{
	PlRegisterConsoleVariable( "server.name", "Name to use for the server.", "unnamed", PL_VAR_STRING, serverName, nullptr, true );
	PlRegisterConsoleVariable( "server.pass", "Password to access server functions.", "", PL_VAR_STRING, serverPass, nullptr, true );
}

bool ape_server_start( const char *ip, unsigned short port )
{
	hostSocket = ape_net_open_socket_( ip, port, true );
	if ( hostSocket == NULL )
	{
		ape_warning_( "Failed to open server socket!\n" );
		return false;
	}

	ape_print_( "APE server active (%s:%u), listening for clients...\n", ip, ape_server_get_port_() );

	return true;
}

void ape_initialize_server_( void )
{
	connectedClients = PlCreateLinkedList();
	if ( connectedClients == NULL )
	{
		ape_error_( true, "Failed to create connected clients list: %s\n", PlGetError() );
	}
}

void ape_shutdown_server_( void )
{
	// drop all connected clients
	PlIterateLinkedList( connectedClients, drop_client_callback, true );
}

void ape_server_drop_client_( ApeServerClient *serverClient )
{
	ape_print_( "Dropping client...\n" );

	ape_net_close_socket_( serverClient->netSocket );

	const ApeGameInterfaceImport *game = ape_game_get_interface();
	assert( game->serverClientDisconnected != nullptr );
	game->serverClientDisconnected( serverClient );

	PlDestroyLinkedListNode( serverClient->node );
	qm_os_memory_free( serverClient );
}

static void validate_client( const ApeProtocolValidationMessage *message, ApeServerClient *client )
{
	//TODO: reject if its over our limit; this should go to the game-side to check, as that's where we're tracking limits

	if ( message->magic != APE_PROTOCOL_MAGIC )
	{
		ape_warning_( "Invalid magic received from client (%u != %u)!\n", message->magic, APE_PROTOCOL_MAGIC );
		client->state = APE_SERVER_CLIENT_STATE_REJECTED;
		return;
	}

	const ApeGameInterfaceImport *game = ape_game_get_interface();

	uint16_t baseProtocolVersion = message->version >> 8;
	if ( baseProtocolVersion != APE_PROTOCOL_VERSION )
	{
		ape_warning_( "Invalid protocol version received from client (%u != %u)!\n", baseProtocolVersion, APE_PROTOCOL_VERSION );
		client->state = APE_SERVER_CLIENT_STATE_REJECTED;
		return;
	}

	uint16_t gameProtocolVersion = ( message->version & 0xFF );
	if ( gameProtocolVersion != game->protocolVersion )
	{
		ape_warning_( "Invalid game protocol version received from client (%u != %u)!\n", gameProtocolVersion, game->protocolVersion );
		client->state = APE_SERVER_CLIENT_STATE_REJECTED;
		return;
	}

	if ( strncmp( message->identifier, game->identifier, sizeof( message->identifier ) ) != 0 )
	{
		ape_warning_( "Invalid identifier received from client (%s != %s)!\n", message->identifier, game->identifier );
		client->state = APE_SERVER_CLIENT_STATE_REJECTED;
		return;
	}

	if ( *message->clientName == '\0' )
	{
		ape_warning_( "No name specified for client!\n" );
		client->state = APE_SERVER_CLIENT_STATE_REJECTED;
		return;
	}

	if ( game->serverClientValidate && !game->serverClientValidate( client ) )
	{
		// no warning here as it's assumed the game will spit something out...
		client->state = APE_SERVER_CLIENT_STATE_REJECTED;
		return;
	}

	snprintf( client->name, sizeof( client->name ), "%s", message->clientName );
	ape_print_( "Client (%s) validated successfully\n", client->name );

	client->state = APE_SERVER_CLIENT_STATE_ACCEPTED;

	assert( game->serverClientConnected != nullptr );
	game->serverClientConnected( client );

	ape_net_send_( client->netSocket, &( ApeProtocolMessageHeader ) {
	                                          .length = sizeof( ApeProtocolMessageHeader ),
	                                          .type   = APE_PROTOCOL_MESSAGE_TYPE_VALIDATED,
	                                  },
	               sizeof( ApeProtocolMessageHeader ) );
}

static void process_client_message( ApeServerClient *client, const void *buf )
{
	const ApeProtocolMessageHeader *messageHeader = ( ApeProtocolMessageHeader * ) buf;
	if ( client->state == APE_SERVER_CLIENT_STATE_VALIDATING )
	{
		if ( messageHeader->type != APE_PROTOCOL_MESSAGE_TYPE_VALIDATION )
		{
			ape_warning_( "Client answered without validation message, rejecting!\n" );
			client->state = APE_SERVER_CLIENT_STATE_REJECTED;
			return;
		}

		const ApeProtocolValidationMessage *message;
		if ( ( message = APE_PROTOCOL_VALIDATE_MESSAGE( &client->message, ApeProtocolValidationMessage ) ) == nullptr )
		{
			client->state = APE_SERVER_CLIENT_STATE_REJECTED;
			return;
		}

		validate_client( message, client );
		return;
	}

	if ( client->state != APE_SERVER_CLIENT_STATE_ACCEPTED )
	{
		return;
	}

	switch ( messageHeader->type )
	{
		default:
			ape_warning_( "Unhandled server message (%u)!\n", messageHeader->type );
			break;
		case APE_PROTOCOL_MESSAGE_TYPE_GAME:
		{
			const ApeGameInterfaceImport *game = ape_game_get_interface();
			assert( game->serverProcessMessage );
			game->serverProcessMessage( client, messageHeader + 1, messageHeader->length - sizeof( ApeProtocolMessageHeader ) );
		}
	}
}

static void tick_server_client( void *userData, bool *breakEarly )
{
	ApeServerClient *serverClient = ( ApeServerClient * ) userData;

	ssize_t r = ape_net_receive_( serverClient->netSocket,
	                              serverClient->message.receiveBuffer + serverClient->message.receivedBytes,
	                              sizeof( serverClient->message.receiveBuffer ) - serverClient->message.receivedBytes );
	if ( r == -1 )
	{
		ape_server_drop_client_( serverClient );
		return;
	}
	else if ( r > 0 )
	{
		serverClient->lastMessageTick = ape_get_num_ticks();
	}

	serverClient->message.receivedBytes += r;
	if ( serverClient->message.receivedBytes >= sizeof( ApeProtocolMessageHeader ) )
	{
		const ApeProtocolMessageHeader *messageHeader = ( ApeProtocolMessageHeader * ) serverClient->message.receiveBuffer;

		uint32_t l = messageHeader->length;
		if ( serverClient->message.receivedBytes >= l )
		{
			// process message
			process_client_message( serverClient, serverClient->message.receiveBuffer );

			memmove( serverClient->message.receiveBuffer, serverClient->message.receiveBuffer + l, serverClient->message.receivedBytes - l );
			serverClient->message.receivedBytes -= l;
		}
		else if ( messageHeader->length > APE_PROTOCOL_MESSAGE_SIZE )
		{
			// boom
			ape_warning_( "Client sent a message of an invalid length: %u/%u\n", messageHeader->length, APE_PROTOCOL_MESSAGE_SIZE );
			ape_server_drop_client_( serverClient );
		}

		if ( serverClient->state == APE_SERVER_CLIENT_STATE_REJECTED )
		{
			ape_server_drop_client_( serverClient );
		}
	}
}

void ape_tick_server_( double delta )
{
	if ( hostSocket == nullptr )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	// check if a new connection is being established
	ApeNetSocket *connectedSocket = ape_net_accept_( hostSocket );
	if ( connectedSocket != NULL )
	{
		ApeServerClient *serverClient = QM_OS_MEMORY_MALLOC_( sizeof( ApeServerClient ) );
		serverClient->netSocket       = connectedSocket;
		serverClient->node            = PlInsertLinkedListNode( connectedClients, serverClient );
		serverClient->state           = APE_SERVER_CLIENT_STATE_VALIDATING;
		// validation still needs to be performed
		ape_print_( "Client connected, awaiting validation...\n" );
	}

	PlIterateLinkedList( connectedClients, tick_server_client, true );

	ape_tick_game_server_( delta );

	COM_PROFILE_FUNCTION_END();
}

/**
 * Return port of local server instance.
 */
unsigned short ape_server_get_port_( void )
{
	if ( hostSocket == NULL )
	{
		return 0;
	}

	return ape_net_get_local_port_( hostSocket );
}

bool ape_server_send( ApeServerClient *clientHandle, const void **buf, size_t *bufSizes, unsigned int numBuffers )
{
	size_t totalSize = 0;
	for ( unsigned int i = 0; i < numBuffers; ++i )
	{
		totalSize += bufSizes[ i ];
	}

	ApeProtocolMessageHeader header = { .length = sizeof( ApeProtocolMessageHeader ) + totalSize, .type = APE_PROTOCOL_MESSAGE_TYPE_GAME };
	if ( !ape_net_send_( clientHandle->netSocket, &header, sizeof( ApeProtocolMessageHeader ) ) )
	{
		ape_warning_( "Failed to send message header!\n" );
		return false;
	}

	for ( unsigned int i = 0; i < numBuffers; ++i )
	{
		if ( ape_net_send_( clientHandle->netSocket, buf[ i ], bufSizes[ i ] ) )
		{
			continue;
		}

		ape_warning_( "Failed to send message buffer (%u)!\n", i );
		return false;
	}

	return true;
}

const char *ape_server_get_client_name( ApeServerClient *client )
{
	return client->name;
}
