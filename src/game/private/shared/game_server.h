// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape/ape_public_server.h"

typedef struct GameServerClient
{
	ApeServerClientHandle *internalHandle;
	PLHashTableNode       *hashTableNode;

	unsigned int slot;
	GamePlayer  *playerSlot;
} GameServerClient;

bool game_server_client_validate_( ApeServerClientHandle *clientHandle );
void game_server_client_connected_( ApeServerClientHandle *clientHandle );
void game_server_client_disconnected_( ApeServerClientHandle *clientHandle );
void game_server_process_message_( ApeServerClientHandle *clientHandle, const void *buf, size_t bufSize );
bool game_server_send_message_( ApeServerClientHandle *clientHandle, GameNetMessageType type, const void *buf, size_t bufSize );

GameServerClient *game_server_get_host_client_();
GamePlayer       *game_server_get_host_player_();
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

void game_server_print_( ApeServerClientHandle *clientHandle, const char *message );
