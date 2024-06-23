
// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

typedef struct ApeNetSocket ApeNetSocket;

typedef enum ApeNetConnectionState
{
	NET_CONNECTION_CONNECTED,
	NET_CONNECTION_PENDING,
	NET_CONNECTION_FAILED,
} ApeNetConnectionState;

#if defined( _MSC_VER )
#	include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

void ape_initialize_net_( void );
void ape_shutdown_net_( void );

ApeNetSocket *ape_net_open_socket_( const char *ip, unsigned short port, bool isHost );
void ape_net_close_socket_( ApeNetSocket *netSocket );

ssize_t ape_net_send_( ApeNetSocket *netSocket, const void *buf, size_t length );
ssize_t ape_net_receive_( ApeNetSocket *netSocket, void *dst, size_t length );
ApeNetSocket *ape_net_accept_( ApeNetSocket *netSocket );

ApeNetConnectionState ape_net_get_connection_status_( ApeNetSocket *netSocket );

unsigned short ape_net_get_local_port_( ApeNetSocket *netSocket );
unsigned short ape_net_get_remote_port_( ApeNetSocket *netSocket );

PL_EXTERN_C_END
