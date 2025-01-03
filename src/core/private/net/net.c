// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "net.h"

#include <limits.h>

#if defined( _WIN32 )
#	include <winsock2.h>
#	include <ws2tcpip.h>
/* included after, due to far/near macros */
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <netdb.h>
#	include <errno.h>
#	include <fcntl.h>
#endif

enum
{
	NET_IP4,
	NET_IP6,
};

#if defined( _MSC_VER )
typedef SOCKET ApeNetSocketHandle;
#else
typedef int ApeNetSocketHandle;
#endif

static void close_socket( ApeNetSocketHandle handle )
{
#if defined( _MSC_VER )
	closesocket( handle );
#else
	close( handle );
#endif
}

typedef struct ApeNetSocket
{
	ApeNetSocketHandle handle; /* system socket handle */
	int addressType;           /* ip4 / ip6 */
	union
	{
		struct sockaddr_in ip4;
		struct sockaddr_in6 ip6;
	} local;
	union
	{
		struct sockaddr_in ip4;
		struct sockaddr_in6 ip6;
	} remote;
	ApeNetConnectionState connectionState;

	unsigned char *sendBuffer; /**< Buffer for data waiting to be sent. */
	size_t sendBufferSize;     /**< Size of sendBuffer. */
	size_t sendBufferUsed;     /**< Number of bytes currently in sendBuffer. */

	size_t osSendBufferSize; /**< Size of OS socket send buffer (i.e. SO_SNDBUF). */
} ApeNetSocket;

#define APE_DEFAULT_SOCKET_SEND_BUFFER_SIZE 65536 /* 64kiB */
#define APE_DEFAULT_SOCKET_MAX_SEND_SIZE    4096  /* 4kiB */

#if !defined( NDEBUG )

static struct
{
	ApeNetSocket
	        *hostSocket,
	        *clientSocket,
	        *acceptSocket;
} testData;

static bool execute_test( void )
{
	static const char *ip = "localhost";

	testData.hostSocket = ape_net_open_socket_( ip, 0, true );
	if ( testData.hostSocket == NULL )
	{
		PRINT_WARNING( "Failed to create host socket!\n" );
		return false;
	}

	testData.clientSocket = ape_net_open_socket_( ip, ape_net_get_local_port_( testData.hostSocket ), false );
	if ( testData.clientSocket == NULL )
	{
		PRINT_WARNING( "Failed to create client socket!\n" );
		return false;
	}

	bool accepted = false;
	while ( !accepted && testData.acceptSocket == NULL )
	{
		if ( ape_net_get_connection_status_( testData.clientSocket ) != NET_CONNECTION_PENDING )
			accepted = true;

		testData.acceptSocket = ape_net_accept_( testData.hostSocket );
	}

	const char *s = "Hello World!";
	size_t sl = strlen( s );
	ape_net_send_( testData.acceptSocket, s, sl );

	PRINT( "Sent \"%s\" to %s\n", s, ip );

	char d[ 16 ];
	size_t dl = 0;
	while ( dl < sl )
	{
		if ( ape_net_receive_( testData.clientSocket, d + dl, sizeof( d ) - dl ) > 0 )
			break;
	}

	PRINT( "Received \"%s\" from %s\n", d, ip );

	if ( strncmp( s, d, sl ) != 0 )
	{
		PRINT_WARNING( "Message did not match expected string!\n" );
		return false;
	}

	return true;
}

static void test_net_command( unsigned int argc, char **argv )
{
	PL_ZERO_( testData );

	PRINT( "%s", execute_test() ? "Test passed successfully!\n" : "Test failed!\n" );

	if ( testData.hostSocket != NULL )
		ape_net_close_socket_( testData.hostSocket );
	if ( testData.clientSocket != NULL )
		ape_net_close_socket_( testData.clientSocket );
	if ( testData.acceptSocket != NULL )
		ape_net_close_socket_( testData.acceptSocket );
}

#endif

void ape_initialize_net_( void )
{
#if defined( _WIN32 )
	WSADATA data;
	int r;
	if ( ( r = WSAStartup( MAKEWORD( 2, 2 ), &data ) ) != 0 )
	{
		PRINT_WARNING( "Failed to initialize Winsock: %d\n", r );
	}
#endif

#if !defined( NDEBUG )
	PlRegisterConsoleCommand( "test_net", "Test networking API", 0, test_net_command );
#endif
}

void ape_shutdown_net_( void )
{
#if defined( _WIN32 )
	WSACleanup();
#endif
}

ApeNetSocket *ape_net_open_socket_( const char *ip, unsigned short port, bool isHost )
{
	struct addrinfo hints;
	PL_ZERO_( hints );

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* if ip is null, assume we don't care */
	if ( ip == NULL )
	{
		hints.ai_flags = AI_PASSIVE;
	}

	/* this sucks... getaddrinfo takes port as a string
	 * so we need to convert it here */
	char buf[ 8 ];
	snprintf( buf, sizeof( buf ), PL_FMT_uint16, port );

	struct addrinfo *result;
	int s = getaddrinfo( ip, buf, &hints, &result );
	if ( s != 0 )
	{
		PRINT_WARNING( "Failed to get address info: %s\n", gai_strerror( s ) );
		return NULL;
	}

	ApeNetSocketHandle handle = -1;
	struct addrinfo *r;
	for ( r = result; r != NULL; r = r->ai_next )
	{
		handle = socket( r->ai_family, r->ai_socktype, r->ai_protocol );
		if ( handle == -1 )
		{
			continue;
		}

#if defined( _WIN32 )
		u_long mode = 1;
		ioctlsocket( handle, FIONBIO, &mode );
#else
		fcntl( handle, F_SETFL, O_NONBLOCK );
#endif

		if ( isHost && ( bind( handle, r->ai_addr, r->ai_addrlen ) == 0 ) )
		{
			assert( listen( handle, 8 ) == 0 );
			break;
		}

#if defined( _WIN32 )
		if ( !isHost && ( connect( handle, r->ai_addr, r->ai_addrlen ) == 0 || WSAGetLastError() == WSAEWOULDBLOCK ) )
#else
		if ( !isHost && ( connect( handle, r->ai_addr, r->ai_addrlen ) == 0 || errno == EINPROGRESS ) )
#endif
		{
			break;
		}

#if defined( _WIN32 )
		const char *GetLastError_strerror( uint32_t errnum );
		PRINT( "Unable to bind/connect for socket: %s\n", GetLastError_strerror( WSAGetLastError() ) );
#else
		PRINT( "Unable to bind/connect for socket: %s\n", strerror( errno ) );
#endif

		close_socket( handle );
		handle = -1;
	}

	int addressType;
	if ( r != NULL )
	{
		addressType = r->ai_family;
		if ( addressType != AF_INET && addressType != AF_INET6 )
		{
			PRINT_WARNING( "Unsupported socket type: %u\n", addressType );
			close_socket( handle );
			handle = -1;
		}
	}

	if ( handle == -1 )
	{
		PRINT_WARNING( "Failed to open and connect/bind socket!\n" );
		freeaddrinfo( result );
		return NULL;
	}

	/* all done, allocate and return it */
	ApeNetSocket *netSocket = PlMAllocA( sizeof( ApeNetSocket ) );
	netSocket->connectionState = NET_CONNECTION_PENDING;
	netSocket->handle = handle;
	netSocket->addressType = addressType;

	socklen_t addrSize;
	addrSize = sizeof( netSocket->local );
	getsockname( handle, ( struct sockaddr * ) &netSocket->local.ip6, &addrSize );
	addrSize = sizeof( netSocket->remote );
	getpeername( handle, ( struct sockaddr * ) &netSocket->remote.ip6, &addrSize );

	freeaddrinfo( result );

	ape_net_set_max_send_size_( netSocket, APE_DEFAULT_SOCKET_MAX_SEND_SIZE );
	ape_net_set_send_buffer_size_( netSocket, APE_DEFAULT_SOCKET_SEND_BUFFER_SIZE );

	return netSocket;
}

void ape_net_close_socket_( ApeNetSocket *netSocket )
{
	close_socket( netSocket->handle );
	PlFree( netSocket->sendBuffer );
	PlFree( netSocket );
}

static bool flush_send_buffer( ApeNetSocket *netSocket )
{
	while ( netSocket->sendBufferUsed > 0 )
	{
		ssize_t sent_bytes = send( netSocket->handle, netSocket->sendBuffer, netSocket->sendBufferUsed, 0 );
		if ( sent_bytes < 0 )
		{
#if defined( _WIN32 )
			DWORD error = WSAGetLastError();
			if ( error != WSAEWOULDBLOCK )
			{
				PRINT_WARNING( "Send error on socket (error code %u)\n", ( unsigned ) ( error ) );
				return false;
			}
#else
			if ( errno != EAGAIN && errno != EWOULDBLOCK )
			{
				PRINT_WARNING( "Send error on socket (%s)\n", strerror( errno ) );
				return false;
			}
#endif

			break;
		}

		memmove( netSocket->sendBuffer, ( netSocket->sendBuffer + netSocket->sendBufferUsed ), ( netSocket->sendBufferUsed - sent_bytes ) );
		netSocket->sendBufferUsed -= sent_bytes;
	}

	return true;
}

bool ape_net_send_( ApeNetSocket *netSocket, const void *buf, size_t length )
{
	if ( !flush_send_buffer( netSocket ) )
	{
		return false;
	}

	if ( ( netSocket->sendBufferUsed + length ) > netSocket->sendBufferSize )
	{
		PRINT_WARNING( "Attempted to send %zu bytes on socket, but send buffer has insufficient space\n", length );
		return false;
	}

	memcpy( ( netSocket->sendBuffer + netSocket->sendBufferUsed ), buf, length );
	netSocket->sendBufferUsed += length;

	return flush_send_buffer( netSocket );
}

ssize_t ape_net_receive_( ApeNetSocket *netSocket, void *dst, size_t length )
{
	/* We get called periodically, so use this point to flush any pending
	 * data from our send buffer.
	*/
	flush_send_buffer( netSocket );

	ssize_t r = recv( netSocket->handle, dst, length, 0 );
	if ( r == -1 &&
#if defined( _WIN32 )
	     ( WSAGetLastError() == WSAEWOULDBLOCK )
#else
	     ( errno == EAGAIN || errno == EWOULDBLOCK )
#endif
	)
	{
		r = 0;
	}
	else if ( r == 0 )
	{
		r = -1;
	}

	return r;
}

ApeNetSocket *ape_net_accept_( ApeNetSocket *netSocket )
{
	struct sockaddr_storage buf;
	socklen_t size = sizeof( buf );

	int handle = accept( netSocket->handle, ( struct sockaddr * ) &buf, &size );
	if ( handle >= 0 )
	{
#if defined( _WIN32 )
		u_long mode = 1;
		ioctlsocket( handle, FIONBIO, &mode );
#else
		fcntl( handle, F_SETFL, O_NONBLOCK );
#endif

		ApeNetSocket *out = PlMAllocA( sizeof( ApeNetSocket ) );
		out->handle = handle;

		socklen_t addrSize;
		addrSize = sizeof( netSocket->local );
		getsockname( out->handle, ( struct sockaddr * ) &netSocket->local.ip6, &addrSize );
		addrSize = sizeof( netSocket->remote );
		getpeername( out->handle, ( struct sockaddr * ) &netSocket->remote.ip6, &addrSize );

		ape_net_set_max_send_size_( out, APE_DEFAULT_SOCKET_MAX_SEND_SIZE );
		ape_net_set_send_buffer_size_( out, APE_DEFAULT_SOCKET_SEND_BUFFER_SIZE );

		return out;
	}
	return NULL;
}

ApeNetConnectionState ape_net_get_connection_status_( ApeNetSocket *netSocket )
{
	if ( netSocket->connectionState != NET_CONNECTION_PENDING )
	{
		return netSocket->connectionState;
	}

	fd_set fdWrite;
	FD_ZERO( &fdWrite );
	FD_SET( netSocket->handle, &fdWrite );

	fd_set fdAccept;
	FD_ZERO( &fdAccept );
	FD_SET( netSocket->handle, &fdAccept );

	struct timeval tv;
	PL_ZERO_( tv );

	int s = select( netSocket->handle + 1, NULL, &fdWrite, &fdAccept, &tv );
	if ( s > 0 )
	{
		char errCode;
		socklen_t errLen = sizeof( errCode );
		getsockopt( netSocket->handle, SOL_SOCKET, SO_ERROR, &errCode, &errLen );
		if ( errCode == 0 )
		{
			return ( netSocket->connectionState = NET_CONNECTION_CONNECTED );
		}

		ape_warning_( "Connection failed with error code %u!\n", errCode );
		return ( netSocket->connectionState = NET_CONNECTION_FAILED );
	}

	return NET_CONNECTION_PENDING;
}

unsigned short ape_net_get_local_port_( ApeNetSocket *netSocket )
{
	return ntohs( ( netSocket->addressType == NET_IP4 ) ? netSocket->local.ip4.sin_port : netSocket->local.ip6.sin6_port );
}

unsigned short ape_net_get_remote_port_( ApeNetSocket *netSocket )
{
	return ntohs( ( netSocket->addressType == NET_IP4 ) ? netSocket->remote.ip4.sin_port : netSocket->remote.ip6.sin6_port );
}

bool ape_net_set_max_send_size_( ApeNetSocket *netSocket, size_t maxSendSize )
{
	if ( maxSendSize < netSocket->sendBufferUsed )
	{
		return false;
	}

	void *newSendBuffer = PlReAlloc( netSocket->sendBuffer, maxSendSize, false );
	if ( newSendBuffer == NULL )
	{
		return false;
	}

	netSocket->sendBuffer = newSendBuffer;
	netSocket->sendBufferSize = maxSendSize;

	return true;
}

size_t ape_net_get_max_send_size_( ApeNetSocket *netSocket )
{
	return netSocket->sendBufferSize;
}

bool ape_net_set_send_buffer_size_( ApeNetSocket *netSocket, size_t sendBufferSize )
{
	assert( sendBufferSize < INT_MAX );

	int sndbuf = ( int ) sendBufferSize;
	int res = setsockopt( netSocket->handle, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof( sndbuf ) );
	if ( res != 0 )
	{
#ifdef _WIN32
		ape_warning_( "Error setting SO_SNDBUF socket option (error code %u)\n", ( unsigned ) ( WSAGetLastError() ) );
#else
		char tmp[ 256 ];
		if ( strerror_r( errno, tmp, sizeof( tmp ) ) != 0 )
		{
			snprintf( tmp, sizeof( tmp ), "error code %u", errno );
		}

		ape_warning_( "Error setting SO_SNDBUF socket option (%s)\n", tmp );
#endif
		return false;
	}

	netSocket->osSendBufferSize = sendBufferSize;
	return true;
}

size_t ape_net_get_send_buffer_size_( ApeNetSocket *netSocket )
{
	return netSocket->osSendBufferSize;
}
