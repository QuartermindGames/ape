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
#include "GameInterface.h"

PLPackage *globalWad = NULL;

SystemInterface globalSystem;
GameInterface globalGame;

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
	cpuTimers[ group ].clock = globalSystem.GetPerformanceCounter();
}

void CPUTimer_EndMeasure( CPUProfilerGroup group ) {
	uint64_t now = globalSystem.GetPerformanceCounter();
	cpuTimers[ group ].timeTaken = ( double ) ( ( now - cpuTimers[ group ].clock ) * 1000 ) / globalSystem.GetPerformanceFrequency();
}

double CPUTimer_GetMeasure( CPUProfilerGroup group ) {
	return cpuTimers[ group ].timeTaken;
}

/****************************************
 * INITIALIZATION
 ****************************************/

const char *FS_GetDataDirectory( void ) {
	static char dataPath[ PL_SYSTEM_MAX_PATH ] = { '\0' };
	if ( dataPath[ 0 ] != '\0' ) {
		return dataPath;
	}

	Print( "Checking for \"" YIN_GLOBAL_WAD "\"\n" );
	if ( !plLocalFileExists( YIN_GLOBAL_WAD ) ) {
		snprintf( dataPath, sizeof( dataPath ), "../../" );
	} else {
		snprintf( dataPath, sizeof( dataPath ), "./" );
	}

	return dataPath;
}

int LOG_LEVEL_ERROR, LOG_LEVEL_WARN, LOG_LEVEL_INFO;
static bool Engine_Initialize( int argc, char **argv ) {
	LOG_LEVEL_ERROR = plAddLogLevel( "yin/error", PL_COLOUR_RED, true );
	LOG_LEVEL_WARN = plAddLogLevel( "yin/warning", PL_COLOUR_ORANGE, true );
	LOG_LEVEL_INFO = plAddLogLevel( "yin", PL_COLOUR_WHITE, true );

	Print( "Yin Engine (%s), Copyright (C) 2020 Mark E Sowden\n", ENGINE_VERSION_STR );

	plRegisterStandardPackageLoaders();
	plRegisterPackageLoader( "pkg", Pkg_LoadPackage );
	plRegisterPackageLoader( "map", Pkg_LoadPackage );

	Print( "Mounting VFS locations...\n" );

	plMountLocalLocation( FS_GetDataDirectory() );
	if ( plMountLocation( YIN_GLOBAL_WAD ) == NULL ) {
		PrintError( "Failed to load \"" YIN_GLOBAL_WAD "\"!\nPL: %s\n", plGetError() );
	}

	/* create our main window
	 * todo: this should be delegated to the launcher... */
	mainWindow = globalSystem.CreateWindow( WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT );
	if ( mainWindow == NULL ) {
		PrintError( "Failed to create main window!\n" );
	}

	plInitializeSubSystems( PL_SUBSYSTEM_GRAPHICS );

	/* register other various loaders */
	PLModel *MD2_LoadFile( const char *path );
	plRegisterModelLoader( "md2", MD2_LoadFile );
	PLModel *GSMDL_LoadFile( const char *path );
	plRegisterModelLoader( "mdl", GSMDL_LoadFile );

	Print( "Initializing core services...\n" );

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

	Print( "Initialization complete!\n" );

	return true;
}

void Engine_Shutdown( void ) {
	Print( "Shutting down...\n" );

	Sch_FlushTasks();

	Game_Shutdown();
	Act_Shutdown();
	Gfx_Shutdown();
	Con_Shutdown();
#if defined( DISCORD_INTEGRATION )
	void DiscordIntegration_Shutdown( void );
	DiscordIntegration_Shutdown();
#endif

	globalSystem.Shutdown();
}

SysWindow *Engine_GetMainWindow( void ) {
	return mainWindow;
}

/****************************************
 * DISPLAY
 ****************************************/

static void Engine_Display( void ) {
	/* ensure we don't keep drawing in the background */
	if ( !globalSystem.IsDisplayActive( mainWindow ) ) {
		return;
	}

	PROFILE_START( PROFILE_DRAW_ALL );

	globalSystem.MakeWindowActive( mainWindow );

	Gfx_SetupDefaultState();

	plClearBuffers( PL_BUFFER_DEPTH | PL_BUFFER_COLOUR );

	Editor_Display();
	Game_Display();

	PROFILE_END( PROFILE_DRAW_ALL );

	Gfx_DrawMenu();

	globalSystem.SwapWindow( mainWindow );
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

static SystemInterface *GetSystemInterface( void ) { return &globalSystem; }
static GameInterface *GetGameInterface( void ) { return &globalGame; }

PL_EXPORT EngineInterface *GetDllInterface( uint32_t version, const SystemInterface *sysIn ) {
	if ( version != BASE_INTERFACE_VERSION ) {
		PrintWarn( "Unexpected interface version (%d vs %d)!\n", version, BASE_INTERFACE_VERSION );
		return false;
	}

	/* copy the system interface across */
	globalSystem = *sysIn;

	/* and now setup our engine interface */
	static EngineInterface engineInterface = {
	        .version = BASE_INTERFACE_VERSION,
	        .Initialize = Engine_Initialize,
	        .Shutdown = Engine_Shutdown,
	        .Display = Engine_Display,
	        .GetNumTicks = Engine_GetNumTicks,
	        .IsRunning = Engine_IsRunning,
	        .Tick = Engine_Tick,
	        .KeyboardEvent = Engine_HandleKeyboardEvent,
	        .TextEvent = Engine_HandleTextEvent,
	        .GetSystemInterface = GetSystemInterface,
	        .GetGameInterface = GetGameInterface,
	};

	return &engineInterface;
}
