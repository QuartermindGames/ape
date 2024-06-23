// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

#define APE_GAME_INTERFACE_VERSION 1

typedef struct ApeServerClient ApeServerClientHandle;

typedef enum ApeGameInterfaceRequest
{
	APE_GAME_INTERFACE_REQUEST_INITIALIZE,// called on engine initialisation
	APE_GAME_INTERFACE_REQUEST_SHUTDOWN,  // called when shutting down engine

	APE_GAME_INTERFACE_REQUEST_DRAW,
	APE_GAME_INTERFACE_REQUEST_DRAW_UI,

	APE_GAME_INTERFACE_REQUEST_TICK_SERVER,// called after entity tick
	APE_GAME_INTERFACE_REQUEST_HANDLE_INPUT,

	APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD,// called before entities are spawned in and
	                                       // before starting and connecting to server

	APE_GAME_INTERFACE_REQUEST_CONNECT,
	APE_GAME_INTERFACE_REQUEST_DISCONNECT,
} ApeGameInterfaceRequest;

// Interface imported from game
typedef struct ApeGameInterfaceImport
{
	unsigned int version;   // interface version
	uint8_t protocolVersion;// protocol version specific to the game itself
	char identifier[ 8 ];   // identifier for the game (clients will only be able to connect if this matches)

	bool ( *requestCallbackMethod )( ApeGameInterfaceRequest gameModeRequest, void *user );

	// client
	void ( *clientConnect )();
	void ( *clientDisconnect )();
	void ( *clientProcessMessage )( const void *buf, size_t bufSize );

	// server
	void ( *serverClientConnected )( ApeServerClientHandle *clientHandle );
	void ( *serverClientDisconnected )( ApeServerClientHandle *clientHandle );
	void ( *serverProcessMessage )( ApeServerClientHandle *clientHandle, const void *buf, size_t bufSize );
} ApeGameInterfaceImport;
const ApeGameInterfaceImport *ape_game_get_interface( void );

PL_EXTERN_C_END
