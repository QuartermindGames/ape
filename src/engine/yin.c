/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <SDL2/SDL.h>

#include "yin.h"
#include "gfx.h"
#include "act.h"
#include "pkg_loader.h"

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

	/* initialize the platform library */
	plInitialize( argc, argv );
	plInitializeSubSystems( PL_SUBSYSTEM_IO | PL_SUBSYSTEM_IMAGE );

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
	const char *rPackages[]={
		"BaseShaders.pkg",
		"BaseTextures.pkg",
	};
	for ( unsigned int i = 0; i < plArrayElements( rPackages ); ++i ) {
		if ( plMountLocation( rPackages[ i ] ) == NULL ) {
			PrintError( "Failed to mount required package \"%s\"!\nPL: %s\n", rPackages[ i ], plGetError() );
		}
	}

	/* ensure our wad is available */
	globalWad = plLoadPackage( YIN_GLOBAL_WAD );
	if( globalWad == NULL ) {
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

	if( plHasCommandLineArgument( "editor" ) ) {}
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
 * TIMER MANAGEMENT
 ****************************************/

static unsigned int numTicks = 0;

unsigned int Engine_GetNumTicks( void ) {
	return numTicks;
}

static void Engine_Tick( void ) {
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
	engOut->Shutdown = Engine_Shutdown;
	//engOut->Display = Engine_Display;
	engOut->GetNumTicks = Engine_GetNumTicks;
	engOut->IsRunning = Engine_IsRunning;
	engOut->Tick = Engine_Tick;
	//engOut->Keyboard = Engine_Keyboard;

	return true;
}
