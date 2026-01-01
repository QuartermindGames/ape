// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

PL_EXTERN_C

typedef struct ApeNetSocket ApeNetSocket;

typedef enum ApeNetConnectionState
{
	NET_CONNECTION_CONNECTED,
	NET_CONNECTION_PENDING,
	NET_CONNECTION_FAILED,
} ApeNetConnectionState;

static constexpr uint16_t APE_NET_DEFAULT_PORT = 38749;

#if defined( _MSC_VER )
#	include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

void ape_initialize_net_( void );
void ape_shutdown_net_( void );

ApeNetSocket *ape_net_open_socket_( const char *ip, unsigned short port, bool isHost );
void ape_net_close_socket_( ApeNetSocket *netSocket );

/**
 * @brief Queues data to be sent on a connected socket.
 *
 * @param netSocket  A connected socket handle.
 * @param buf        Address of data to be sent.
 * @param length     Length of data to send.
 *
 * @return true on success, false on failure.
 *
 * This function queues up data to be sent on a connected socket.
 *
 * If the provided message is larger than the maximum send size set using the
 * ape_net_set_max_send_size_() function or an unexpected socket error occurs,
 * false will be returned, in which case, the connection should be abandoned.
*/
bool ape_net_send_( ApeNetSocket *netSocket, const void *buf, size_t length );

ssize_t ape_net_receive_( ApeNetSocket *netSocket, void *dst, size_t length );
ApeNetSocket *ape_net_accept_( ApeNetSocket *netSocket );

ApeNetConnectionState ape_net_get_connection_status_( ApeNetSocket *netSocket );

unsigned short ape_net_get_local_port_( ApeNetSocket *netSocket );
unsigned short ape_net_get_remote_port_( ApeNetSocket *netSocket );

/**
 * @brief Sets the maximum size message which may be sent.
 *
 * @param netSocket    A connected socket handle.
 * @param maxSendSize  The maximum message size, in bytes.
 *
 * @return true on success, false on failure.
 *
 * APE queues up data to be sent in buffers managed by the engine and will only
 * send all (or none) of a message so downstream users of the library don't have
 * to handle buffering and partial sends themselves.
 *
 * This function sets the maximum amount of data which may be passed into a
 * single ape_net_send_() call - attempting to write larger buffers will always
 * fail.
*/
bool ape_net_set_max_send_size_( ApeNetSocket *netSocket, size_t maxSendSize );

/**
 * @brief Get the maximum message size set by ape_net_set_max_send_size_().
 *
 * @param netSocket  A connected socket handle.
 *
 * @return The maximum send size, in bytes.
*/
size_t ape_net_get_max_send_size_( const ApeNetSocket *netSocket );

/**
 * @brief Sets the send buffer size.
 *
 * @param netSocket       A connected socket handle.
 * @param sendBufferSize  Send buffer size in bytes.
 *
 * @return true on success, false on failure.
 *
 * This function sets the size of the outgoing data buffer on a connected
 * socket, it should be large enough to accomodate all "in flight" data on the
 * socket at any given time (e.g. data which has been sent and not yet
 * acknowledged by the TCP stack on the receiving end).
*/
bool ape_net_set_send_buffer_size_( ApeNetSocket *netSocket, size_t sendBufferSize );

/**
 * @brief Get the send buffer size set by ape_net_set_send_buffer_size().
 *
 * @param netSocket  A connected socket handle.
 *
 * @return The send buffer size, in bytes.
*/
size_t ape_net_get_send_buffer_size_( ApeNetSocket *netSocket );

PL_EXTERN_C_END
