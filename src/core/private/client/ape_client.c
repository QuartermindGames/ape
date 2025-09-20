// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "../ape_private.h"

#include "../net/net.h"
#include "ape_client.h"
#include "ape_client_input.h"
#include "game/game_public.h"
#include "ape_client_gui.h"
#include "editor/editor.h"
#include "renderer/renderer.h"
#include "audio/audio.h"
#include "ape_protocol.h"
#include "renderer/material/material.h"
#include "yin/core_game.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef enum ClientServerState
{
	CLIENT_SERVER_STATE_DISCONNECTED,// has lost connection with the server
	CLIENT_SERVER_STATE_VALIDATING,  // has connected but is pending validation
	CLIENT_SERVER_STATE_REJECTED,    // client has been rejected and will be dropped
	CLIENT_SERVER_STATE_ACCEPTED,    // is connected and validation was successful
} ClientServerState;

typedef struct ClientState
{
	ClientServerState state;

	ApeNetSocket *netSocket;
	bool          isConnected;

	ApeProtocolMessage message;
	unsigned int       lastMessageTick;

	PLConsoleString name;
} ClientState;
static ClientState clientState;

void        ape_prepare_screenshot_capture_( void );
static void capture_screenshot_action( ApeInputState state, const char * )
{
	if ( state != APE_INPUT_STATE_DOWN )
	{
		return;
	}

	ape_prepare_screenshot_capture_();
}

static void process_server_message()
{
	const ApeProtocolMessageHeader *messageHeader = ( ApeProtocolMessageHeader * ) clientState.message.receiveBuffer;
	if ( clientState.state == CLIENT_SERVER_STATE_VALIDATING )
	{
		if ( messageHeader->type != APE_PROTOCOL_MESSAGE_TYPE_VALIDATED )
		{
			ape_warning_( "Invalid message type received: %u\n", messageHeader->type );
			clientState.state = CLIENT_SERVER_STATE_REJECTED;
			return;
		}

		clientState.state = CLIENT_SERVER_STATE_ACCEPTED;
		return;
	}

	if ( clientState.state != CLIENT_SERVER_STATE_ACCEPTED )
	{
		return;
	}

	switch ( messageHeader->type )
	{
		default:
			ape_warning_( "Unhandled client message (%u)!\n", messageHeader->type );
			break;
		case APE_PROTOCOL_MESSAGE_TYPE_GAME:
		{
			const ApeGameInterfaceImport *game = ape_game_get_interface();
			assert( game->clientProcessMessage );
			game->clientProcessMessage( messageHeader + 1, messageHeader->length - sizeof( ApeProtocolMessageHeader ) );
		}
	}
}

static void handle_connection_state( void )
{
	// socket hasn't been created, so...
	if ( clientState.netSocket == NULL )
	{
		return;
	}

	// check if the client is connected to anything
	if ( !clientState.isConnected )
	{
		ApeNetConnectionState state = ape_net_get_connection_status_( clientState.netSocket );
		if ( state != NET_CONNECTION_CONNECTED )
		{
			if ( state == NET_CONNECTION_FAILED )
			{
				ape_client_disconnect_();
				CLIENT_PRINT_WARNING( "Connection failed!\n" );
			}
			return;
		}

		const ApeGameInterfaceImport *game              = ape_game_get_interface();
		ApeProtocolValidationMessage  validationMessage = {
		         .header  = { .length = sizeof( ApeProtocolValidationMessage ), .type = APE_PROTOCOL_MESSAGE_TYPE_VALIDATION },
		         .magic   = APE_PROTOCOL_MAGIC,
		         .version = ( ( uint16_t ) APE_PROTOCOL_VERSION << 8 ) | game->protocolVersion,
        };

		const char *id = game_get_identifier();
		strncpy( validationMessage.identifier, id, sizeof( validationMessage.identifier ) );
		strncpy( validationMessage.clientName, clientState.name, sizeof( validationMessage.clientName ) - 1 );

		ape_net_send_( clientState.netSocket, &validationMessage, sizeof( ApeProtocolValidationMessage ) );

		clientState.isConnected = true;
		clientState.state       = CLIENT_SERVER_STATE_VALIDATING;
		CLIENT_PRINT( "Connected successfully!\n" );
		return;
	}

	ssize_t r = ape_net_receive_( clientState.netSocket, &clientState.message.receiveBuffer + clientState.message.receivedBytes,
	                              sizeof( clientState.message.receiveBuffer ) - clientState.message.receivedBytes );
	if ( r == -1 )
	{
		ape_client_disconnect_();
		return;
	}
	if ( r > 0 )
	{
		clientState.lastMessageTick = ape_get_num_ticks();
	}

	clientState.message.receivedBytes += r;
	if ( clientState.message.receivedBytes >= sizeof( ApeProtocolMessageHeader ) )
	{
		const ApeProtocolMessageHeader *messageHeader = ( ApeProtocolMessageHeader * ) clientState.message.receiveBuffer;

		uint32_t l = messageHeader->length;
		if ( clientState.message.receivedBytes >= l )
		{
			// process message
			process_server_message();

			memmove( clientState.message.receiveBuffer, clientState.message.receiveBuffer + l, clientState.message.receivedBytes - l );
			clientState.message.receivedBytes -= l;
		}
		else if ( messageHeader->length > APE_PROTOCOL_MESSAGE_SIZE )
		{
			// boom
			ape_warning_( "Client sent a message of an invalid length: %u/%u\n", messageHeader->length, APE_PROTOCOL_MESSAGE_SIZE );
			ape_client_disconnect_();
		}

		if ( clientState.state == CLIENT_SERVER_STATE_REJECTED )
		{
			ape_warning_( "Client rejected by server, disconnecting\n" );
			ape_client_disconnect_();
		}
	}
}

static void connect_command( PL_UNUSED unsigned int argc, char **argv )
{
	uint16_t port = strtoul( argv[ 1 ], nullptr, 10 );
	ape_initiate_client_connection_( "localhost", port );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_client_( void )
{
	CLIENT_PRINT( "Initializing client\n" );

	PL_ZERO_( clientState );
	snprintf( clientState.name, sizeof( clientState.name ), "anonymous" );

	ape_renderer_initialize_();
	ape_audio_initialize_();
	ape_initialize_gui_();
	ape_input_initialize_();

	PlRegisterConsoleCommand( "connect",
	                          "Attempt to connect to the specified server.",
	                          1, connect_command );

	ape_client_input_register_action( "capture", nullptr, 0, &( ApeInputKey ) { KEY_F12 }, 1, capture_screenshot_action );
}

void ape_shutdown_client_( void )
{
	ape_shutdown_input_();
	ape_shutdown_gui_();
	ape_audio_shutdown_();
	ape_shutdown_renderer_();
}

void ape_render_frame_( ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	ape_draw_begin_( viewport );

	// Let the game draw from its own camera
	if ( !ape_is_editor_active_() )
	{
		ape_game_get_interface()->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_DRAW, viewport );
	}
	else
	{
		ape_camera_draw_perspective( viewport->camera, viewport );
	}

	ape_draw_menu_( viewport );
	ape_draw_end_( viewport );

	COM_PROFILE_FUNCTION_END();
}

void ape_tick_client_( double delta )
{
	COM_PROFILE_FUNCTION_START();

	ape_begin_input_frame_();

	ape_input_tick_();
	ape_tick_gui_( delta );
	ape_tick_materials_();
	ape_audio_tick_();

	if ( ape_gameInterface->clientTick != nullptr )
	{
		ape_gameInterface->clientTick( delta );
	}

	handle_connection_state();

	ape_end_input_frame_();

	COM_PROFILE_FUNCTION_END();
}

/**
 * Begin connection process - client will continue connecting per
 * tick until success or failure, and then begin handshake process.
 */
void ape_initiate_client_connection_( const char *ip, unsigned short port )
{
	clientState.netSocket = ape_net_open_socket_( ip, port, false );
	if ( clientState.netSocket == NULL )
	{
		CLIENT_PRINT_WARNING( "Failed to open client socket!\n" );
		return;
	}

	CLIENT_PRINT( "Initiated connection to %s, pending...\n", ip );
}

void ape_client_disconnect_( void )
{
	if ( clientState.netSocket != NULL )
	{
		/* todo: let the server know first? */
		ape_net_close_socket_( clientState.netSocket );
		clientState.netSocket = nullptr;
	}

	clientState.state = CLIENT_SERVER_STATE_DISCONNECTED;
}

/**
 * Returns true if the current client is connected and validated.
 */
bool ape_is_client_connected( void )
{
	return ( clientState.state == CLIENT_SERVER_STATE_ACCEPTED );
}

bool ape_client_send( const void **buf, size_t *bufSizes, unsigned int numBuffers )
{
	if ( !ape_is_client_connected() )
	{
		ape_warning_( "Attempted to send message while disconnected!\n" );
		return false;
	}

	size_t totalSize = 0;
	for ( unsigned int i = 0; i < numBuffers; ++i )
	{
		totalSize += bufSizes[ i ];
	}

	ApeProtocolMessageHeader header = { .length = sizeof( ApeProtocolMessageHeader ) + totalSize, .type = APE_PROTOCOL_MESSAGE_TYPE_GAME };
	if ( !ape_net_send_( clientState.netSocket, &header, sizeof( ApeProtocolMessageHeader ) ) )
	{
		ape_warning_( "Failed to send message header!\n" );
		return false;
	}

	for ( unsigned int i = 0; i < numBuffers; ++i )
	{
		if ( ape_net_send_( clientState.netSocket, buf[ i ], bufSizes[ i ] ) )
		{
			continue;
		}

		ape_warning_( "Failed to send message buffer (%u)!\n", i );
		return false;
	}

	return true;
}

void ape_client_register_console_variables_()
{
	PlRegisterConsoleVariable( "client.name", "Identifying name for the client.", "anonymous", PL_VAR_STRING, &clientState.name, nullptr, true );
}
