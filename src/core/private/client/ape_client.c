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

typedef struct ClientState {
	NetSocket *netSocket;
	bool isConnected;

	bool isEditorMode;

	char userName[ 32 ];
} ClientState;
static ClientState clientState;

void apeInitializeClient_( void ) {
	CLIENT_PRINT( "Initializing client\n" );

	PL_ZERO_( clientState );
	strcpy( clientState.userName, "Anon" );

	ar_initialize_();
	apeInitializeAudio_();
	apeInitializeEditor_();
	apeInitializeGUI_();
	apeInitializeInput_();
}

void apeShutdownClient_( void ) {
	apeShutdownGUI_();
	apeShutdownEditor_();
	apeShutdownAudio_();
	ar_shutdown_();
}

void apeDrawClient( ApeViewport *viewport ) {
	COM_PROFILE_FUNCTION_START();

	ar_draw_begin( viewport );

	arl_camera_draw_perspective_( viewport->camera, viewport );

	arl_draw_menu( viewport );

	ar_draw_end( viewport );

	COM_PROFILE_FUNCTION_END();
}

static void apeHandleClientConnectionState_( void ) {
	/* check if the client is connected to anything */
	if ( !clientState.isConnected ) {
		/* socket hasn't been created, so... */
		if ( clientState.netSocket == NULL ) {
			return;
		}

		NetConnectionState state = Net_GetConnectionStatus( clientState.netSocket );
		if ( state != NET_CONNECTION_CONNECTED ) {
			if ( state == NET_CONNECTION_FAILED ) {
				apeDisconnectClient_();
				CLIENT_PRINT_WARNING( "Connection failed!\n" );
			}
			return;
		}

		clientState.isConnected = true;
		CLIENT_PRINT( "Connected successfully!\n" );
	}
}

void apeTickClient( void ) {
	COM_PROFILE_FUNCTION_START();

	apeBeginInputFrame_();

	apeTickInput_();
	apeTickGUI_();

#if defined( APE_EDITOR_ENABLED )
	edTick();
#endif

	apeTickClientWorld_();

	apeHandleClientConnectionState_();

	apeEndInputFrame_();

	apeTickAudio_();

	COM_PROFILE_FUNCTION_END();
}

/**
 * Begin connection process - client will continue connecting per
 * tick until success or failure, and then begin handshake process.
 */
void apeInitiateClientConnection_( const char *ip, unsigned short port ) {
	clientState.netSocket = Net_OpenSocket( ip, port, false );
	if ( clientState.netSocket == NULL ) {
		CLIENT_PRINT_WARNING( "Failed to open client socket!\n" );
		return;
	}

	CLIENT_PRINT( "Initiated connection to %s, pending...\n", ip );
}

void apeDisconnectClient_( void ) {
	if ( clientState.netSocket != NULL ) {
		/* todo: let the server know first? */
		Net_CloseSocket( clientState.netSocket );
		clientState.netSocket = NULL;
	}
}
