/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "core_private.h"
#include "net.h"

#if defined( _WIN32 )
#	include <WinSock2.h>
#	include <WS2tcpip.h>
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
typedef SOCKET NetSocketHandle;
#else
typedef int NetSocketHandle;
#endif

static void CloseSocket( NetSocketHandle handle )
{
#if defined( _MSC_VER )
	closesocket( handle );
#else
	close( handle );
#endif
}

typedef struct NetSocket
{
	NetSocketHandle handle;      /* system socket handle */
	int             addressType; /* ip4 / ip6 */
	union
	{
		struct sockaddr_in  ip4;
		struct sockaddr_in6 ip6;
	} local;
	union
	{
		struct sockaddr_in  ip4;
		struct sockaddr_in6 ip6;
	} remote;
	NetConnectionState connectionState;
} NetSocket;

#if !defined( NDEBUG )

static struct
{
	NetSocket
	        *hostSocket,
	        *clientSocket,
	        *acceptSocket;
} testData;

static bool ExecuteTest( void )
{
	static const char *ip = "localhost";

	testData.hostSocket = Net_OpenSocket( ip, 0, true );
	if ( testData.hostSocket == NULL )
	{
		PRINT_WARNING( "Failed to create host socket!\n" );
		return false;
	}

	testData.clientSocket = Net_OpenSocket( ip, Net_GetLocalPort( testData.hostSocket ), false );
	if ( testData.clientSocket == NULL )
	{
		PRINT_WARNING( "Failed to create client socket!\n" );
		return false;
	}

	bool accepted = false;
	while ( !accepted && testData.acceptSocket == NULL )
	{
		if ( Net_GetConnectionStatus( testData.clientSocket ) != NET_CONNECTION_PENDING )
			accepted = true;

		testData.acceptSocket = Net_Accept( testData.hostSocket );
	}

	const char *s = "Hello World!";
	size_t      sl = strlen( s );
	Net_Send( testData.acceptSocket, s, sl );

	PRINT( "Sent \"%s\" to %s\n", s, ip );

	char   d[ 16 ];
	size_t dl = 0;
	while ( dl < sl )
	{
		if ( Net_Receive( testData.clientSocket, d + dl, sizeof( d ) - dl ) > 0 )
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

static void TestNetCommand( unsigned int argc, char **argv )
{
	PL_ZERO_( testData );

	PRINT( "%s", ExecuteTest() ? "Test passed successfully!\n" : "Test failed!\n" );

	if ( testData.hostSocket != NULL )
		Net_CloseSocket( testData.hostSocket );
	if ( testData.clientSocket != NULL )
		Net_CloseSocket( testData.clientSocket );
	if ( testData.acceptSocket != NULL )
		Net_CloseSocket( testData.acceptSocket );
}

#endif

void apeInitializeNet( void )
{
#if defined( _WIN32 )
	WSADATA data;
	int     r;
	if ( ( r = WSAStartup( MAKEWORD( 2, 2 ), &data ) ) != 0 )
		PRINT_WARNING( "Failed to initialize Winsock: %d\n", r );
#endif

#if !defined( NDEBUG )
	PlRegisterConsoleCommand( "test_net", "Test networking API", 0, TestNetCommand );
#endif
}

void ogeShutdownNet( void )
{
#if defined( _WIN32 )
	WSACleanup();
#endif
}

NetSocket *Net_OpenSocket( const char *ip, unsigned short port, bool isHost )
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
	int              s = getaddrinfo( ip, buf, &hints, &result );
	if ( s != 0 )
	{
		PRINT_WARNING( "Failed to get address info: %s\n", gai_strerror( s ) );
		return NULL;
	}

	NetSocketHandle  handle = -1;
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

		CloseSocket( handle );
		handle = -1;
	}

	int addressType;
	if ( r != NULL )
	{
		addressType = r->ai_family;
		if ( addressType != AF_INET && addressType != AF_INET6 )
		{
			PRINT_WARNING( "Unsupported socket type: %u\n", addressType );
			CloseSocket( handle );
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
	NetSocket *netSocket = PlMAllocA( sizeof( NetSocket ) );
	netSocket->connectionState = NET_CONNECTION_PENDING;
	netSocket->handle = handle;
	netSocket->addressType = addressType;

	socklen_t addrSize;
	addrSize = sizeof( netSocket->local );
	getsockname( handle, ( struct sockaddr * ) &netSocket->local.ip6, &addrSize );
	addrSize = sizeof( netSocket->remote );
	getpeername( handle, ( struct sockaddr * ) &netSocket->remote.ip6, &addrSize );

	freeaddrinfo( result );

	return netSocket;
}

void Net_CloseSocket( NetSocket *netSocket )
{
	CloseSocket( netSocket->handle );
	PlFree( netSocket );
}

ssize_t Net_Send( NetSocket *netSocket, const void *buf, ssize_t length )
{
	return send( netSocket->handle, buf, length, 0 );
}

ssize_t Net_Receive( NetSocket *netSocket, void *dst, ssize_t length )
{
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

NetSocket *Net_Accept( NetSocket *netSocket )
{
	struct sockaddr_storage buf;
	socklen_t               size = sizeof( buf );

	int handle = accept( netSocket->handle, ( struct sockaddr * ) &buf, &size );
	if ( handle >= 0 )
	{
#if defined( _WIN32 )
		u_long mode = 1;
		ioctlsocket( handle, FIONBIO, &mode );
#else
		fcntl( handle, F_SETFL, O_NONBLOCK );
#endif

		NetSocket *out = PlMAllocA( sizeof( NetSocket ) );
		out->handle = handle;

		socklen_t addrSize;
		addrSize = sizeof( netSocket->local );
		getsockname( out->handle, ( struct sockaddr * ) &netSocket->local.ip6, &addrSize );
		addrSize = sizeof( netSocket->remote );
		getpeername( out->handle, ( struct sockaddr * ) &netSocket->remote.ip6, &addrSize );

		return out;
	}
	return NULL;
}

NetConnectionState Net_GetConnectionStatus( NetSocket *netSocket )
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
		char      errCode;
		socklen_t errLen = sizeof( errCode );
		getsockopt( netSocket->handle, SOL_SOCKET, SO_ERROR, &errCode, &errLen );
		if ( errCode == 0 )
		{
			return ( netSocket->connectionState = NET_CONNECTION_CONNECTED );
		}

		return ( netSocket->connectionState = NET_CONNECTION_FAILED );
	}

	return NET_CONNECTION_PENDING;
}

unsigned short Net_GetLocalPort( NetSocket *netSocket )
{
	return ntohs( ( netSocket->addressType == NET_IP4 ) ? netSocket->local.ip4.sin_port : netSocket->local.ip6.sin6_port );
}

unsigned short Net_GetRemotePort( NetSocket *netSocket )
{
	return ntohs( ( netSocket->addressType == NET_IP4 ) ? netSocket->remote.ip4.sin_port : netSocket->remote.ip6.sin6_port );
}
