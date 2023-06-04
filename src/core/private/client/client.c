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

void apeInitializeClient( void )
{
	CLIENT_PRINT( "Initializing client\n" );

	PL_ZERO_( clientState );

	apeInitializeRenderer();
	YnCore_InitializeAudio();
	YnCore_InitializeEditor();

	YnCore_InitializeGUI();

	apeInitializeInput_();
}

void ogeShutdownClient( void )
{
	YnCore_ShutdownGUI();

	ogeShutdownEditor();
	YnCore_ShutdownAudio();
	apeShutdownRenderer();
}

void apeDrawClient( ApeViewport *viewport )
{
	apeBeginDraw( viewport );

	apeDrawPerspective_( viewport->camera, viewport );

	OGE_PROFILE_START( PROFILE_DRAW_UI );
	apeDrawMenu( viewport );
	OGE_PROFILE_END( PROFILE_DRAW_UI );

	apeEndDraw( viewport );
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

void apeTickClient( void )
{
	apeBeginInputFrame_();

	apeTickInput_();

	YnCore_TickEditor();
	YnCore_TickGUI();

	Client_HandleConnectionState();

	apeEndInputFrame_();

	YnCore_TickAudio();
}

/**
 * Begin connection process - client will continue connecting per
 * tick until success or failure, and then begin handshake process.
 */
void ogeClient_InitiateConnection( const char *ip, unsigned short port )
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
