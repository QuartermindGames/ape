// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "plcore/pl.h"
#include "plcore/pl_string.h"
#include "plcore/pl_memory.h"

#include "yin/core.h"
#include "yin/node.h"

#define FORGE_APP_NAME  "forge"
#define FORGE_APP_TITLE "Forge"

#define FORGE_APP_VERSION_MAJOR 1
#define FORGE_APP_VERSION_MINOR 0
#define FORGE_APP_VERSION_PATCH 0
#define FORGE_APP_VERSION_BUILD GIT_COMMIT_COUNT

//void forge_message_box( GtkWindow *window, GtkMessageType type, const char *msg, ... );

GtkWindow *forge_get_main_window( void );
const char *forge_get_window_title( void );

/////////////////////////////////////////////////////////////////////////////////////

typedef struct ForgeProject
{
	char name[ 64 ];
	char internalName[ 64 ];
} ForgeProject;

void forge_project_show_selector( void );
const ForgeProject *forge_project_scan( unsigned int *num );
