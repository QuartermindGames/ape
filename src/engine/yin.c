/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plmodel/plm.h>
#include <plgraphics/plg_driver_interface.h>

#include "yin.h"
#include "actor.h"
#include "pkg_loader.h"
#include "game_interface.h"

#include "client/client.h"

PLPackage *globalWad = NULL;

OSSystemInterface globalSystem;
GameInterface     globalGame;

const int ENGINE_VERSION[ 3 ] = { ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH };

/****************************************
 * PERFORMANCE
 * Move into performance.c
 ****************************************/

#define NUM_GRAPH_POINTS 32
static float cpuPerfGraphs[ MAX_PROFILER_GROUPS ][ NUM_GRAPH_POINTS ];

typedef struct CPUTime
{
	uint64_t clock;
	double   timeTaken;
} CPUTime;
static CPUTime cpuTimers[ MAX_PROFILER_GROUPS ];

const char *cpuProfilerDescriptions[ MAX_PROFILER_GROUPS ] = {
        PL_TOSTRING( PROFILE_DRAW_ALL ),
        PL_TOSTRING( PROFILE_DRAW_WORLD ),
        PL_TOSTRING( PROFILE_DRAW_ACTORS ),
        PL_TOSTRING( PROFILE_DRAW_UI ),
};

void CPUTimer_Initialize( void )
{
	Print( "Initializing profiler\n" );

	memset( cpuTimers, 0, sizeof( CPUTime ) * MAX_PROFILER_GROUPS );
	memset( cpuPerfGraphs, 0, sizeof( float ) * MAX_PROFILER_GROUPS * NUM_GRAPH_POINTS );
}

void CPUTimer_StartMeasure( CPUProfilerGroup group )
{
	cpuTimers[ group ].clock = globalSystem.GetPerformanceCounter();
}

void CPUTimer_EndMeasure( CPUProfilerGroup group )
{
	uint64_t now                 = globalSystem.GetPerformanceCounter();
	cpuTimers[ group ].timeTaken = ( double ) ( ( now - cpuTimers[ group ].clock ) * 1000 ) / globalSystem.GetPerformanceFrequency();
}

double CPUTimer_GetMeasure( CPUProfilerGroup group )
{
	return cpuTimers[ group ].timeTaken;
}

void PF_UpdateGraphs( void )
{
	static unsigned int refreshTime = 0;
	if ( refreshTime > Engine_GetNumTicks() )
		return;

	for ( uint8_t i = 0; i < MAX_PROFILER_GROUPS; ++i )
	{
		// Shuffle the list along
		for ( uint8_t j = 0; j < NUM_GRAPH_POINTS - 1; ++j )
			cpuPerfGraphs[ i ][ j ] = cpuPerfGraphs[ i ][ j + 1 ];

		cpuPerfGraphs[ i ][ NUM_GRAPH_POINTS - 1 ] = ( float ) CPUTimer_GetMeasure( i );
	}

	CVar( "debug.profilerFrequency", profilerFrequency );
	refreshTime += ( profilerFrequency != NULL ) ? profilerFrequency->i_value : 16;
}

const float *PF_GetGraph( CPUProfilerGroup group, uint8_t *numPoints )
{
	*numPoints = NUM_GRAPH_POINTS;
	return cpuPerfGraphs[ group ];
}

/****************************************
 * INITIALIZATION
 ****************************************/

#define MAX_GAME_PACKAGES 255
static PLFileSystemMount *gamePackages[ MAX_GAME_PACKAGES ];

int LOG_LEVEL_ERROR, LOG_LEVEL_WARN, LOG_LEVEL_INFO;

static bool Engine_Initialize( int argc, char **argv )
{
	LOG_LEVEL_ERROR = PlAddLogLevel( "yin/error", PL_COLOUR_RED, true );
	LOG_LEVEL_WARN  = PlAddLogLevel( "yin/warning", PL_COLOUR_ORANGE, true );
	LOG_LEVEL_INFO  = PlAddLogLevel( "yin", PL_COLOUR_WHITE, true );

	Print( "Yin Engine (%s), Copyright (C) 2020-2021 Mark E Sowden\n", ENGINE_VERSION_STR );

	PlRegisterStandardPackageLoaders();
	PlRegisterPackageLoader( "pkg", Pkg_LoadPackage );

	Print( "Registering plugins...\n" );

	PlRegisterPlugins( "./" );
	PlInitializePlugins();

	/* todo: move into launcher */
	PlgInitializeGraphics();
	PlgScanForDrivers( "./" );

	Print( "Mounting VFS locations...\n" );

	PlMountLocalLocation( ComFS_GetDataDirectory() );
	if ( PlMountLocation( YIN_GLOBAL_WAD ) == NULL )
		PrintError( "Failed to load \"" YIN_GLOBAL_WAD "\"!\nPL: %s\n", PlGetError() );

	for ( uint8_t i = 0; i < MAX_GAME_PACKAGES; ++i )
	{
		char gamePkg[ PL_SYSTEM_MAX_PATH ];
		snprintf( gamePkg, sizeof( gamePkg ), "game%d.pkg", i );
		if ( ( gamePackages[ i ] = PlMountLocation( gamePkg ) ) != NULL )
			continue;

		break;
	}

	/* register other various loaders */
	PLMModel *MD2_LoadFile( const char *path );
	PlmRegisterModelLoader( "md2", MD2_LoadFile );
	PLMModel *GSMDL_LoadFile( const char *path );
	PlmRegisterModelLoader( "mdl", GSMDL_LoadFile );

	Print( "Initializing core services...\n" );

	/* initialize core services */
	CPUTimer_Initialize();
	Con_Initialize();
	Sch_Initialize();
	Mem_Initialize();
	CL_Initialize();
	Game_Initialize();
	Act_Initialize();

	Print( "Initialization complete!\n" );

	return true;
}

void Engine_Shutdown( void )
{
	Print( "Shutting down...\n" );

	Sch_FlushTasks();

	Game_Shutdown();
	Act_Shutdown();
	CL_Shutdown();
	Con_Shutdown();
	Mem_Shutdown();

	globalSystem.Shutdown();
}

/****************************************
 * TIMER MANAGEMENT
 ****************************************/

static unsigned int numTicks = 0;

unsigned int Engine_GetNumTicks( void )
{
	return numTicks;
}

static void Engine_Tick( void )
{
#if defined( DISCORD_INTEGRATION )
	void DiscordIntegration_Tick( void );
	DiscordIntegration_Tick();
#endif

	Sch_RunTasks();

	/* todo: split between CL and SV */
	Game_Tick();

	PF_UpdateGraphs();

	numTicks++;
}

static bool Engine_IsRunning( void )
{
	/* always running */
	return true;
}

/****************************************
 * INTERFACE
 ****************************************/

bool Con_HandleKeyboardEvent( int key, unsigned int keyState );

static void Engine_HandleKeyboardEvent( int key, unsigned int keyState )
{
	if ( Con_HandleKeyboardEvent( key, keyState ) )
		return;
}

bool Con_HandleTextEvent( const char *key );

static void Engine_HandleTextEvent( const char *key )
{
	if ( Con_HandleTextEvent( key ) )
		return;
}

static OSSystemInterface *GetSystemInterface( void ) { return &globalSystem; }
static GameInterface *    GetGameInterface( void ) { return &globalGame; }

PL_EXPORT OSEngineInterface *GetDllInterface( uint32_t version, const OSSystemInterface *sysIn )
{
	if ( version != ENGINE_INTERFACE_VERSION )
		PrintWarn( "Unexpected interface version (%d vs %d)!\n", version, ENGINE_INTERFACE_VERSION );

	/* copy the system interface across */
	globalSystem = *sysIn;

	/* and now setup our engine interface */
	static OSEngineInterface engineInterface = {
	        .version          = { ENGINE_INTERFACE_VERSION_MAJOR, ENGINE_INTERFACE_VERSION_MINOR },
	        .Initialize       = Engine_Initialize,
	        .Shutdown         = Engine_Shutdown,
	        .Display          = CL_Display,
	        .GetNumTicks      = Engine_GetNumTicks,
	        .IsRunning        = Engine_IsRunning,
	        .Tick             = Engine_Tick,
	        .KeyboardEvent    = Engine_HandleKeyboardEvent,
	        .TextEvent        = Engine_HandleTextEvent,
	        .GetOSInterface   = GetSystemInterface,
	        .GetGameInterface = GetGameInterface,
	};

	return &engineInterface;
}
