/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"
#include "net.h"

#if defined( _WIN32 )
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

/* setup a server */
void Net_SetupServer( void )
{
}

/* connect to the server */
void Net_ConnectServer( void )
{
}
