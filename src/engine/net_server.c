/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"

#if defined( _WIN32 )
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

#define NET_DEFAULT_ADDRESS 127.0.0.1
#define NET_DEFAULT_PORT    8080

static SOCKET serverSocket;

void NS_Initialize( void )
{
#if defined( _WIN32 )
	WSADATA wsaData;
	int result = WSAStartup( MAKEWORD( 2,2 ), &wsaData );
	if ( result != 0 )
		PrintError( "Failed to initialize Winsock: %d\n", result );
#endif

	// Now we need to create the socket
	serverSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( serverSocket == INVALID_SOCKET )
	{
		int errCode =
#if defined( _WIN32 )
		        WSAGetLastError();
#else
		        errno;
#endif
		PrintError( "Failed to create socket: %d\n", errCode );
	}


}

void NS_Shutdown( void )
{
#if defined( _WIN32 )
	WSACleanup();
#endif
}
