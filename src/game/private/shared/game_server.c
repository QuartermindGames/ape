// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "game_private.h"
#include "game_server.h"

#include "entities/entity_player_spawn.h"

#include <common_project.h>

static PLHashTable *serverClientsLookup;//GameServerClient

//TODO: both serverClients and players would probably be better off as a linked list

static GameServerClient serverClients[ GAME_MAX_CLIENTS ];
static unsigned int     numServerClients;

static GamePlayer   players[ GAME_MAX_PLAYERS ];
static unsigned int numPlayers;

void game_server_initialize_()
{
	serverClientsLookup = PlCreateHashTable();

	PL_ZERO_( serverClients );
}

bool game_server_client_validate_( PL_UNUSED ApeServerClient *clientHandle )
{
	if ( numServerClients >= GAME_MAX_CLIENTS )
	{
		game_warning_( "Max clients limit hit, rejecting new client!\n" );
		return false;
	}

	// if it's over the player limit, they'll get put in some sort of
	// lobby-state, so we skip that check here...

	return true;
}

static void spawn_player( GamePlayer *self );

static void assign_client_to_player( GameServerClient *client )
{
	if ( numPlayers >= GAME_MAX_PLAYERS )
	{
		game_warning_( "Max players limit hit, rejecting new player!\n" );
		return;
	}

	GamePlayer *player = nullptr;
	for ( UInt i = 0; i < GAME_MAX_PLAYERS; ++i )
	{
		player = &players[ i ];
		if ( player->serverClient == nullptr )
		{
			break;
		}
	}

	if ( player == nullptr || player->serverClient != nullptr )
	{
		game_warning_( "Failed to find an open player slot, rejecting new player!\n" );
		return;
	}

	int team = game_team_assign( player );
	if ( team == -1 )
	{
		return;
	}

	player->team         = team;
	player->serverClient = client;
	client->playerSlot   = player;

	spawn_player( player );
}

void game_server_client_connected_( ApeServerClient *clientHandle )
{
	GameServerClient *serverClient = &serverClients[ numServerClients ];
	serverClient->slot             = numServerClients;
	serverClient->internalHandle   = clientHandle;
	PlInsertHashTableNode( serverClientsLookup, clientHandle, sizeof( ApeServerClient * ), serverClient );
	numServerClients++;

	serverClient->state = GAME_SERVER_CLIENT_STATE_SPECTATING;

	assign_client_to_player( serverClient );
}

void game_server_client_disconnected_( ApeServerClient *clientHandle )
{
	GameServerClient *serverClient = PlLookupHashTableUserData( serverClientsLookup, clientHandle, sizeof( ApeServerClient * ) );
	assert( serverClient != nullptr );
	PlDestroyHashTableNode( serverClient->hashTableNode );
	serverClient->slot = 0;
}

void game_server_process_message_( ApeServerClient *clientHandle, const void *buf, size_t bufSize )
{
	const GameNetMessageHeader *header = buf;
	switch ( header->type )
	{
		default:
			game_warning_( "Unhandled client message (%u)!\n", header->type );
			break;
		case GAME_NET_MESSAGE_SAY:
		{
			GameServerClient *otherClient;
			COM_ITERATE_HASHED_LIST( otherClient, serverClientsLookup, i )
			{
				ape_server_send( otherClient->internalHandle, ( const void *[] ) { buf }, ( size_t[] ) { bufSize }, 1 );
			}
			break;
		}
	}
}

bool game_server_send_message_( ApeServerClient *clientHandle, GameNetMessageType type, const void *buf, size_t bufSize )
{
	const void *items[] = {
	        &( GameNetMessageHeader ) { .type = type },
	        buf,
	};
	size_t sizes[] = {
	        sizeof( GameNetMessageHeader ),
	        bufSize,
	};

	return ape_server_send( clientHandle, items, sizes, PL_ARRAY_ELEMENTS( items ) );
}

void game_server_tick_( double delta )
{
	// iterate over connected clients, and see if any are waiting for a free slot
	for ( UInt i = 0; i < GAME_MAX_CLIENTS; ++i )
	{
		if ( serverClients[ i ].internalHandle == nullptr || serverClients[ i ].playerSlot != nullptr )
		{
			continue;
		}

		assign_client_to_player( &serverClients[ i ] );
	}
}

GameServerClient *game_server_get_host_client_()
{
	if ( ape_is_dedicated() )
	{
		return nullptr;
	}

	// this assumes the first slot is the host
	// if it's not dedicated... which might be dumb
	return game_server_get_client_( 0 );
}

GamePlayer *game_server_get_host_player_()
{
	GameServerClient *client = game_server_get_host_client_();
	if ( client == nullptr )
	{
		return nullptr;
	}

	return client->playerSlot;
}

GameServerClient *game_server_get_client_( unsigned int slot )
{
	assert( slot < GAME_MAX_CLIENTS );
	return &serverClients[ slot ];
}

unsigned int game_server_get_num_clients_()
{
	return numServerClients;
}

unsigned int game_server_get_num_players_()
{
	return numPlayers;
}

void game_server_broadcast_message_( GameNetMessageType type, const void *buf, size_t bufSize )
{
	const void *items[] = {
	        &( GameNetMessageHeader ) { .type = type },
	        buf,
	};
	size_t sizes[] = {
	        sizeof( GameNetMessageHeader ),
	        bufSize,
	};

	PLHashTableNode *hashNode = PlGetFirstHashTableNode( serverClientsLookup );
	while ( hashNode != nullptr )
	{
		GameServerClient *client = PlGetHashTableNodeUserData( hashNode );
		hashNode                 = PlGetNextHashTableNode( hashNode );
		if ( !ape_server_send( client->internalHandle, items, sizes, PL_ARRAY_ELEMENTS( items ) ) )
		{
			game_warning_( "Failed to send message to client %p!\n", client->internalHandle );
		}
	}
}

void game_server_print_( ApeServerClient *clientHandle, const char *message )
{
	size_t messageSize = strlen( message );
	if ( messageSize > GAME_NET_MAX_SAY_MESSAGE )
	{
		game_warning_( "Invalid say message length (%u > %u)!\n", messageSize, GAME_NET_MAX_SAY_MESSAGE );
		return;
	}

	game_print_( "%s", message );

	if ( clientHandle == nullptr )
	{
		game_server_broadcast_message_( GAME_NET_MESSAGE_ANNOUNCE, message, strlen( message ) );
		return;
	}

	game_server_send_message_( clientHandle, GAME_NET_MESSAGE_ANNOUNCE, message, strlen( message ) );
}

/////////////////////////////////////////////////////////////////////////////////////
// Player
// TODO: should probably go out into its own source file eventually

static void spawn_player( GamePlayer *self )
{
	static const char *playerClassName;
	if ( playerClassName == nullptr )
	{
		AcmBranch *root = com_project_get_config();
		assert( root != nullptr );
		playerClassName = acm_get_string( root, "playerClassName", nullptr );
	}

	if ( playerClassName == nullptr )
	{
		game_warning_( "Player spawn class not specified in config (see \"playerClassName\")!\n" );
		return;
	}

	// lookup a spawn point...
	//TODO: this is all placeholder logic

	PLLinkedList *playerSpawns = game_player_spawn_get_spawn_points();
	if ( playerSpawns == nullptr )
	{
		game_warning_( "Unable to spawn player, no spawn points!\n" );
		return;
	}

	ApeEntity *entity = PlGetLinkedListNodeUserData( PlGetFirstNode( playerSpawns ) );
	ApeRoom   *room   = ape_world_node_get_room( APE_WORLD_NODE( entity ) );
	if ( room == nullptr )
	{
		game_warning_( "Encountered a player spawn outside a room!\n" );
		return;
	}

	PLVector3 pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );
	PLVector3 ang = ape_world_node_get_angles( APE_WORLD_NODE( entity ) );

	self->entity = ape_entity_create( APE_WORLD_NODE( room ), playerClassName, "player", nullptr, &pos, &ang );

	ape_entity_spawn( self->entity );
	self->serverClient->state = GAME_SERVER_CLIENT_STATE_SPAWNED;
}

const char *game_player_get_name_( const GamePlayer *self )
{
	assert( self->serverClient != nullptr );
	return ape_server_get_client_name( self->serverClient->internalHandle );
}
