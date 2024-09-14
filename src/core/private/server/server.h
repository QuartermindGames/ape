// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "net/net.h"
#include "ape_protocol.h"

PL_EXTERN_C

typedef struct ApeServerClient ApeServerClient;

bool ape_server_start( const char *ip, unsigned short port );

void ape_initialize_server_( void );
void ape_shutdown_server_( void );

void ape_server_drop_client_( ApeServerClient *serverClient );
void ape_tick_server_( void );

unsigned short ape_server_get_port_( void );

PL_EXTERN_C_END
