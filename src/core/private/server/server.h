/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

#include "net/net.h"
#include "core_protocol.h"

PL_EXTERN_C

typedef struct ApeServerClient ApeServerClient;

bool apeStartServer( const char *ip, unsigned short port );

void apeInitializeServer( void );
void apeShutdownServer( void );
void apeDropServerClient( ApeServerClient *serverClient );
void apeTickServer( void );

unsigned short apeGetServerPort( void );

PL_EXTERN_C_END
