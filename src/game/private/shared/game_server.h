// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape/ape_public_server.h"

typedef struct GameServerClient
{
	ApeServerClientHandle *internalHandle;
	PLHashTableNode *hashTableNode;
} GameServerClient;

void game_server_client_connected_( ApeServerClientHandle *clientHandle );
void game_server_client_disconnected_( ApeServerClientHandle *clientHandle );
void game_server_process_message_( ApeServerClientHandle *clientHandle, const void *buf, size_t bufSize );
bool game_server_send_message_( ApeServerClientHandle *clientHandle, GameNetMessageType type, const void *buf, size_t bufSize );
