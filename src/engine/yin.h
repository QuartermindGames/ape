/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/platform.h>
#include <PL/platform_console.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_package.h>
#include <PL/pl_graphics.h>
#include <PL/pl_graphics_camera.h>

#include <assert.h>

#include "shared/interfaces.h"

enum {
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARN,
	LOG_LEVEL_INFO,
};

//#define DEBUG_CAM
//#define DEBUG_WALL_NORMALS

typedef struct SysWindow SysWindow;

#define WINDOW_TITLE    "Yin Technology Demo"
#define WINDOW_WIDTH    1024
#define WINDOW_HEIGHT   768

#define DISPLAY_WIDTH   1024
#define DISPLAY_HEIGHT  768

#define PrintError( ... ) plLogMessage( LOG_LEVEL_ERROR, __VA_ARGS__ ); exit( EXIT_FAILURE )
#define PrintWarn( ... )  plLogMessage( LOG_LEVEL_WARN, __VA_ARGS__ )
#define PrintMsg( ... )   plLogMessage( LOG_LEVEL_INFO, __VA_ARGS__ )

extern PLPackage *globalWad;
#define YIN_GLOBAL_WAD "yin.wad"

SysWindow *Engine_GetMainWindow( void );

unsigned int Engine_GetNumTicks( void );
