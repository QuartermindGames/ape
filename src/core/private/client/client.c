// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "../core_private.h"
#include "../net/net.h"

#include "client.h"
#include "client_input.h"

#include "game_interface.h"
#include "client_gui.h"

#include "editor/editor.h"
#include "renderer/renderer.h"
#include "audio/audio.h"

typedef struct ClientState
{
	NetSocket *netSocket;
	bool isConnected;

	bool isEditMode;

	char userName[ 32 ];
} ClientState;
static ClientState clientState;

void YnCore_InitializeClient( void )
{
	CLIENT_PRINT( "Initializing client\n" );

	PL_ZERO_( clientState );

	YnCore_InitializeRenderer();
	YnCore_InitializeAudio();
	YnCore_InitializeEditor();

	YnCore_InitializeGUI();

	Client_Input_Initialize();
}

void YnCore_ShutdownClient( void )
{
	YnCore_ShutdownGUI();

	YnCore_ShutdownEditor();
	YnCore_ShutdownAudio();
	YnCore_ShutdownRenderer();
}

void YnCore_DrawClient( YNCoreViewport *viewport )
{
	YnCore_BeginDraw( viewport );

	YnCore_DrawPerspective( viewport->camera, viewport );

	YN_CORE_PROFILE_START( PROFILE_DRAW_UI );
	YnCore_DrawMenu( viewport );
	YN_CORE_PROFILE_END( PROFILE_DRAW_UI );

	YnCore_EndDraw( viewport );
}

static void Client_HandleConnectionState( void )
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
				YnCore_Client_Disconnect();
				CLIENT_PRINT_WARNING( "Connection failed!\n" );
			}
			return;
		}

		clientState.isConnected = true;
		CLIENT_PRINT( "Connected successfully!\n" );
	}
}

void YnCore_TickClient( void )
{
	Client_Input_BeginFrame();

	Client_Input_Tick();

	YnCore_TickEditor();
	YnCore_TickGUI();

	Client_HandleConnectionState();

	Client_Input_EndFrame();

	YnCore_TickAudio();
}

/**
 * Begin connection process - client will continue connecting per
 * tick until success or failure, and then begin handshake process.
 */
void YnCore_Client_InitiateConnection( const char *ip, unsigned short port )
{
	clientState.netSocket = Net_OpenSocket( ip, port, false );
	if ( clientState.netSocket == NULL )
	{
		CLIENT_PRINT_WARNING( "Failed to open client socket!\n" );
		return;
	}

	CLIENT_PRINT( "Initiated connection to %s, pending...\n", ip );
}

void YnCore_Client_Disconnect( void )
{
	if ( clientState.netSocket != NULL )
	{
		/* todo: let the server know first? */
		Net_CloseSocket( clientState.netSocket );
		clientState.netSocket = NULL;
	}
}
