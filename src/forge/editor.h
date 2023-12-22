// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <vector>
#include <map>
#include <string>

// Hei Library
#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>
#include <plcore/pl_console.h>
#include <plcore/pl_hashtable.h>
#include <plcore/pl_linkedlist.h>

#include "common.h"

// Yin
#include <yin/core.h>
#include <yin/node.h>

// FOX Toolkit
#include <fx.h>
#include <fxkeys.h>

#define SS_FORGE_APP_NAME    "forge-editor"
#define SS_FORGE_APP_TITLE   "Forge Editor"
#define SS_FORGE_APP_VERSION "v0.1.0"

#define EDITOR_CONFIG_FILENAME "editor"

typedef enum EditorLogLevel
{
	EDITOR_LOG_PRINT,
	EDITOR_LOG_WARNING,
	EDITOR_LOG_ERROR,
	EDITOR_LOG_DEBUG,

	EDITOR_MAX_LOG_LEVELS
} EditorLogLevel;
extern int editorLogLevels[ EDITOR_MAX_LOG_LEVELS ];

#define EDITOR_PRINT( ... ) PlLogMessage( editorLogLevels[ EDITOR_LOG_PRINT ], __VA_ARGS__ )
#define EDITOR_WARN( ... )  PlLogMessage( editorLogLevels[ EDITOR_LOG_WARNING ], __VA_ARGS__ )

namespace ss::forge
{
	extern FXWindow *editorWindow;

	FXIcon *load_fx_icon( FXApp *app, const char *path );

	/////////////////////////////////////////////////////////////////////////

	struct Project
	{
		explicit Project( const std::string &name )
		    : name( name )
		{
		}

		std::string name;
		std::string internalName;
		NdBranch *config{ nullptr };
		PLFileSystemMount *mount{ nullptr };
		FXIcon *icon;
	};
	extern Project *editorProject;

	Project *create_project( const std::string &name, const std::string &folderName );
	bool open_project( const char *path );

	/////////////////////////////////////////////////////////////////////////

	enum
	{
		PATH_EXE,      // where the exe is located
		PATH_RESOURCES,// general resources
		PATH_CONFIG,   // location of our config
		PATH_PROJECTS, // location where *all* projects are stored
		PATH_PROJECT,  // current project

		MAX_CACHED_PATHS
	};
	extern PLPath cachedPaths[];

	extern NdBranch *editorConfig;
}// namespace ss::forge
