/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/platform_model.h>

#include "yin.h"
#include "renderer/renderer.h"
#include "actor.h"
#include "pkg_loader.h"
#include "editor.h"
#include "game.h"

PLPackage *globalWad = NULL;

EngineInterface g_engine;
SystemInterface g_system;

static SysWindow *mainWindow;

const int ENGINE_VERSION[ 3 ] = { ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH };

/****************************************
 * PERFORMANCE
 * Move into performance.c
 ****************************************/

typedef struct CPUTime {
	uint64_t clock;
	double timeTaken;
} CPUTime;
static CPUTime cpuTimers[ MAX_PROFILER_GROUPS ];

void CPUTimer_Initialize( void ) {
	memset( cpuTimers, 0, sizeof( CPUTime ) * MAX_PROFILER_GROUPS );
}

void CPUTimer_StartMeasure( CPUProfilerGroup group ) {
	cpuTimers[ group ].clock = g_system.GetPerformanceCounter();
}

void CPUTimer_EndMeasure( CPUProfilerGroup group ) {
	uint64_t now = g_system.GetPerformanceCounter();
	cpuTimers[ group ].timeTaken = ( double ) ( ( now - cpuTimers[ group ].clock ) * 1000 ) / g_system.GetPerformanceFrequency();
}

double CPUTimer_GetMeasure( CPUProfilerGroup group ) {
	return cpuTimers[ group ].timeTaken;
}

/****************************************
 * MEMORY MANAGEMENT
 ****************************************/

void *Sys_calloc( size_t num, size_t size ) {
	void *mem = calloc( num, size );
	if ( mem == NULL ) {
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

/**
 * Allocate a pool of zeroed memory.
 * Aborts on error if abort is true.
 */
void *AllocMemory( size_t size, bool abort ) {
	void *buf = calloc( 1, size );
	if ( buf == NULL && abort ) {
		PrintError( "Failed to allocate %du bytes!\n", size );
	}

	/* otherwise, it's the callers problem */
	return buf;
}

/****************************************
 * INITIALIZATION
 ****************************************/

const char *FS_GetDataDirectory( void ) {
	static char dataPath[ PL_SYSTEM_MAX_PATH ] = { '\0' };
	if ( dataPath[ 0 ] != '\0' ) {
		return dataPath;
	}

	PrintMsg( "Checking for \"" YIN_GLOBAL_WAD "\"\n" );
	if ( !plLocalFileExists( YIN_GLOBAL_WAD ) ) {
		snprintf( dataPath, sizeof( dataPath ), "../../" );
	} else {
		snprintf( dataPath, sizeof( dataPath ), "./" );
	}

	return dataPath;
}

int LOG_LEVEL_ERROR, LOG_LEVEL_WARN, LOG_LEVEL_INFO;
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

	LOG_LEVEL_ERROR = plAddLogLevel( "yin/error", PL_COLOUR_RED, true );
	LOG_LEVEL_WARN = plAddLogLevel( "yin/warning", PL_COLOUR_ORANGE, true );
	LOG_LEVEL_INFO = plAddLogLevel( "yin", PL_COLOUR_WHITE, true );

	PrintMsg( "Yin Engine (%s), Copyright (C) 2020 Mark E Sowden\n", ENGINE_VERSION_STR );

	plRegisterStandardPackageLoaders();
	plRegisterPackageLoader( "pkg", Pkg_LoadPackage );
	plRegisterPackageLoader( "map", Pkg_LoadPackage );

	CommonLibrary_Initialize();

	PrintMsg( "Mounting VFS locations...\n" );

	plMountLocalLocation( FS_GetDataDirectory() );
	if ( plMountLocation( YIN_GLOBAL_WAD ) == NULL ) {
		PrintError( "Failed to load \"" YIN_GLOBAL_WAD "\"!\nPL: %s\n", plGetError() );
	}

	/* create our main window
	 * todo: this should be delegated to the launcher... */
	mainWindow = g_system.CreateWindow( WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT );
	if ( mainWindow == NULL ) {
		PrintError( "Failed to create main window!\n" );
	}

	plInitializeSubSystems( PL_SUBSYSTEM_GRAPHICS );

	/* register other various loaders */
	PLModel *MD2_LoadFile( const char *path );
	plRegisterModelLoader( "md2", MD2_LoadFile );
	PLModel *GSMDL_LoadFile( const char *path );
	plRegisterModelLoader( "mdl", GSMDL_LoadFile );

	/* initialize core services */
	CPUTimer_Initialize();
	Con_Initialize();
	Gfx_Initialize();
	Act_Initialize();

	Game_Initialize();
	if ( plHasCommandLineArgument( "editor" ) ) {
		Editor_Initialize();
	}

#if defined( DISCORD_INTEGRATION )
	void DiscordIntegration_Initialize( void );
	DiscordIntegration_Initialize();
#endif

	Con_Toggle();

	PrintMsg( "Initialization complete - waiting for input\n" );

	return true;
}

void Engine_Shutdown( void ) {
	PrintMsg( "Shutting down...\n" );

	Sch_FlushTasks();

	Act_Shutdown();
	Gfx_Shutdown();
	Con_Shutdown();
#if defined( DISCORD_INTEGRATION )
	void DiscordIntegration_Shutdown( void );
	DiscordIntegration_Shutdown();
#endif

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
#if defined( DISCORD_INTEGRATION )
	void DiscordIntegration_Tick( void );
	DiscordIntegration_Tick();
#endif

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

bool Con_HandleKeyboardEvent( int key, unsigned int keyState );
static void Engine_HandleKeyboardEvent( int key, unsigned int keyState ) {
	if ( Con_HandleKeyboardEvent( key, keyState ) ) {
		return;
	}
}

bool Con_HandleTextEvent( const char *key );
static void Engine_HandleTextEvent( const char *key ) {
	if ( Con_HandleTextEvent( key ) ) {
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
	engOut->TextEvent = Engine_HandleTextEvent;

	return true;
}
