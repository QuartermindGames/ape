/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

PL_EXTERN_C

typedef struct SSAclNetSocket SSAclNetSocket;

typedef enum SSAclNetConnectionState
{
	NET_CONNECTION_CONNECTED,
	NET_CONNECTION_PENDING,
	NET_CONNECTION_FAILED,
} SSAclNetConnectionState;

#if defined( _MSC_VER )
#	include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

void ss_acl_initialize_net_( void );
void ss_acl_shutdown_net_( void );

SSAclNetSocket *ss_acl_net_open_socket_( const char *ip, unsigned short port, bool isHost );
void ss_acl_net_close_socket_( SSAclNetSocket *netSocket );

ssize_t ss_acl_net_send_( SSAclNetSocket *netSocket, const void *buf, ssize_t length );
ssize_t ss_acl_net_receive_( SSAclNetSocket *netSocket, void *dst, ssize_t length );
SSAclNetSocket *ss_acl_net_accept_( SSAclNetSocket *netSocket );

SSAclNetConnectionState ss_acl_net_get_connection_status_( SSAclNetSocket *netSocket );

unsigned short ss_acl_net_get_local_port_( SSAclNetSocket *netSocket );
unsigned short ss_acl_net_get_remote_port_( SSAclNetSocket *netSocket );

PL_EXTERN_C_END
