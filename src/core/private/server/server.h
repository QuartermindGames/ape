// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "../public/ape/ape_public_server.h"

#include "net/net.h"
#include "ape_protocol.h"

PL_EXTERN_C

void ape_initialize_server_( void );
void ape_shutdown_server_( void );

void ape_server_drop_client_( ApeServerClient *serverClient );
void ape_tick_server_( double delta );

PL_EXTERN_C_END
