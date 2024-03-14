// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

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

#define SS_FORGE_APP_VERSION "v0.1.0"

static inline constexpr const char *FORGE_APP_NAME = "forge";
static inline constexpr const char *FORGE_APP_TITLE = "Forge";

static inline constexpr uint8_t FORGE_VERSION_MAJOR = 0;
static inline constexpr uint8_t FORGE_VERSION_MINOR = 0;
static inline constexpr uint8_t FORGE_VERSION_PATCH = 0;

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

namespace ss::forge
{
	FXGLVisual *get_shared_gl_visual();

	FXIcon *load_fx_icon( FXApp *app, const char *path );

	enum ThemeColour : uint8_t
	{
		THEME_COLOUR_BASE,
		THEME_COLOUR_FORE,
		THEME_COLOUR_HILITE,
		THEME_COLOUR_BACK,

		MAX_THEME_COLOURS
	};
	extern FXColor themeColours[ ThemeColour::MAX_THEME_COLOURS ];

	/////////////////////////////////////////////////////////////////////////

	struct Project
	{
		explicit Project( const std::string &name )
		    : name( name )
		{
		}

		std::string name;
		std::string internalName;
		NdBranch *config{};
		PLFileSystemMount *mount{};
		FXIcon *icon{};
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
		PATH_COOK,     // cooking tool

		MAX_CACHED_PATHS
	};
	extern PLPath cachedPaths[];

	extern bool isCookAvailable;

	extern NdBranch *editorConfig;

	extern ApeEditorState *engineEditorState;
}// namespace ss::forge
