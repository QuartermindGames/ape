// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

PL_EXTERN_C

//TODO: urgh...
#include "ape_public_server.h"

typedef struct ApeWorld ApeWorld;
typedef struct ApeRoom  ApeRoom;

#define APE_GAME_INTERFACE_VERSION 1

typedef enum ApeGameInterfaceRequest
{
	APE_GAME_INTERFACE_REQUEST_INITIALIZE,// called on engine initialisation
	APE_GAME_INTERFACE_REQUEST_SHUTDOWN,  // called when shutting down engine

	APE_GAME_INTERFACE_REQUEST_DRAW,
	APE_GAME_INTERFACE_REQUEST_DRAW_UI,

	APE_GAME_INTERFACE_REQUEST_HANDLE_INPUT,
} ApeGameInterfaceRequest;

// Interface imported from game
typedef struct ApeGameInterfaceImport
{
	unsigned int version;        // interface version
	uint8_t      protocolVersion;// protocol version specific to the game itself
	char         identifier[ 8 ];// identifier for the game (clients will only be able to connect if this matches)

	bool ( *requestCallbackMethod )( ApeGameInterfaceRequest gameModeRequest, void *user );

	void ( *spawnWorld )( ApeRoom *room );
	void ( *onDestroyRoom )( ApeRoom *room );

	// client
	void ( *clientConnect )();
	void ( *clientDisconnect )();
	void ( *clientProcessMessage )( const void *buf, size_t bufSize );
	void ( *clientTick )( double delta );

	// server
	bool ( *serverClientValidate )( ApeServerClient *clientHandle );
	void ( *serverClientConnected )( ApeServerClient *clientHandle );
	void ( *serverClientDisconnected )( ApeServerClient *clientHandle );
	void ( *serverProcessMessage )( ApeServerClient *clientHandle, const void *buf, size_t bufSize );
	void ( *serverTick )( double delta );
} ApeGameInterfaceImport;
const ApeGameInterfaceImport *ape_game_get_interface( void );

PL_EXTERN_C_END
