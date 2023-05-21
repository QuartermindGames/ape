/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

#include "net/net.h"
#include "core_protocol.h"

PL_EXTERN_C

typedef struct ServerClient ServerClient;

bool Server_Start( const char *ip, unsigned short port );

void ogeInitializeServer( void );
void YnCore_ShutdownServer( void );
void Server_DropClient( ServerClient *serverClient );
void YnCore_TickServer( void );

unsigned short Server_GetPort( void );

PL_EXTERN_C_END
