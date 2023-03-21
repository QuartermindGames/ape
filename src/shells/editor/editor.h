// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <vector>

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

#define EDITOR_APP_NAME    "yin-editor"
#define EDITOR_APP_TITLE   "Yin Editor"
#define EDITOR_APP_VERSION "v0.1.0"

#define EDITOR_CONFIG_FILENAME "editor.cfg.n"

typedef enum EditorLogLevel
{
	EDITOR_LOG_PRINT,
	EDITOR_LOG_WARNING,
	EDITOR_LOG_ERROR,
	EDITOR_LOG_DEBUG,

	EDITOR_MAX_LOG_LEVELS
} EditorLogLevel;
extern int editorLogLevels[];

#define EDITOR_PRINT( ... ) PlLogMessage( editorLogLevels[ EDITOR_LOG_PRINT ], __VA_ARGS__ )
#define EDITOR_WARN( ... )  PlLogMessage( editorLogLevels[ EDITOR_LOG_WARNING ], __VA_ARGS__ )

void MainWindow_UpdateStatus( const char *message );

namespace os::editor
{
	extern FXWindow *editorWindow;

	FXIcon *LoadFXIcon( FXApp *app, const char *path );

	/////////////////////////////////////////////////////////////////////////

	struct Project
	{
		Project( const char *name ) {}

		PLPath path{};
		YNNodeBranch *config{ nullptr };
		PLFileSystemMount *mount{ nullptr };
		const char *name{ nullptr };
	};
	extern Project editorProject;

	Project *CreateProject( const char *name, const char *folderName );
	Project *OpenProject( const char *path );

	/////////////////////////////////////////////////////////////////////////

	enum
	{
		PATH_EXE,
		PATH_RESOURCES,
		PATH_CONFIG,
		PATH_PROJECTS,
		PATH_REPO,

		MAX_CACHED_PATHS
	};
	extern PLPath cachedPaths[];

	extern YNNodeBranch *editorConfig;
}// namespace os::editor
