/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <SDL2/SDL.h>

#include "yin.h"
#include "renderer/renderer.h"
#include "actor.h"
#include "pkg_loader.h"
#include "editor.h"
#include "game.h"
#include "timespec.h"

PLPackage *globalWad = NULL;

EngineInterface g_engine;
SystemInterface g_system;

static SysWindow *mainWindow;

/****************************************
 * PERFORMANCE
 * Move into performance.c
 ****************************************/

typedef struct CPUTime {
	struct timespec clock;
	double timeTaken;
} CPUTime;
static CPUTime cpuTimers[ MAX_PROFILER_GROUPS ];

void CPUTimer_Initialize( void ) {
	memset( cpuTimers, 0, sizeof( CPUTime ) * MAX_PROFILER_GROUPS );
}

void CPUTimer_StartMeasure( CPUProfilerGroup group ) {
	clock_gettime( CLOCK_MONOTONIC, &cpuTimers[ group ].clock );
}

void CPUTimer_EndMeasure( CPUProfilerGroup group ) {
	struct timespec end;
	clock_gettime( CLOCK_MONOTONIC, &end );

	cpuTimers[ group ].timeTaken = timespec_to_double( timespec_sub( end, cpuTimers[ group ].clock ) );
}

double CPUTimer_GetMeasure( CPUProfilerGroup group ) {
	return cpuTimers[ group ].timeTaken;
}

/****************************************
 * MEMORY MANAGEMENT
 ****************************************/

void *Sys_calloc( size_t num, size_t size ) {
	void *mem = calloc( num, size );
	if( mem == NULL ) {
		PrintError( "Failed to allocate %d bytes!\n", num * size );
	}

	return mem;
}

/* wrapper for malloc */
void *Sys_malloc( size_t size ) {
	return Sys_calloc( 1, size );
}

/* wrapper for realloc */
void *Sys_realloc( void *ptr, size_t newSize ) {
	void *buf = realloc( ptr, newSize );
	if ( buf == NULL ) {
		PrintError( "Failed to allocate %lu bytes!\n", newSize );
	}

	return buf;
}

/****************************************
 * INITIALIZATION
 ****************************************/

static void Sys_SetupVFS( void ) {
	PrintMsg( "Mounting VFS locations...\n" );

	/* check whether or not we're launching from the 'runtime' dir */
	PrintMsg( "Checking for \"" YIN_GLOBAL_WAD "\"\n" );
	if ( !plLocalFileExists( YIN_GLOBAL_WAD ) ) {
		PrintMsg( "Did not find \"" YIN_GLOBAL_WAD "\", attempting to mount data directory...\n" );
		plMountLocalLocation( "../../" );
	}

	if( plMountLocation( YIN_GLOBAL_WAD ) == NULL ) {
		PrintError( "Failed to load \"" YIN_GLOBAL_WAD "\"!\nPL: %s\n", plGetError() );
	}
}

static bool Engine_Initialize( int argc, char **argv ) {
	pl_calloc = Sys_calloc;
	pl_malloc = Sys_malloc;
	pl_realloc = Sys_realloc;

	/* initialize the platform library */
	plInitialize( argc, argv );
	plInitializeSubSystems( PL_SUBSYSTEM_IO );

	if ( plHasCommandLineArgument( "-log" ) ) {
		const char *path = plGetCommandLineArgumentValue( "-log" );
		if ( path == NULL ) {
			path = "log.txt";
		}

		plSetupLogOutput( path );
	}

	plSetupLogLevel( LOG_LEVEL_ERROR, "error", PL_COLOUR_RED, true );
	plSetupLogLevel( LOG_LEVEL_WARN, "warning", PL_COLOUR_ORANGE, true );
	plSetupLogLevel( LOG_LEVEL_INFO, NULL, PL_COLOUR_WHITE, true );

	PrintMsg( "Yin Engine, Copyright (C) 2020 Mark E Sowden\n" );

	plRegisterStandardPackageLoaders();
	plRegisterPackageLoader( "pkg", Pkg_LoadPackage );
	plRegisterPackageLoader( "map", Pkg_LoadPackage );

	Sys_SetupVFS();

	/* create our main window
	 * todo: this should be delegated to the launcher... */
	mainWindow = g_system.CreateWindow( WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT );
	if ( mainWindow == NULL ) {
		PrintError( "Failed to create main window!\n" );
	}

	plInitializeSubSystems( PL_SUBSYSTEM_GRAPHICS );

	/* initialize core services */
	CPUTimer_Initialize();
	Con_Initialize();
	Gfx_Initialize();
	Act_Initialize();

	Game_Initialize();
	if( plHasCommandLineArgument( "editor" ) ) {
		Editor_Initialize();
	}

	PrintMsg( "Initialization complete\n" );

	return true;
}

static void Engine_Shutdown( void ) {
	PrintMsg( "Shutting down...\n" );

	Sch_FlushTasks();

	Act_Shutdown();
	Gfx_Shutdown();
	Con_Shutdown();

	g_system.Shutdown();
}

SysWindow *Engine_GetMainWindow( void ) {
	return mainWindow;
}

/****************************************
 * DISPLAY
 ****************************************/

static void Engine_Display( void ) {
	/* ensure we don't keep drawing in the background */
    if ( !g_system.IsDisplayActive( mainWindow ) ) {
        return;
    }

	memset( &g_gfxPerfStats, 0, sizeof( g_gfxPerfStats ) );

	PROFILE_START( PROFILE_DRAW_ALL );

	g_system.MakeWindowActive( mainWindow );

	Gfx_SetupDefaultState();

	plClearBuffers( PL_BUFFER_DEPTH | PL_BUFFER_COLOUR );

	Editor_Display();
	Game_Display();

	PROFILE_END( PROFILE_DRAW_ALL );

	Gfx_DrawMenu();

	g_system.SwapWindow( mainWindow );
}

/****************************************
 * TIMER MANAGEMENT
 ****************************************/

static unsigned int numTicks = 0;

unsigned int Engine_GetNumTicks( void ) {
	return numTicks;
}

static void Engine_Tick( void ) {
	if ( g_system.GetKeyState( '`' ) || g_system.GetKeyState( '~' ) ) {
		Con_Toggle();
	}

	/* temp */
	if ( g_system.GetKeyState( 'p' ) ) Con_ScrollForward();
	if ( g_system.GetKeyState( 'l' ) ) Con_ScrollBackward();

	Sch_RunTasks();

	Editor_Tick();
	Game_Tick();

	numTicks++;
}

static bool Engine_IsRunning( void ) {
	/* always running */
	return true;
}

/****************************************
 * INTERFACE
 ****************************************/

bool Con_HandleKeyboardEvent( int key, bool isDown );
static void Engine_HandleKeyboardEvent( int key, bool isDown ) {
    if ( Con_HandleKeyboardEvent( key, isDown ) ) {
		return;
	}
}

PL_EXPORT bool GetDllInterface( uint32_t version, const SystemInterface *sysIn, EngineInterface *engOut ) {
	if ( version != BASE_INTERFACE_VERSION ) {
		PrintWarn( "Unexpected interface version (%d vs %d)!\n", version, BASE_INTERFACE_VERSION );
		return false;
	}

	/* copy the system interface across */
	g_system = *sysIn;

	/* and now setup our engine interface */
	engOut->Initialize = Engine_Initialize;
	engOut->Shutdown = Engine_Shutdown;
	engOut->Display = Engine_Display;
	engOut->GetNumTicks = Engine_GetNumTicks;
	engOut->IsRunning = Engine_IsRunning;
	engOut->Tick = Engine_Tick;
	engOut->KeyboardEvent = Engine_HandleKeyboardEvent;

	return true;
}
