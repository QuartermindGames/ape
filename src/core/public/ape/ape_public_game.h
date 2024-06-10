// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

#define APE_GAME_INTERFACE_VERSION 1

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
	unsigned int version;
	bool ( *requestCallbackMethod )( ApeGameInterfaceRequest gameModeRequest, void *user );
} ApeGameInterfaceImport;
const ApeGameInterfaceImport *ape_game_get_interface( void );

PL_EXTERN_C_END
