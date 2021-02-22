/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/platform.h>
#include <PL/platform_console.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_package.h>
#include <PL/pl_graphics.h>
#include <PL/pl_graphics_camera.h>

#include <assert.h>

#include "common/common.h"
#include "shared/interfaces.h"

#define ENGINE_APP_NAME "yin"

#define ENGINE_VERSION_MAJOR 2
#define ENGINE_VERSION_MINOR 0
#define ENGINE_VERSION_PATCH 0

extern const int ENGINE_VERSION[ 3 ];
#define ENGINE_VERSION_STR              \
	PL_TOSTRING( ENGINE_VERSION_MAJOR ) \
	"." PL_TOSTRING( ENGINE_VERSION_MINOR ) "." PL_TOSTRING( ENGINE_VERSION_PATCH )

typedef struct MemRefCnt {
	int numRefs;    /* number of total references */
	double ttl;     /* time to live */
} MemRefCnt;
#define INIT_REFERENCE_COUNTER( A ) memset( A, 0, sizeof( MemRefCnt ) )

inline void MemRefCnt_AddReference( MemRefCnt *v ) { v->numRefs++; }
inline void MemRefCnt_RemoveReference( MemRefCnt *v ) {
	u_assert( v->numRefs > 0 );
	v->numRefs--;
}

enum {
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARN,
	LOG_LEVEL_INFO,
};

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
void Con_ScrollForward( void );
void Con_ScrollBackward( void );
bool Con_GetState( void );
void Con_Draw( const PLViewport *viewport );

//#define DEBUG_CAM
//#define DEBUG_WALL_NORMALS
#define YIN_ENABLE_LOCAL_FS

typedef struct SysWindow SysWindow;

#define WINDOW_TITLE "Yin Technology Demo"
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

#define PrintError( ... )                         \
	plLogMessage( LOG_LEVEL_ERROR, __VA_ARGS__ ); \
	exit( EXIT_FAILURE )
#define PrintWarn( ... ) plLogMessage( LOG_LEVEL_WARN, __VA_ARGS__ )
#define PrintMsg( ... ) plLogMessage( LOG_LEVEL_INFO, __VA_ARGS__ )
#if !defined( NDEBUG )
#   define DebugMsg( ... ) plLogMessage( LOG_LEVEL_INFO, __VA_ARGS__ )
#else
#   define DebugMsg( ... )
#endif

extern PLPackage *globalWad;
#define YIN_GLOBAL_WAD "base.pkg"

void Engine_Shutdown( void );

SysWindow *Engine_GetMainWindow( void );

unsigned int Engine_GetNumTicks( void );

void *Sys_calloc( size_t num, size_t size );
void *Sys_malloc( size_t size );
void *AllocMemory( size_t size, bool abort );

const char *FS_GetDataDirectory( void );
