// SPDX-License-Identifier: LGPL-3.0-or-later
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

#define ENGINE_NAME        "Orcus"
#define ENGINE_APP_NAME    "orcus"
#define ENGINE_BASE_CONFIG "engine.cfg.n"

#define VERSION_MAJOR    3
#define VERSION_MINOR    0
#define VERSION_PATCH    0
#define VERSION_CODENAME "LUNA"

PL_EXTERN_C

#define ENGINE_VERSION_STR       \
	PL_TOSTRING( VERSION_MAJOR ) \
	"." PL_TOSTRING( VERSION_MINOR ) "." PL_TOSTRING( VERSION_PATCH ) " (" VERSION_CODENAME ")"

#if !defined( NDEBUG )
#	define ENABLE_PROFILER 1
#endif

typedef enum CPUProfilerGroup
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
} ProfilerGroup;
extern const char *cpuProfilerDescriptions[ MAX_PROFILER_GROUPS ];
#if defined( ENABLE_PROFILER )
void ogeInitializeProfiler( void );
void ogeProfiler_EndFrame( void );
void Profiler_StartMeasure( ProfilerGroup group );
void Profiler_EndMeasure( ProfilerGroup group );
double ogeProfiler_GetMeasure( ProfilerGroup group );
#	define OGE_PROFILE_START( GROUP ) Profiler_StartMeasure( GROUP )
#	define OGE_PROFILE_END( GROUP )   Profiler_EndMeasure( GROUP )
#else
#	define Profiler_Initialize()
#	define Profiler_StartMeasure()
#	define Profiler_EndMeasure()
#	define Profiler_GetMeasure() 0
#	define PROFILE_START( GROUP )
#	define PROFILE_END( GROUP )
#endif
void ogeProfiler_UpdateGraphs( void );
const double *ogeProfiler_GetGraph( ProfilerGroup group, uint8_t *numPoints );
double Profiler_GetGraphValue( ProfilerGroup group );
double Profiler_GetGraphAverage( ProfilerGroup group );

#include "core_scheduler.h"
#include "core_memory_manager.h"

/****************************************
 * CONSOLE
 ****************************************/

typedef enum ConsoleLogLevel
{
	YINENGINE_LOG_ERROR,
	YINENGINE_LOG_WARNING,
	YINENGINE_LOG_INFORMATION,

	YINENGINE_LOG_CLIENT_ERROR,
	YINENGINE_LOG_CLIENT_WARNING,
	YINENGINE_LOG_CLIENT_INFORMATION,

	YINENGINE_LOG_SERVER_ERROR,
	YINENGINE_LOG_SERVER_WARNING,
	YINENGINE_LOG_SERVER_INFORMATION,

	YINENGINE_LOG_LEVELS
} ConsoleLogLevel;

#define CONSOLE_BUFFER_MAX_LENGTH 256
#define CONSOLE_BUFFER_MAX_LINES  2048

typedef struct ConsoleLine
{
	char buffer[ CONSOLE_BUFFER_MAX_LENGTH ];
	PLColour colour;
} ConsoleLine;

typedef struct ConsoleOutput
{
	ConsoleLine lines[ CONSOLE_BUFFER_MAX_LINES ];
	unsigned int numLines;
	unsigned int scrollPos;
} ConsoleOutput;

ConsoleOutput *Console_GetOutput( void );

void ogeInitializeConsole( void );
void ogeShutdownConsole( void );

int Console_GetLogLevel( ConsoleLogLevel level );
void Console_Print( ConsoleLogLevel level, const char *message, ... );

void ogeRegisterConsoleCommands_( bool isDedicated );
void ogeRegisterConsoleVariables( bool isDedicated );

void ogeDrawConsole_( const OgeViewport *viewport );
bool ogeIsConsoleOpen( void );
void Client_Console_RegisterConsoleCommands( void );
void ogeRegisterClientConsoleVariables_( void );

#define PRINT( FORMAT, ... ) \
	Console_Print( YINENGINE_LOG_INFORMATION, FORMAT, ##__VA_ARGS__ )
#define PRINT_WARNING( FORMAT, ... ) \
	Console_Print( YINENGINE_LOG_WARNING, "WARNING: " FORMAT, ##__VA_ARGS__ )
#define PRINT_ERROR( FORMAT, ... )                                                         \
	{                                                                                      \
		PlLogMessage( Console_GetLogLevel( YINENGINE_LOG_ERROR ), FORMAT, ##__VA_ARGS__ ); \
		abort();                                                                           \
	}

#if !defined( NDEBUG )
#	define PRINT_DEBUG( FORMAT, ... ) PlLogWFunction( Console_GetLogLevel( YINENGINE_LOG_INFORMATION ), FORMAT, ##__VA_ARGS__ )
#else
#	define PRINT_DEBUG( FORMAT, ... )
#endif

PL_EXTERN_C_END
