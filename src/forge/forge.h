// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "plcore/pl_string.h"
#include "plcore/pl_memory.h"

#define FORGE_APP_NAME  "forge"
#define FORGE_APP_TITLE "Forge"

#define FORGE_APP_VERSION_MAJOR 0
#define FORGE_APP_VERSION_MINOR 1
#define FORGE_APP_VERSION_PATCH 0

void forge_message_box( const char *msg, ... );

const char *forge_get_window_title( void );
