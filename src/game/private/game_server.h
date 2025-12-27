// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape/ape_public_server.h"

typedef enum GameServerClientState
{
	GAME_SERVER_CLIENT_STATE_SPECTATING,
	GAME_SERVER_CLIENT_STATE_SPAWNED,
} GameServerClientState;

typedef struct GameServerClient
{
	ApeServerClient *internalHandle;
	PLHashTableNode *hashTableNode;

	unsigned int slot;
	GamePlayer  *playerSlot;

	GameServerClientState state;
} GameServerClient;

bool game_server_client_validate_( ApeServerClient *clientHandle );
void game_server_client_connected_( ApeServerClient *clientHandle );
void game_server_client_disconnected_( ApeServerClient *clientHandle );
void game_server_process_message_( ApeServerClient *clientHandle, const void *buf, size_t bufSize );
bool game_server_send_message_( ApeServerClient *clientHandle, GameNetMessageType type, const void *buf, size_t bufSize );
void game_server_tick_( double delta );

GameServerClient *game_server_get_host_client_();
GamePlayer       *game_server_get_host_player_();
ApeEntity        *game_server_get_host_entity_();
GameServerClient *game_server_get_client_( unsigned int slot );

unsigned int game_server_get_num_clients_();
unsigned int game_server_get_num_players_();

/**
 * Similar to server_send_message, but attempts to issue the same message to all clients.
 *
 * @param type		The type of message.
 * @param buf		Buffer holding the message data.
 * @param bufSize	Size of the buffer.
 */
void game_server_broadcast_message_( GameNetMessageType type, const void *buf, size_t bufSize );

void game_server_print_( ApeServerClient *clientHandle, const char *message );
