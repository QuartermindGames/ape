// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"
#include "game_client.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static void say_command( unsigned int argc, char **argv )
{
	if ( !ape_is_client_connected() )
	{
		return;
	}

	const char *message = argv[ 1 ];
	size_t messageSize = strlen( message );
	if ( messageSize > GAME_NET_MAX_SAY_MESSAGE )
	{
		game_warning_( "Invalid say message length (%u > %u)!\n", messageSize, GAME_NET_MAX_SAY_MESSAGE );
		return;
	}

	uint16_t size = messageSize;
	const void *items[] = {
	        &( GameNetMessageHeader ){ .type = GAME_NET_MESSAGE_SAY },
	        &size,
	        message,
	};
	size_t sizes[] = {
	        sizeof( GameNetMessageHeader ),
	        sizeof( size ),
	        messageSize,
	};

	if ( !ape_client_send( items, sizes, PL_ARRAY_ELEMENTS( items ) ) )
	{
		game_warning_( "Failed to send \"say\" command!\n" );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void game_client_initialize_()
{
	//todo: check if we're running in dedicated mode

	PlRegisterConsoleCommand( "game.say",
	                          "Broadcast a text message to other clients.",
	                          1, say_command );
}

void game_client_process_message_( const void *buf, size_t bufSize )
{
	const GameNetMessageHeader *header = ( const GameNetMessageHeader * ) buf;
	switch ( header->type )
	{
		case GAME_NET_MESSAGE_SAY:
		{
			uint16_t *p = ( uint16_t * ) ( header + 1 );
			char buf[ GAME_NET_MAX_SAY_MESSAGE + 1 ] = {};
			strncpy( buf, ( char * ) ( p + 1 ), *p );
			game_print_( "%s\n", buf );
			break;
		}
	}
}

bool game_client_send_message_( GameNetMessageType type, const void *buf, size_t bufSize )
{
	const void *items[] = {
	        &( GameNetMessageHeader ){ .type = type },
	        buf,
	};
	size_t sizes[] = {
	        sizeof( GameNetMessageHeader ),
	        bufSize,
	};

	return ape_client_send( items, sizes, PL_ARRAY_ELEMENTS( items ) );
}
