// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"

#include "yin/core_game.h"
#include "yin/core_fs.h"

#include "model/model.h"
#include "client/ape_client.h"
#include "client/ape_client_input.h"
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
static bool engineInitialized = false;

/****************************************
 * PUBLIC
 ****************************************/

ApeConfig ape_config_;

NdBranch *apeGetConfig( void ) { return engineConfig; }
NdBranch *apeGetUserConfig( void ) { return userConfig; }

bool apeInitialize( const char *config ) {
	PL_ZERO_( ape_config_ );

	// Call this first, so we can buffer console output
	apeInitializeConsole();

	PRINT( ENGINE_NAME " %d (%s / (%s:%s, %s)), Copyright (C) 2020-2023 Mark E Sowden\n",
	       VERSION_MAJOR,
	       ENGINE_VERSION_STR,
	       GIT_BRANCH, GIT_COMMIT_COUNT, GIT_COMMIT_HASH );
	PRINT( "Current working directory: \"%s\"\n", PlGetWorkingDirectory() );

	engineTerminalMode = PlHasCommandLineArgument( "cmd" );
	if ( engineTerminalMode ) {
		PRINT( "Operating in command-line mode!\n" );
	}

	acl_console_register_variables_( engineTerminalMode );
	acl_console_register_commands_( engineTerminalMode );

	PlRegisterStandardPackageLoaders();

	// Need to do this before anything else IO related
	apeMountBaseLocations();

	// And now we can fetch the engine config that provides mount locations, aliases and more
	if ( config == NULL ) {
		PRINT( "Shell didn't provide config - "
		       "checking for command-line argument, otherwise will use default.\n" );
		config = ENGINE_BASE_CONFIG;
	}
	const char *configPath = PlGetCommandLineArgumentValue( "-config" );
	engineConfig = ndLoadFile( configPath != NULL ? configPath : config, "config" );
	if ( engineConfig == NULL ) {
		PRINT_WARNING( "Failed to open engine config: %s\n", ndGetErrorMessage() );
		return false;
	}

	userConfig = ndLoadFile( acl_get_user_config_location(), "config" );
	if ( userConfig == NULL ) {
		PRINT( "No existing user config found, will use defaults.\n" );
		userConfig = ndPushBackObject( NULL, "config" );
	}

	acl_setup_config( engineConfig );

	PRINT( "Initializing core services...\n" );

	// TODO: move these somewhere more appropriate??
	PlmRegisterModelLoader( "mdl.n", apeCacheModel, NULL );

	apeInitializeScheduler();
	apeInitializeMemoryManager();
	apeInitializeNet();

	apeInitializeServer();
	apeInitializeClient_();

	acl_initialize_game_();

	PRINT( "Initialization complete!\n" );

	engineInitialized = true;

	return true;
}

void apeShutdown( void ) {
	PRINT( "Shutting down...\n" );

	apeFlushTasks();

	acl_shutdown_game_();
	apeShutdownEditor_();

	apeShutdownClient_();
	apeShutdownServer();
	apeShutdownConsole();
	apeShutdownMemoryManager();
	apeShutdownScheduler();
	ogeShutdownNet();

	apeShellInterface_Shutdown();

	engineInitialized = false;
}

unsigned int apeGetNumTicks( void ) {
	return numTicks;
}

void apeTickFrame( void ) {
	if ( !engineInitialized ) {
		return;
	}

	COM_PROFILE_FUNCTION_START();

	apeTickTasks();

#if !defined( APE_EDITOR_ENABLED )

	apeTickClient();
	apeTickServer();

#else

	if ( edIsActive() ) {
		edTick();
	} else {
		APE_PROFILE_START( PROFILE_TICK_CLIENT );
		apeTickClient();
		APE_PROFILE_END( PROFILE_TICK_CLIENT );

		APE_PROFILE_START( PROFILE_TICK_SERVER );
		apeTickServer();
		APE_PROFILE_END( PROFILE_TICK_SERVER );
	}

#endif

	numTicks++;

	COM_PROFILE_FUNCTION_END();
}

bool apeIsEngineRunning( void ) {
	/* always running */
	return engineInitialized;
}

void apeRenderFrame( ApeViewport *viewport ) {
	if ( !engineInitialized ) {
		return;
	}

	assert( viewport != NULL );
	if ( viewport == NULL ) {
		PRINT_WARNING( "Attempted to draw without a valid viewport!\n" );
		return;
	}

	COM_PROFILE_FUNCTION_CALL( "apeDrawClient", apeDrawClient( viewport ) );
}

void apeHandleKeyboardEvent( int key, unsigned int keyState ) {
	Client_Input_HandleKeyboardEvent( key, keyState );
}

bool apeHandleConsoleTextEvent_( const char *key );

void apeHandleTextEvent( const char *key ) {
	if ( apeHandleConsoleTextEvent_( key ) ) {
		return;
	}
}

void apeHandleMouseButtonEvent( int button, ApeInputState buttonState ) {
	Client_Input_HandleMouseButtonEvent( button, buttonState );
}

void apeHandleMouseWheelEvent( float x, float y ) {
	Client_Input_HandleMouseWheelEvent( x, y );
}

void apeHandleMouseMotionEvent( int x, int y ) {
	Client_Input_HandleMouseMotionEvent( x, y );
}
