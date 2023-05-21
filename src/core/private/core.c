// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plmodel/plm.h>

#include "core_private.h"

#include <yin/core_game.h>

#include "core_filesystem.h"
#include "core_model.h"

#include "client/client.h"
#include "client/client_input.h"
#include "editor/editor.h"

#include "server/server.h"
#include "net/net.h"

#include <yin/node.h>

/****************************************
 * PRIVATE
 ****************************************/

static unsigned int numTicks = 0;

static NdBranch *engineConfig;
static NdBranch *userConfig;

static bool engineTerminalMode = false;
static bool engineInitialized  = false;

/****************************************
 * PUBLIC
 ****************************************/

NdBranch *ogeGetConfig( void ) { return engineConfig; }
NdBranch *ogeGetUserConfig( void ) { return userConfig; }

bool ogeInitialize( const char *config )
{
	// Call this first, so we can buffer console output
	ogeInitializeConsole();

	PRINT( "Yin %d (%s / (%s:%s, %s)), Copyright (C) 2020-2023 Mark E Sowden\n",
	       VERSION_MAJOR,
	       ENGINE_VERSION_STR,
	       GIT_BRANCH, GIT_COMMIT_COUNT, GIT_COMMIT_HASH );
	PRINT( "Current working directory: \"%s\"\n", PlGetWorkingDirectory() );

	engineTerminalMode = PlHasCommandLineArgument( "cmd" );
	if ( engineTerminalMode )
	{
		PRINT( "Operating in command-line mode!\n" );
	}

	ogeRegisterConsoleVariables( engineTerminalMode );
	ogeRegisterConsoleCommands( engineTerminalMode );

	// Need to do this before anything else IO related
	ogeFileSystem_MountBaseLocations();

	// And now we can fetch the engine config that provides mount locations, aliases and more
	if ( config == NULL )
	{
		PRINT( "Shell didn't provide config - "
		       "checking for command-line argument, otherwise will use default.\n" );
		config = ENGINE_BASE_CONFIG;
	}
	const char *configPath = PlGetCommandLineArgumentValue( "-config" );
	engineConfig           = ndLoadFile( configPath != NULL ? configPath : config, "config" );
	if ( engineConfig == NULL )
	{
		PRINT_WARNING( "Failed to open engine config: %s\n", ndGetErrorMessage() );
		return false;
	}

	userConfig = ndLoadFile( FileSystem_GetUserConfigLocation(), "config" );
	if ( userConfig == NULL )
	{
		PRINT( "No existing user config found, will use defaults.\n" );
		userConfig = ndPushBackObject( NULL, "config" );
	}

	ogeFileSystem_SetupConfig( engineConfig );
	ogeFileSystem_MountLocations();

	PRINT( "Initializing core services...\n" );

	// TODO: move these somewhere more appropriate??
	PlmRegisterModelLoader( "mdl.n", Model_Cache, NULL );

	ogeInitializeProfiler();
	ogeInitializeScheduler();
	ogeInitializeMemoryManager();
	ogeInitializeNet();

	ogeInitializeServer();
	ogeInitializeClient();

	ogeInitializeGame();

	PRINT( "Initialization complete!\n" );

	engineInitialized = true;

	return true;
}

void ogeShutdown( void )
{
	PRINT( "Shutting down...\n" );

	ogeFlushTasks();

	ogeShutdownGame();
	ogeShutdownEditor();

	ogeShutdownClient();
	ogeShutdownServer();
	ogeShutdownConsole();
	ogeShutdownMemoryManager();
	ogeShutdownScheduler();
	ogeShutdownNet();

	ogeFileSystem_ClearMountedLocations();

	ogeShellInterface_Shutdown();

	engineInitialized = false;
}

unsigned int ogeGetNumTicks( void )
{
	return numTicks;
}

void ogeTickFrame( void )
{
	if ( !engineInitialized )
	{
		return;
	}

	OGE_PROFILE_START( PROFILE_SIM_ALL );

	ogeTickTasks();

	OGE_PROFILE_START( PROFILE_TICK_CLIENT );
	ogeTickClient();
	OGE_PROFILE_END( PROFILE_TICK_CLIENT );

	OGE_PROFILE_START( PROFILE_TICK_SERVER );
	ogeTickServer();
	OGE_PROFILE_END( PROFILE_TICK_SERVER );

	numTicks++;

	OGE_PROFILE_END( PROFILE_SIM_ALL );

	ogeProfiler_UpdateGraphs();
	ogeProfiler_EndFrame();
}

bool ogeIsEngineRunning( void )
{
	/* always running */
	return engineInitialized;
}

void ogeRenderFrame( OgeViewport *viewport )
{
	if ( !engineInitialized )
	{
		return;
	}

	assert( viewport != NULL );
	if ( viewport == NULL )
	{
		PRINT_WARNING( "Attempted to draw without a valid viewport!\n" );
		return;
	}

	OGE_PROFILE_START( PROFILE_DRAW_ALL );
	ogeDrawClient( viewport );
	OGE_PROFILE_END( PROFILE_DRAW_ALL );

	ogeProfiler_UpdateGraphs();
	ogeProfiler_EndFrame();
}

void ogeHandleKeyboardEvent( int key, unsigned int keyState )
{
	Client_Input_HandleKeyboardEvent( key, keyState );
}

bool ogeConsole_HandleTextEvent( const char *key );

void ogeHandleTextEvent( const char *key )
{
	if ( ogeConsole_HandleTextEvent( key ) )
	{
		return;
	}
}

void ogeHandleMouseButtonEvent( int button, YNCoreInputState buttonState )
{
	Client_Input_HandleMouseButtonEvent( button, buttonState );
}

void ogeHandleMouseWheelEvent( float x, float y )
{
	Client_Input_HandleMouseWheelEvent( x, y );
}

void ogeHandleMouseMotionEvent( int x, int y )
{
	Client_Input_HandleMouseMotionEvent( x, y );
}
