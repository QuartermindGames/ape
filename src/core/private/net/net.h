/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

PL_EXTERN_C

typedef struct NetSocket NetSocket;

typedef enum NetConnectionState {
	NET_CONNECTION_CONNECTED,
	NET_CONNECTION_PENDING,
	NET_CONNECTION_FAILED,
} NetConnectionState;

#if defined( _MSC_VER )
#	include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

void apeInitializeNet( void );
void ogeShutdownNet( void );
NetSocket *Net_OpenSocket( const char *ip, unsigned short port, bool isHost );
void Net_CloseSocket( NetSocket *netSocket );
ssize_t Net_Send( NetSocket *netSocket, const void *buf, ssize_t length );
ssize_t Net_Receive( NetSocket *netSocket, void *dst, ssize_t length );
NetSocket *Net_Accept( NetSocket *netSocket );
NetConnectionState Net_GetConnectionStatus( NetSocket *netSocket );
unsigned short Net_GetLocalPort( NetSocket *netSocket );
unsigned short Net_GetRemotePort( NetSocket *netSocket );

PL_EXTERN_C_END
