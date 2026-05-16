// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

PL_EXTERN_C

//TODO: urgh...
#include "ape_public_server.h"

typedef struct ApeViewport ApeViewport;
typedef struct ApeWorld    ApeWorld;
typedef struct ApeRoom     ApeRoom;

static constexpr unsigned int APE_GAME_INTERFACE_VERSION = 1;

// Interface imported from game
typedef struct ApeGameInterfaceImport
{
	unsigned int version;        // interface version
	uint8_t      protocolVersion;// protocol version specific to the game itself
	char         identifier[ 8 ];// identifier for the game (clients will only be able to connect if this matches)

	bool ( *initialize )();
	void ( *shutdown )();

	void ( *draw )( const ApeViewport *viewport );
	void ( *drawUI )( const ApeViewport *viewport );

	void ( *spawnWorld )( ApeRoom *room );
	void ( *onDestroyRoom )( ApeRoom *room );

	// client
	void ( *clientConnected )();// called on successful validation
	void ( *clientDisconnect )();
	void ( *clientProcessMessage )( const void *buf, size_t bufSize );
	void ( *clientTick )( double delta );

	// server
	bool ( *serverClientValidate )( ApeServerClient *clientHandle );
	void ( *serverClientConnected )( ApeServerClient *clientHandle );// called on successful validation
	void ( *serverClientDisconnected )( ApeServerClient *clientHandle );
	void ( *serverProcessMessage )( ApeServerClient *clientHandle, const void *buf, size_t bufSize );
	void ( *serverTick )( double delta );
} ApeGameInterfaceImport;
const ApeGameInterfaceImport *ape_game_get_interface( void );

PL_EXTERN_C_END
