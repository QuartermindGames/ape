// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "../ape_private.h"
#include "../net/net.h"

#include "ape_client.h"
#include "ape_client_input.h"

#include "game/game_interface.h"
#include "ape_client_gui.h"

#include "editors/editors.h"
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

void apeInitializeClient_( void )
{
	CLIENT_PRINT( "Initializing client\n" );

	PL_ZERO_( clientState );

	apeInitializeRenderer_();
	apeInitializeAudio_();
	apeInitializeEditor_();
	apeInitializeGUI_();
	apeInitializeInput_();
}

void apeShutdownClient_( void )
{
	apeShutdownGUI_();
	apeShutdownEditor_();
	apeShutdownAudio_();
	apeShutdownRenderer_();
}

void apeDrawClient( ApeViewport *viewport )
{
	apeBeginDraw( viewport );

	apeDrawPerspective_( viewport->camera, viewport );

	APE_PROFILE_START( PROFILE_DRAW_UI );
	apeDrawMenu( viewport );
	APE_PROFILE_END( PROFILE_DRAW_UI );

	apeEndDraw( viewport );
}

static void apeHandleClientConnectionState_( void )
{
	/* check if the client is connected to anything */
	if ( !clientState.isConnected )
	{
		/* socket hasn't been created, so... */
		if ( clientState.netSocket == NULL )
		{
			return;
		}

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
	apeBeginInputFrame_();

	apeTickInput_();
	apeTickEditor_();
	apeTickGUI_();

	apeHandleClientConnectionState_();

	apeEndInputFrame_();

	apeTickAudio_();
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
