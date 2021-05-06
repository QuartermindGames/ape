/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl.h>
#include <plcore/pl_console.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>
#include <plcore/pl_linkedlist.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>
#include <plgraphics/plg_polygon.h>

#include <assert.h>

#include "common/common.h"

//#define DISCORD_INTEGRATION

#define ENGINE_APP_NAME "yin"

#define ENGINE_VERSION_MAJOR 2
#define ENGINE_VERSION_MINOR 0
#define ENGINE_VERSION_PATCH 0

extern const int ENGINE_VERSION[ 3 ];
#define ENGINE_VERSION_STR PL_TOSTRING( ENGINE_VERSION_MAJOR ) \
"." PL_TOSTRING( ENGINE_VERSION_MINOR ) "." PL_TOSTRING( ENGINE_VERSION_PATCH )

extern int LOG_LEVEL_ERROR, LOG_LEVEL_WARN, LOG_LEVEL_INFO;

typedef enum CacheGroup {
	CACHE_GROUP_STATIC, /* these exist from the start to the end of the application */
	CACHE_GROUP_WORLD,  /* everything that is cached during level load */
	MAX_CACHE_GROUPS
} CacheGroup;

#if !defined( NDEBUG )
#define ENABLE_PROFILER 1
#endif

typedef enum CPUProfilerGroup {
	PROFILE_DRAW_ALL,
	PROFILE_DRAW_MAP,
	MAX_PROFILER_GROUPS
} CPUProfilerGroup;
#if defined( ENABLE_PROFILER )
void CPUTimer_Initialize( void );
void CPUTimer_StartMeasure( CPUProfilerGroup group );
void CPUTimer_EndMeasure( CPUProfilerGroup group );
double CPUTimer_GetMeasure( CPUProfilerGroup group );
#define PROFILE_START( GROUP ) CPUTimer_StartMeasure( GROUP )
#define PROFILE_END( GROUP ) CPUTimer_EndMeasure( GROUP )
#else
#define PROFILE_START( GROUP )
#define PROFILE_END( GROUP )
#endif

#include "scheduler.h"

/* Console */
void Con_Initialize( void );
void Con_Shutdown( void );
void Con_Toggle( void );
void Con_Draw( const PLGViewport *viewport );

#if !defined( NDEBUG )
#define YIN_ENABLE_LOCAL_FS
#endif

typedef struct OSWindow OSWindow;

#define PrintError( FORMAT, ... )                         \
	PlLogWFunction( LOG_LEVEL_ERROR, FORMAT, ## __VA_ARGS__ ); \
	exit( EXIT_FAILURE )
#define PrintWarn( FORMAT, ... ) PlLogWFunction( LOG_LEVEL_WARN, FORMAT, ## __VA_ARGS__ )
#define Print( FORMAT, ... ) PlLogWFunction( LOG_LEVEL_INFO, FORMAT, ## __VA_ARGS__ )
#if !defined( NDEBUG )
#define DebugMsg( FORMAT, ... ) PlLogWFunction( LOG_LEVEL_INFO, FORMAT, ## __VA_ARGS__ )
#else
#define DebugMsg( ... )
#endif

extern PLPackage *globalWad;

void Engine_Shutdown( void );

OSWindow *Engine_GetMainWindow( void );
unsigned int Engine_GetNumTicks( void );

#include "memory_manager.h"
