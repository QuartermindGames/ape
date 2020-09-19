/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <SDL2/SDL.h>

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

/****************************************
 * INITIALIZATION
 ****************************************/

static bool Engine_Initialize( int argc, char **argv ) {
	pl_calloc = g_system.calloc;
	pl_malloc = g_system.malloc;
	pl_realloc = g_system.realloc;

	/* initialize the platform library */
	plInitialize( argc, argv );
	plInitializeSubSystems( PL_SUBSYSTEM_IO );

	plSetupLogOutput( "log.txt" );
	plSetupLogLevel( LOG_LEVEL_ERROR, "error", PL_COLOUR_RED, true );
	plSetupLogLevel( LOG_LEVEL_WARN, "warning", PL_COLOUR_ORANGE, true );
	plSetupLogLevel( LOG_LEVEL_INFO, NULL, PL_COLOUR_WHITE, true );

	PrintMsg( "Yin Engine, Copyright (C) 2020 OldTimes Software\n" );

	plRegisterStandardPackageLoaders();
	plRegisterPackageLoader( "pkg", Pkg_LoadPackage );
	plRegisterPackageLoader( "map", Pkg_LoadPackage );

	PrintMsg( "Mounting VFS locations...\n" );

	/* mount all the dirs and packages we need */

	plMountLocation( plGetWorkingDirectory() );

	/* ensure our base wad is available */
	if( plMountLocation( YIN_GLOBAL_WAD ) == NULL ) {
		PrintError( "Failed to load \"" YIN_GLOBAL_WAD "\"!\nPL: %s\n", plGetError() );
	}

	mainWindow = g_system.CreateWindow( WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT );
	if ( mainWindow == NULL ) {
		PrintError( "Failed to create main window!\n" );
	}

	plInitializeSubSystems( PL_SUBSYSTEM_GRAPHICS );

	/* initialize core services */
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

	Act_Shutdown();
	Gfx_Shutdown();

	g_system.Shutdown();
}

SysWindow *Engine_GetMainWindow( void ) {
	return mainWindow;
}

/****************************************
 * DISPLAY
 ****************************************/

static void Engine_Display( void ) {
	SysWindow *window = Engine_GetMainWindow(); /* g_system.GetMainWindow(); */
	g_system.MakeWindowActive( window );

	Gfx_SetupDefaultState();

	plClearBuffers( PL_BUFFER_DEPTH | PL_BUFFER_COLOUR );

	Editor_Display();
	Game_Display();

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
	//engOut->Keyboard = Engine_Keyboard;

	return true;
}
