/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "net.h"

#ifndef _WIN32
#   include <sys/socket.h>
#else
#   include <winsock.h>
#endif

#define NET_DEFAULT_ADDRESS 127.0.0.1
#define NET_DEFAULT_PORT    8080

int serverSocket = 0;

/* setup a server */
void Net_SetupServer( void ) {
	serverSocket = socket( AF_INET, SOCK_STREAM, 0 );
	if ( serverSocket == 0 ) {
		PrintWarn( "Failed to create socket!\n" );
		return;
	}
}

/* connect to the server */
void Net_ConnectServer( void ) {

}
