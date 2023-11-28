// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

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
	NetSocket *netSocket;
	bool isConnected;

	bool isEditorMode;

	char userName[ 32 ];
} ClientState;
static ClientState clientState;

void apeInitializeClient_( void )
{
	CLIENT_PRINT( "Initializing client\n" );

	PL_ZERO_( clientState );
	strcpy( clientState.userName, "Anon" );

	ss_arl_initialize_();

	apeInitializeAudio_();
	apeInitializeGUI_();
	apeInitializeInput_();
}

void apeShutdownClient_( void )
{
	apeShutdownGUI_();
	apeShutdownAudio_();
	ss_arl_shutdown_();
}

void ss_arl_render_frame( SS_Arl_Viewport *viewport )
{
	// If we're capturing, ignore the request from the
	// caller to render the frame because we'll lock it
	// with the frame tick instead...
	if ( ss_arl_get_capture_state_() )
		return;

	COM_PROFILE_FUNCTION_START();

	ss_arl_draw_begin_( viewport );

	ss_arl_camera_draw_perspective_( viewport->camera, viewport );
	ss_arl_draw_menu_( viewport );

	ss_arl_draw_end_( viewport );

	ss_shell_swap_window( viewport );

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

		NetConnectionState state = Net_GetConnectionStatus( clientState.netSocket );
		if ( state != NET_CONNECTION_CONNECTED )
		{
			if ( state == NET_CONNECTION_FAILED )
			{
				apeDisconnectClient_();
				CLIENT_PRINT_WARNING( "Connection failed!\n" );
			}
			return;
		}

		clientState.isConnected = true;
		CLIENT_PRINT( "Connected successfully!\n" );
	}
}

void apeTickClient( void )
{
	COM_PROFILE_FUNCTION_START();

	apeBeginInputFrame_();

	apeTickInput_();
	apeTickGUI_();

	ss_acl_level_client_tick_();

	ss_arl_tick_materials_();

	handle_connection_state();

	apeEndInputFrame_();

	apeTickAudio_();

	COM_PROFILE_FUNCTION_END();
}

/**
 * Begin connection process - client will continue connecting per
 * tick until success or failure, and then begin handshake process.
 */
void apeInitiateClientConnection_( const char *ip, unsigned short port )
{
	clientState.netSocket = Net_OpenSocket( ip, port, false );
	if ( clientState.netSocket == NULL )
	{
		CLIENT_PRINT_WARNING( "Failed to open client socket!\n" );
		return;
	}

	CLIENT_PRINT( "Initiated connection to %s, pending...\n", ip );
}

void apeDisconnectClient_( void )
{
	if ( clientState.netSocket != NULL )
	{
		/* todo: let the server know first? */
		Net_CloseSocket( clientState.netSocket );
		clientState.netSocket = NULL;
	}
}
