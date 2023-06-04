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

NdBranch *apeGetConfig( void ) { return engineConfig; }
NdBranch *apeGetUserConfig( void ) { return userConfig; }

bool apeInitialize( const char *config )
{
	// Call this first, so we can buffer console output
	apeInitializeConsole();

	PRINT( ENGINE_NAME " %d (%s / (%s:%s, %s)), Copyright (C) 2020-2023 Mark E Sowden\n",
	       VERSION_MAJOR,
	       ENGINE_VERSION_STR,
	       GIT_BRANCH, GIT_COMMIT_COUNT, GIT_COMMIT_HASH );
	PRINT( "Current working directory: \"%s\"\n", PlGetWorkingDirectory() );

	engineTerminalMode = PlHasCommandLineArgument( "cmd" );
	if ( engineTerminalMode )
	{
		PRINT( "Operating in command-line mode!\n" );
	}

	apeRegisterConsoleVariables_( engineTerminalMode );
	apeRegisterConsoleCommands_( engineTerminalMode );

	// Need to do this before anything else IO related
	apeMountBaseLocations();

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
	apeMountLocations();

	PRINT( "Initializing core services...\n" );

	// TODO: move these somewhere more appropriate??
	PlmRegisterModelLoader( "mdl.n", apeCacheModel, NULL );

	apeInitializeProfiler();
	apeInitializeScheduler();
	apeInitializeMemoryManager();
	apeInitializeNet();

	apeInitializeServer();
	apeInitializeClient();

	apeInitializeGame();

	PRINT( "Initialization complete!\n" );

	engineInitialized = true;

	return true;
}

void apeShutdown( void )
{
	PRINT( "Shutting down...\n" );

	apeFlushTasks();

	apeShutdownGame();
	ogeShutdownEditor();

	ogeShutdownClient();
	apeShutdownServer();
	apeShutdownConsole();
	apeShutdownMemoryManager();
	apeShutdownScheduler();
	ogeShutdownNet();

	ogeFileSystem_ClearMountedLocations();

	apeShellInterface_Shutdown();

	engineInitialized = false;
}

unsigned int apeGetNumTicks( void )
{
	return numTicks;
}

void apeTickFrame( void )
{
	if ( !engineInitialized )
	{
		return;
	}

	OGE_PROFILE_START( PROFILE_SIM_ALL );

	apeTickTasks();

	OGE_PROFILE_START( PROFILE_TICK_CLIENT );
	apeTickClient();
	OGE_PROFILE_END( PROFILE_TICK_CLIENT );

	OGE_PROFILE_START( PROFILE_TICK_SERVER );
	apeTickServer();
	OGE_PROFILE_END( PROFILE_TICK_SERVER );

	numTicks++;

	OGE_PROFILE_END( PROFILE_SIM_ALL );

	apeUpdateProfilerGraphs();
	apeEndProfilerFrame();
}

bool apeIsEngineRunning( void )
{
	/* always running */
	return engineInitialized;
}

void apeRenderFrame( ApeViewport *viewport )
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
	apeDrawClient( viewport );
	OGE_PROFILE_END( PROFILE_DRAW_ALL );

	apeUpdateProfilerGraphs();
	apeEndProfilerFrame();
}

void apeHandleKeyboardEvent( int key, unsigned int keyState )
{
	Client_Input_HandleKeyboardEvent( key, keyState );
}

bool apeHandleConsoleTextEvent_( const char *key );

void apeHandleTextEvent( const char *key )
{
	if ( apeHandleConsoleTextEvent_( key ) )
	{
		return;
	}
}

void apeHandleMouseButtonEvent( int button, ApeInputState buttonState )
{
	Client_Input_HandleMouseButtonEvent( button, buttonState );
}

void apeHandleMouseWheelEvent( float x, float y )
{
	Client_Input_HandleMouseWheelEvent( x, y );
}

void apeHandleMouseMotionEvent( int x, int y )
{
	Client_Input_HandleMouseMotionEvent( x, y );
}
