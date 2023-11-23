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

#include "script_public.h"
#include "client/renderer/renderer.h"

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

bool ss_acl_initialize( const char *config )
{
	PL_ZERO_( ape_config_ );

	// Call this first, so we can buffer console output
	ss_acl_initialize_console_();

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

	ss_acl_console_register_variables_( engineTerminalMode );
	ss_acl_console_register_commands_( engineTerminalMode );

	ss_script_register_commands();

	PlRegisterStandardPackageLoaders();

	// Need to do this before anything else IO related
	ss_acl_fs_mount_base_locations();

	// And now we can fetch the engine config that provides mount locations, aliases and more
	if ( config == NULL )
	{
		PRINT( "Shell didn't provide config - "
		       "checking for command-line argument, otherwise will use default.\n" );
		config = ENGINE_BASE_CONFIG;
	}
	const char *configPath = PlGetCommandLineArgumentValue( "-config" );
	engineConfig = ndLoadFile( configPath != NULL ? configPath : config, "config" );
	if ( engineConfig == NULL )
	{
		PRINT_WARNING( "Failed to open engine config: %s\n", ndGetErrorMessage() );
		return false;
	}

	userConfig = ndLoadFile( ss_acl_fs_get_user_config_location(), "config" );
	if ( userConfig == NULL )
	{
		PRINT( "No existing user config found, will use defaults.\n" );
		userConfig = ndPushBackObject( NULL, "config" );
	}

	ss_acl_fs_setup_config( engineConfig );

	PRINT( "Initializing core services...\n" );

	// TODO: move these somewhere more appropriate??
	PlmRegisterModelLoader( "mdl.n", apeCacheModel, NULL );

	apeInitializeScheduler();
	apeInitializeMemoryManager();
	apeInitializeNet();

	apeInitializeServer();
	apeInitializeClient_();

	ss_acl_initialize_game_();

	PRINT( "Initialization complete!\n" );

	engineInitialized = true;

	return true;
}

void ss_acl_shutdown( void )
{
	PRINT( "Shutting down...\n" );

	apeFlushTasks();

	ss_acl_shutdown_game_();
	apeShutdownEditor_();

	apeShutdownClient_();
	apeShutdownServer();
	ss_acl_shutdown_console_();
	apeShutdownMemoryManager();
	apeShutdownScheduler();
	ogeShutdownNet();

	ss_shell_shutdown();

	engineInitialized = false;
}

unsigned int ss_acl_get_num_ticks( void )
{
	return numTicks;
}

void ss_acl_tick_frame( void )
{
	if ( !engineInitialized )
		return;

	COM_PROFILE_FUNCTION_START();

	apeTickTasks();
	apeTickClient();
	apeTickServer();

	if ( ss_arl_get_capture_state_() )
	{
		SS_Arl_Viewport *viewport = ss_shell_viewport_get_active();
		if ( viewport != NULL )
			ss_arl_render_frame( viewport );
	}

	numTicks++;

	COM_PROFILE_FUNCTION_END();
}

bool ss_acl_is_engine_running( void )
{
	/* always running */
	return engineInitialized;
}

void ss_acl_render_frame( SS_Arl_Viewport *viewport )
{
	if ( !engineInitialized )
		return;

	assert( viewport != NULL );
	if ( viewport == NULL )
	{
		PRINT_WARNING( "Attempted to draw without a valid viewport!\n" );
		return;
	}

	COM_PROFILE_FUNCTION_CALL( "ss_arl_render_frame", ss_arl_render_frame( viewport ) );
}

void ss_acl_input_handle_keyboard_event( int key, unsigned int keyState )
{
	Client_Input_HandleKeyboardEvent( key, keyState );
}

bool acl_console_handle_text_event_( const char *key );

void ss_acl_input_handle_text_event( const char *key )
{
	if ( acl_console_handle_text_event_( key ) )
	{
		return;
	}
}

void ss_acl_input_handle_mouse_button_event( int button, ApeInputState buttonState )
{
	Client_Input_HandleMouseButtonEvent( button, buttonState );
}

void ss_acl_input_handle_mouse_wheel_event( float x, float y )
{
	Client_Input_HandleMouseWheelEvent( x, y );
}

void ss_acl_input_handle_mouse_motion_event( int x, int y )
{
	Client_Input_HandleMouseMotionEvent( x, y );
}
