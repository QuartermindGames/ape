// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "../ape_private.h"

#include "../net/net.h"
#include "ape_client.h"
#include "ape_client_input.h"
#include "game/game_interface.h"
#include "ape_client_gui.h"
#include "editor/editor.h"
#include "renderer/renderer.h"
#include "audio/audio.h"
#include "world/world.h"

typedef struct ClientState
{
	ApeNetSocket *netSocket;
	bool isConnected;

	char userName[ 32 ];
} ClientState;
static ClientState clientState;

void ape_prepare_screenshot_capture_( void );
static void capture_screenshot_action( ApeInputState state, const char * )
{
	if ( state != APE_INPUT_STATE_DOWN )
	{
		return;
	}

	ape_prepare_screenshot_capture_();
}

void ape_initialize_client_( void )
{
	CLIENT_PRINT( "Initializing client\n" );

	PL_ZERO_( clientState );
	strcpy( clientState.userName, "Anon" );

	ape_initialize_renderer_();
	ape_initialize_audio_();
	ape_initialize_gui_();
	ape_initialize_input_();

	ape_client_input_register_action( "capture", NULL, 0, &( ApeInputKey ){ KEY_F12 }, 1, capture_screenshot_action );
}

void ape_shutdown_client_( void )
{
	ape_shutdown_input_();
	ape_shutdown_gui_();
	ape_shutdown_audio_();
	ape_shutdown_renderer_();
}

void ape_render_frame_( ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	ape_draw_begin_( viewport );

	// Let the game draw from its own camera
	if ( !ape_is_editor_active() )
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

static void handle_connection_state( void )
{
	/* check if the client is connected to anything */
	if ( !clientState.isConnected )
	{
		/* socket hasn't been created, so... */
		if ( clientState.netSocket == NULL )
			return;

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

		clientState.isConnected = true;
		CLIENT_PRINT( "Connected successfully!\n" );
	}
}

void ape_tick_client_( void )
{
	COM_PROFILE_FUNCTION_START();

	ape_begin_input_frame_();

	ape_tick_input_();
	ape_tick_gui_();

	ape_clear_flare_queue_();

	ape_tick_materials_();
	ape_tick_audio_();

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
		clientState.netSocket = NULL;
	}
}
