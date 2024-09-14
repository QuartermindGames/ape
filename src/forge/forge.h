// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <stdexcept>
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
#include "acm/public/acm/acm.h"

// FOX Toolkit
#include <fx.h>
#include <fxkeys.h>

#define FORGE_APP_VERSION "v0.1.0"

static inline constexpr const char *FORGE_APP_NAME  = "forge";
static inline constexpr const char *FORGE_APP_TITLE = "Forge";

static inline constexpr uint8_t FORGE_VERSION_MAJOR = 0;
static inline constexpr uint8_t FORGE_VERSION_MINOR = 1;
static inline constexpr uint8_t FORGE_VERSION_PATCH = 0;

#define FORGE_CONFIG_FILENAME "editor"

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

enum ForgeIconType
{
	FORGE_ICON_TYPE_MODE_BRUSH,
	FORGE_ICON_TYPE_MODE_EDGE,
	FORGE_ICON_TYPE_MODE_FACE,

	FORGE_ICON_TYPE_NODE,
	FORGE_ICON_TYPE_WORLD,
	FORGE_ICON_TYPE_ROOM,
	FORGE_ICON_TYPE_BRUSH,
	FORGE_ICON_TYPE_CAMERA,
	FORGE_ICON_TYPE_ENTITY,
	FORGE_ICON_TYPE_LIGHT,

	FORGE_ICON_TYPE_TEXTURE,

	FORGE_ICON_TYPE_NEW,
	FORGE_ICON_TYPE_NEW_BRUSH,
	FORGE_ICON_TYPE_NEW_CAMERA,
	FORGE_ICON_TYPE_NEW_LIGHT,
	FORGE_ICON_TYPE_NEW_MATERIAL,
	FORGE_ICON_TYPE_NEW_ROOM,
	FORGE_ICON_TYPE_NEW_WORLD,

	FORGE_ICON_TYPE_OPEN,
	FORGE_ICON_TYPE_OPEN_MATERIAL,
	FORGE_ICON_TYPE_OPEN_MODEL,
	FORGE_ICON_TYPE_OPEN_WORLD,

	FORGE_ICON_TYPE_CLOSE,

	FORGE_ICON_TYPE_CAMERA_FORWARD,

	MAX_FORGE_ICONS
};
extern FXIcon *forge_cachedIcons[ MAX_FORGE_ICONS ];

namespace forge
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
		AcmBranch  *config{};
		FXIcon     *icon{};
	};
	extern Project *editorProject;

	Project *create_project( const std::string &name, const std::string &folderName );
	bool     open_project( const char *path );

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

	extern AcmBranch *editorConfig;
}// namespace forge
