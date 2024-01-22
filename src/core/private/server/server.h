// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "net/net.h"
#include "ape_protocol.h"

PL_EXTERN_C

typedef struct ApeServerClient ApeServerClient;

bool ss_acl_start_server_( const char *ip, unsigned short port );

void ss_acl_initialize_server_( void );
void ss_acl_shutdown_server_( void );

void ape_server_drop_client_( ApeServerClient *serverClient );
void ape_server_tick_( void );

unsigned short ape_server_get_port_( void );

PL_EXTERN_C_END
