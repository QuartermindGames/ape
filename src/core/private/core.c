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

static YNNodeBranch *engineConfig;
static YNNodeBranch *userConfig;

static bool engineTerminalMode = false;
static bool engineInitialized  = false;

/****************************************
 * PUBLIC
 ****************************************/

YNNodeBranch *YnCore_GetConfig( void ) { return engineConfig; }
YNNodeBranch *YnCore_GetUserConfig( void ) { return userConfig; }

bool ogeInitialize( const char *config )
{
	// Call this first, so we can buffer console output
	YnCore_InitializeConsole();

	PRINT( "Yin %d (%s / (%s:%s, %s)), Copyright (C) 2020-2023 Mark E Sowden\n",
	       VERSION_MAJOR,
	       ENGINE_VERSION_STR,
	       GIT_BRANCH, GIT_COMMIT_COUNT, GIT_COMMIT_HASH );
	PRINT( "Current working directory: \"%s\"\n", PlGetWorkingDirectory() );

	engineTerminalMode = PlHasCommandLineArgument( "cmd" );
	if ( engineTerminalMode )
		PRINT( "Operating in command-line mode!\n" );

	YnCore_RegisterConsoleVariables( engineTerminalMode );
	YnCore_RegisterConsoleCommands( engineTerminalMode );

	// Need to do this before anything else IO related
	YnCore_FileSystem_MountBaseLocations();

	// And now we can fetch the engine config that provides mount locations, aliases and more
	if ( config == NULL )
	{
		PRINT( "Shell didn't provide config - "
		       "checking for command-line argument, otherwise will use default.\n" );
		config = ENGINE_BASE_CONFIG;
	}
	const char *configPath = PlGetCommandLineArgumentValue( "-config" );
	engineConfig           = YnNode_LoadFile( configPath != NULL ? configPath : config, "config" );
	if ( engineConfig == NULL )
	{
		PRINT_WARNING( "Failed to open engine config: %s\n", YnNode_GetErrorMessage() );
		return false;
	}

	userConfig = YnNode_LoadFile( FileSystem_GetUserConfigLocation(), "config" );
	if ( userConfig == NULL )
	{
		PRINT( "No existing user config found, will use defaults.\n" );
		userConfig = YnNode_PushBackObject( NULL, "config" );
	}

	FileSystem_SetupConfig( engineConfig );
	FileSystem_MountLocations();

	PRINT( "Initializing core services...\n" );

	// TODO: move these somewhere more appropriate??
	PlmRegisterModelLoader( "mdl.n", Model_Cache, NULL );

	YnCore_InitializeProfiler();
	YnCore_InitializeScheduler();
	YnCore_InitializeMemoryManager();
	YnCore_InitializeNet();

	YnCore_InitializeServer();
	YnCore_InitializeClient();

	YnCore_InitializeGame();

	PRINT( "Initialization complete!\n" );

	engineInitialized = true;

	return true;
}

void ogeShutdown( void )
{
	PRINT( "Shutting down...\n" );

	YnCore_FlushTasks();

	YnCore_ShutdownGame();
	YnCore_ShutdownEditor();

	YnCore_ShutdownClient();
	YnCore_ShutdownServer();
	YnCore_ShutdownConsole();
	YnCore_ShutdownMemoryManager();
	YnCore_ShutdownScheduler();
	YnCore_ShutdownNet();

	YnCore_FileSystem_ClearMountedLocations();

	ogeShellInterface_Shutdown();

	engineInitialized = false;
}

unsigned int YnCore_GetNumTicks( void )
{
	return numTicks;
}

void ogeTickFrame( void )
{
	if ( !engineInitialized )
		return;

	YN_CORE_PROFILE_START( PROFILE_SIM_ALL );

	YinCore_TickTasks();

	YN_CORE_PROFILE_START( PROFILE_TICK_CLIENT );
	YnCore_TickClient();
	YN_CORE_PROFILE_END( PROFILE_TICK_CLIENT );

	YN_CORE_PROFILE_START( PROFILE_TICK_SERVER );
	YnCore_TickServer();
	YN_CORE_PROFILE_END( PROFILE_TICK_SERVER );

	numTicks++;

	YN_CORE_PROFILE_END( PROFILE_SIM_ALL );

	YnCore_Profiler_UpdateGraphs();
	YnCore_Profiler_EndFrame();
}

bool ogeIsEngineRunning( void )
{
	/* always running */
	return engineInitialized;
}

void ogeRenderFrame( YNCoreViewport *viewport )
{
	if ( !engineInitialized )
		return;

	assert( viewport != NULL );
	if ( viewport == NULL )
	{
		PRINT_WARNING( "Attempted to draw without a valid viewport!\n" );
		return;
	}

	YN_CORE_PROFILE_START( PROFILE_DRAW_ALL );
	YnCore_DrawClient( viewport );
	YN_CORE_PROFILE_END( PROFILE_DRAW_ALL );

	YnCore_Profiler_UpdateGraphs();
	YnCore_Profiler_EndFrame();
}

void ogeHandleKeyboardEvent( int key, unsigned int keyState )
{
	Client_Input_HandleKeyboardEvent( key, keyState );
}

bool YnCore_Console_HandleTextEvent( const char *key );

void ogeHandleTextEvent( const char *key )
{
	if ( YnCore_Console_HandleTextEvent( key ) )
		return;
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
