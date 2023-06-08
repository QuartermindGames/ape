// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_console.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>
#include <plcore/pl_linkedlist.h>
#include <plcore/pl_array_vector.h>
#include <plcore/pl_timer.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>
#include <plgraphics/plg_polygon.h>

#include <assert.h>

#include "common.h"

#include <yin/core.h>

#define ENGINE_NAME        "APE"
#define ENGINE_APP_NAME    "ape"
#define ENGINE_BASE_CONFIG "engine.cfg.n"

#define VERSION_MAJOR    3
#define VERSION_MINOR    0
#define VERSION_PATCH    0
#define VERSION_CODENAME "Amber"

PL_EXTERN_C

#define ENGINE_VERSION_STR       \
	PL_TOSTRING( VERSION_MAJOR ) \
	"." PL_TOSTRING( VERSION_MINOR ) "." PL_TOSTRING( VERSION_PATCH ) " (" VERSION_CODENAME ")"

#if !defined( NDEBUG )
#	define ENABLE_PROFILER 1
#endif

typedef enum ApeProfilerGroup
{
	PROFILE_SIM_ALL,

	PROFILE_TICK_CLIENT,
	PROFILE_TICK_SERVER,

	PROFILE_DRAW_ALL,
	PROFILE_DRAW_WORLD,
	PROFILE_DRAW_ACTORS,
	PROFILE_DRAW_UI,
	PROFILE_DRAW_GUI,

	MAX_PROFILER_GROUPS
} ApeProfilerGroup;
extern const char *cpuProfilerDescriptions[ MAX_PROFILER_GROUPS ];
#if defined( ENABLE_PROFILER )
void apeInitializeProfiler( void );
void apeEndProfilerFrame( void );
void Profiler_StartMeasure( ApeProfilerGroup group );
void Profiler_EndMeasure( ApeProfilerGroup group );
double apeGetProfilerMeasure( ApeProfilerGroup group );
#	define APE_PROFILE_START( GROUP ) Profiler_StartMeasure( GROUP )
#	define APE_PROFILE_END( GROUP )   Profiler_EndMeasure( GROUP )
#else
#	define Profiler_Initialize()
#	define Profiler_StartMeasure()
#	define Profiler_EndMeasure()
#	define Profiler_GetMeasure() 0
#	define APE_PROFILE_START( GROUP )
#	define APE_PROFILE_END( GROUP )
#endif
void apeUpdateProfilerGraphs( void );
const double *apeGetProfilerGraph( ApeProfilerGroup group, uint8_t *numPoints );
double apeGetProfilerGraphValue( ApeProfilerGroup group );
double apeGetProfilerGraphAverage( ApeProfilerGroup group );

#include "core_scheduler.h"
#include "core_memory_manager.h"

/****************************************
 * CONSOLE
 ****************************************/

typedef enum ApeConsoleLogLevel
{
	APE_LOG_ERROR,
	APE_LOG_WARNING,
	APE_LOG_INFORMATION,

	APE_LOG_CLIENT_ERROR,
	APE_LOG_CLIENT_WARNING,
	APE_LOG_CLIENT_INFORMATION,

	APE_LOG_SERVER_ERROR,
	APE_LOG_SERVER_WARNING,
	APE_LOG_SERVER_INFORMATION,

	APE_LOG_LEVELS
} ApeConsoleLogLevel;

#define CONSOLE_BUFFER_MAX_LENGTH 256
#define CONSOLE_BUFFER_MAX_LINES  2048

typedef struct ApeConsoleLine
{
	char buffer[ CONSOLE_BUFFER_MAX_LENGTH ];
	PLColour colour;
} ApeConsoleLine;

typedef struct ApeConsoleOutput
{
	ApeConsoleLine lines[ CONSOLE_BUFFER_MAX_LINES ];
	unsigned int numLines;
	unsigned int scrollPos;
} ApeConsoleOutput;

ApeConsoleOutput *apeGetConsoleOutput( void );

void apeInitializeConsole( void );
void apeShutdownConsole( void );

int Console_GetLogLevel( ApeConsoleLogLevel level );
void Console_Print( ApeConsoleLogLevel level, const char *message, ... );

void apeRegisterConsoleCommands_( bool isDedicated );
void apeRegisterConsoleVariables_( bool isDedicated );

void apeDrawConsole_( const ApeViewport *viewport );
void apeRegisterClientConsoleCommands_( void );
void apeRegisterClientConsoleVariables_( void );

#define PRINT( FORMAT, ... ) \
	Console_Print( APE_LOG_INFORMATION, FORMAT, ##__VA_ARGS__ )
#define PRINT_WARNING( FORMAT, ... ) \
	Console_Print( APE_LOG_WARNING, "WARNING: " FORMAT, ##__VA_ARGS__ )
#define PRINT_ERROR( FORMAT, ... )                                                   \
	{                                                                                \
		PlLogMessage( Console_GetLogLevel( APE_LOG_ERROR ), FORMAT, ##__VA_ARGS__ ); \
		abort();                                                                     \
	}

#if !defined( NDEBUG )
#	define PRINT_DEBUG( FORMAT, ... ) PlLogWFunction( Console_GetLogLevel( APE_LOG_INFORMATION ), FORMAT, ##__VA_ARGS__ )
#else
#	define PRINT_DEBUG( FORMAT, ... )
#endif

PL_EXTERN_C_END
