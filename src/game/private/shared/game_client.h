// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape/ape_public_client.h"

void game_client_connected_();
void game_client_disconnected_();
void game_client_process_message_( const void *buf, size_t bufSize );
bool game_client_send_message_( GameNetMessageType type, const void *buf, size_t bufSize );
