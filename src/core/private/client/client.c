// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "qmos/public/qm_os_time.h"
#include "qmos/public/qm_os_string.h"

#include "ape_private.h"

#include "net/net.h"

#include "client.h"
#include "client_input.h"
#include "client_gui.h"

#include "game/game_public.h"
#include "editor/editor.h"

#include "renderer/renderer.h"
#include "renderer/material/material.h"

#include "model/model.h"
#include "audio/audio.h"
#include "ape_protocol.h"
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
	double             lastMessageTime;

	ApeConsoleVarString name;
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
			ape_console_warning_( "Invalid message type received: %u\n", messageHeader->type );
			clientState.state = CLIENT_SERVER_STATE_REJECTED;
			return;
		}

		clientState.state = CLIENT_SERVER_STATE_ACCEPTED;

		const ApeGameInterfaceImport *game = ape_game_get_interface();
		if ( game != nullptr && game->clientConnected != nullptr )
		{
			game->clientConnected();
		}

		return;
	}

	if ( clientState.state != CLIENT_SERVER_STATE_ACCEPTED )
	{
		return;
	}

	switch ( messageHeader->type )
	{
		default:
			ape_console_warning_( "Unhandled client message (%u)!\n", messageHeader->type );
			break;
		case APE_PROTOCOL_MESSAGE_TYPE_HEARTBEAT_RESPONSE:
			//ape_console_print_( "CL: Received heartbeat response\n" );
			break;
		case APE_PROTOCOL_MESSAGE_TYPE_HEARTBEAT_REQUEST:
		{
			//ape_console_print_( "CL: Received heartbeat request\n" );
			static constexpr ApeProtocolMessageHeader header = { .length = sizeof( ApeProtocolMessageHeader ), .type = APE_PROTOCOL_MESSAGE_TYPE_HEARTBEAT_RESPONSE };
			if ( !ape_net_send_( clientState.netSocket, &header, sizeof( ApeProtocolMessageHeader ) ) )
			{
				ape_client_disconnect();
			}
			break;
		}
		case APE_PROTOCOL_MESSAGE_TYPE_GAME:
		{
			const ApeGameInterfaceImport *game = ape_game_get_interface();
			assert( game->clientProcessMessage );
			game->clientProcessMessage( messageHeader + 1, messageHeader->length - sizeof( ApeProtocolMessageHeader ) );
			break;
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
				ape_client_disconnect();
				ape_console_warning_( "Connection failed!\n" );
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
		ape_console_print_( "Connected successfully!\n" );
		return;
	}

	ssize_t r = ape_net_receive_( clientState.netSocket, &clientState.message.receiveBuffer + clientState.message.receivedBytes,
	                              sizeof( clientState.message.receiveBuffer ) - clientState.message.receivedBytes );
	if ( r == -1 )
	{
		ape_client_disconnect();
		return;
	}

	double time = qm_os_time_get_seconds();
	if ( r == 0 && time - clientState.lastMessageTime >= APE_PROTOCOL_TIMEOUT )
	{
		ape_console_warning_( "Received no response from server, disconnecting!\n", clientState.lastMessageTime );
		ape_client_disconnect();
		return;
	}
	if ( r == 0 && time - clientState.lastMessageTime >= APE_PROTOCOL_HEARTBEAT_TIME )
	{
		ApeProtocolMessageHeader header = { .length = sizeof( ApeProtocolMessageHeader ), .type = APE_PROTOCOL_MESSAGE_TYPE_HEARTBEAT_REQUEST };
		if ( !ape_net_send_( clientState.netSocket, &header, sizeof( ApeProtocolMessageHeader ) ) )
		{
			ape_client_disconnect();
		}
		return;
	}

	if ( r > 0 )
	{
		clientState.lastMessageTime = time;
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
			ape_console_warning_( "Client sent a message of an invalid length: %u/%u\n", messageHeader->length, APE_PROTOCOL_MESSAGE_SIZE );
			ape_client_disconnect();
		}

		if ( clientState.state == CLIENT_SERVER_STATE_REJECTED )
		{
			ape_console_warning_( "Client rejected by server, disconnecting\n" );
			ape_client_disconnect();
		}
	}
}

static void connect_command( unsigned int argc, const char *const *argv )
{
	char *address = qm_os_string_alloc( "%s", argv[ 1 ] );
	if ( address == nullptr )
	{
		ape_console_warning_( "Failed to allocate string for connect!\n" );
		return;
	}

	uint16_t port = APE_NET_DEFAULT_PORT;
	if ( *address == '[' )
	{
		// ipv6

		char *p = strrchr( address, ']' );
		if ( p == nullptr )
		{
			ape_console_warning_( "Not a valid ipv6 address (%s)!\n", argv[ 1 ] );
			goto cleanup;
		}

		unsigned int length = p - 1 - address;

		// check if the port is provided at the end
		p = strrchr( p, ':' );
		if ( p != nullptr )
		{
			port = strtoul( p + 1, nullptr, 10 );
			*p   = '\0';
		}

		strncpy( address, address + 1, length );
		address[ length ] = '\0';
	}
	else
	{
		// ipv4

		// check if the port is provided at the end
		char *p = strrchr( address, ':' );
		if ( p != nullptr )
		{
			port = strtoul( p + 1, nullptr, 10 );
			*p   = '\0';
		}
	}

	ape_client_connect( address, port );

cleanup:
	qm_os_memory_free( address );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_client_( void )
{
	ape_console_print_( "Initializing client\n" );

	QM_OS_ZERO_( clientState );
	snprintf( clientState.name, sizeof( clientState.name ), "anonymous" );

	ape_renderer_initialize_();
	ape_audio_initialize_();
	ape_initialize_gui_();
	ape_input_initialize_();

	ape_console_cmd_register( "connect",
	                          "Attempt to connect to the specified server.",
	                          1, connect_command );

	ape_client_input_register_action( "capture", nullptr, 0, &( ApeInputKey ) { KEY_F12 }, 1, capture_screenshot_action, 0 );
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

	ape_renderer_begin( viewport );

	if ( ape_editor_is_active() && viewport->camera != nullptr )
	{
		ape_camera_draw_perspective( viewport->camera, viewport );
	}
	else if ( ape_gameInterface->draw != nullptr )
	{
		ape_gameInterface->draw( viewport );
	}

	ape_renderer_draw_menu( viewport );
	ape_renderer_end( viewport );

	COM_PROFILE_FUNCTION_END();
}

void ape_tick_client_( const double delta )
{
	COM_PROFILE_FUNCTION_START();

	ape_begin_input_frame_();

	ape_input_tick_();
	ape_tick_gui_( delta );
	ape_tick_materials_( delta );
	ape_audio_tick_();

	//TODO: move this somewhere better
	ape_model_compute_models_lighting( delta );

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
void ape_client_connect( const char *address, const uint16_t port )
{
	clientState.netSocket = ape_net_open_socket_( address, port, false );
	if ( clientState.netSocket == NULL )
	{
		ape_console_warning_( "Failed to open client socket!\n" );
		return;
	}

	ape_console_print_( "Initiated connection to %s, pending...\n", address );
}

void ape_client_disconnect( void )
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
		ape_console_warning_( "Attempted to send message while disconnected!\n" );
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
		ape_console_warning_( "Failed to send message header!\n" );
		return false;
	}

	for ( unsigned int i = 0; i < numBuffers; ++i )
	{
		if ( ape_net_send_( clientState.netSocket, buf[ i ], bufSizes[ i ] ) )
		{
			continue;
		}

		ape_console_warning_( "Failed to send message buffer (%u)!\n", i );
		return false;
	}

	return true;
}

void ape_client_register_console_variables_()
{
	ape_console_var_register( "client.name", "Identifying name for the client.", "anonymous", PL_VAR_STRING, &clientState.name, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
}
