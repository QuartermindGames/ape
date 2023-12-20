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
	SSAclNetSocket *netSocket;
	bool isConnected;

	bool isEditorMode;

	char userName[ 32 ];
} ClientState;
static ClientState clientState;

void ss_acl_initialize_client_( void )
{
	CLIENT_PRINT( "Initializing client\n" );

	PL_ZERO_( clientState );
	strcpy( clientState.userName, "Anon" );

	ss_arl_initialize_();

	apeInitializeAudio_();
	ss_acl_initialize_gui_();
	apeInitializeInput_();
}

void ss_acl_shutdown_client_( void )
{
	ss_acl_shutdown_gui_();
	apeShutdownAudio_();
	ss_arl_shutdown_();
}

void ss_arl_render_frame_( SSArlViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	ss_arl_draw_begin_( viewport );

	if ( ss_acl_is_editor_active() || !ss_game_mode_get_interface()->requestCallbackMethod( GAME_MODE_REQUEST_DRAW, viewport ) )
		ss_arl_camera_draw_perspective( viewport->camera, viewport );

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

		SSAclNetConnectionState state = ss_acl_net_get_connection_status_( clientState.netSocket );
		if ( state != NET_CONNECTION_CONNECTED )
		{
			if ( state == NET_CONNECTION_FAILED )
			{
				ss_acl_client_disconnect_();
				CLIENT_PRINT_WARNING( "Connection failed!\n" );
			}
			return;
		}

		clientState.isConnected = true;
		CLIENT_PRINT( "Connected successfully!\n" );
	}
}

void ss_acl_tick_client_( void )
{
	COM_PROFILE_FUNCTION_START();

	apeBeginInputFrame_();

	ss_acl_tick_input_();
	ss_acl_tick_gui_();

	ss_acl_level_client_tick_();

	ss_arl_tick_materials_();

	handle_connection_state();

	apeEndInputFrame_();

	ss_acl_audio_tick_();

	COM_PROFILE_FUNCTION_END();
}

/**
 * Begin connection process - client will continue connecting per
 * tick until success or failure, and then begin handshake process.
 */
void ss_acl_initiate_client_connection_( const char *ip, unsigned short port )
{
	clientState.netSocket = ss_acl_net_open_socket_( ip, port, false );
	if ( clientState.netSocket == NULL )
	{
		CLIENT_PRINT_WARNING( "Failed to open client socket!\n" );
		return;
	}

	CLIENT_PRINT( "Initiated connection to %s, pending...\n", ip );
}

void ss_acl_client_disconnect_( void )
{
	if ( clientState.netSocket != NULL )
	{
		/* todo: let the server know first? */
		ss_acl_net_close_socket_( clientState.netSocket );
		clientState.netSocket = NULL;
	}
}
