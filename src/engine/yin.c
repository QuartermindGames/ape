/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plmodel/plm.h>
#include <plgraphics/plg_driver_interface.h>

#include "yin.h"
#include "renderer/renderer.h"
#include "audio.h"
#include "actor.h"
#include "pkg_loader.h"
#include "editor.h"
#include "game_interface.h"

PLPackage *globalWad = NULL;

OSInterface   globalSystem;
GameInterface globalGame;

static OSWindow *mainWindow;

const int ENGINE_VERSION[ 3 ] = { ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH };

/****************************************
 * PERFORMANCE
 * Move into performance.c
 ****************************************/

typedef struct CPUTime
{
	uint64_t clock;
	double   timeTaken;
} CPUTime;
static CPUTime cpuTimers[ MAX_PROFILER_GROUPS ];

void CPUTimer_Initialize( void )
{
	Print( "Initializing timer\n" );

	memset( cpuTimers, 0, sizeof( CPUTime ) * MAX_PROFILER_GROUPS );
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

/****************************************
 * INITIALIZATION
 ****************************************/

int         LOG_LEVEL_ERROR, LOG_LEVEL_WARN, LOG_LEVEL_INFO;
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
	{
		PrintError( "Failed to load \"" YIN_GLOBAL_WAD "\"!\nPL: %s\n", PlGetError() );
	}

	/* register other various loaders */
	PLMModel *MD2_LoadFile( const char *path );
	PlmRegisterModelLoader( "md2", MD2_LoadFile );
	PLMModel *GSMDL_LoadFile( const char *path );
	PlmRegisterModelLoader( "mdl", GSMDL_LoadFile );

	/* create our main window
 * todo: this should be delegated to the launcher... */
	mainWindow = globalSystem.CreateWindow( "", 0, 0 );
	if ( mainWindow == NULL )
	{
		PrintError( "Failed to create main window!\n" );
	}

	Print( "Initializing core services...\n" );

	/* initialize core services */
	CPUTimer_Initialize();
	Con_Initialize();
	Sch_Initialize();
	Mem_Initialize();
	R_Initialize();
	A_Initialize();
	Game_Initialize();
	Act_Initialize();

	if ( PlHasCommandLineArgument( "editor" ) )
	{
		Editor_Initialize();
	}

	Print( "Initialization complete!\n" );

	return true;
}

void Engine_Shutdown( void )
{
	Print( "Shutting down...\n" );

	Sch_FlushTasks();

	Game_Shutdown();
	Act_Shutdown();
	A_Shutdown();
	R_Shutdown();
	Con_Shutdown();
	Mem_Shutdown();

	globalSystem.Shutdown();
}

OSWindow *Engine_GetMainWindow( void )
{
	return mainWindow;
}

/****************************************
 * DISPLAY
 ****************************************/

static void Engine_Display( void )
{
	PROFILE_START( PROFILE_DRAW_ALL );

	Gfx_SetupDefaultState();

	PlgClearBuffers( PLG_BUFFER_DEPTH | PLG_BUFFER_COLOUR );

	Editor_Display();
	Game_Display();

	PROFILE_END( PROFILE_DRAW_ALL );

	Gfx_DrawMenu();
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

	Editor_Tick();
	Game_Tick();

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

bool        Con_HandleKeyboardEvent( int key, unsigned int keyState );
static void Engine_HandleKeyboardEvent( int key, unsigned int keyState )
{
	if ( Con_HandleKeyboardEvent( key, keyState ) )
	{
		return;
	}
}

bool        Con_HandleTextEvent( const char *key );
static void Engine_HandleTextEvent( const char *key )
{
	if ( Con_HandleTextEvent( key ) )
	{
		return;
	}
}

static OSInterface *  GetSystemInterface( void ) { return &globalSystem; }
static GameInterface *GetGameInterface( void ) { return &globalGame; }

PL_EXPORT EngineInterface *GetDllInterface( uint32_t version, const OSInterface *sysIn )
{
	if ( version != ENGINE_INTERFACE_VERSION )
	{
		PrintWarn( "Unexpected interface version (%d vs %d)!\n", version, ENGINE_INTERFACE_VERSION );
	}

	/* copy the system interface across */
	globalSystem = *sysIn;

	/* and now setup our engine interface */
	static EngineInterface engineInterface = {
	        .version          = { ENGINE_INTERFACE_VERSION_MAJOR, ENGINE_INTERFACE_VERSION_MINOR },
	        .Initialize       = Engine_Initialize,
	        .Shutdown         = Engine_Shutdown,
	        .Display          = Engine_Display,
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
