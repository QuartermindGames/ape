// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"

#include "yin/core_fs.h"

#include "client/ape_client.h"
#include "client/ape_client_input.h"
#include "client/renderer/renderer.h"

#include "server/server.h"
#include "net/net.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static unsigned int numTicks = 0;

static NdBranch *engineConfig;
static NdBranch *userConfig;

static bool engineTerminalMode = false;
static bool engineInitialized = false;

static void execute_launch_commands( unsigned int argc, char **argv )
{
	// First go over any command-line arguments here...
	for ( unsigned int i = 0; i < argc; ++i )
	{
		if ( *argv[ i ] != '+' )
			continue;

		const char *command = argv[ i + 1 ];
		const char *argument = NULL;

		char commandBuf[ 1024 ];
		if ( argument == NULL )
			snprintf( commandBuf, sizeof( commandBuf ), "%s", command );
		else
			snprintf( commandBuf, sizeof( commandBuf ), "%s %s", command, argument );

		PlParseConsoleString( commandBuf );
	}

	if ( engineConfig == NULL )
		return;

	NdBranch *branch = ndGetChildByName( engineConfig, "launchCommands" );
	if ( branch == NULL )
		return;

	static const unsigned int MAX_COMMANDS = 256;
	unsigned int numCommands = ndGetNumOfChildren( branch );
	if ( numCommands == 0 )
		return;
	else if ( numCommands >= MAX_COMMANDS )
	{
		PRINT_WARNING( "Excessive number of launch commands (%u >= %u), some commands will be ignored!\n", numCommands, MAX_COMMANDS );
		numCommands = ( MAX_COMMANDS - 1 );
	}

	char *commands[ MAX_COMMANDS ];
	if ( ndGetStringArray( branch, commands, numCommands ) != ND_ERROR_SUCCESS )
		return;

	for ( unsigned int i = 0; i < numCommands; ++i )
	{
		if ( commands[ i ] == NULL )
			continue;

		PlParseConsoleString( commands[ i ] );
		PL_DELETE( commands[ i ] );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeConfig ape_config_;

NdBranch *ss_acl_get_config( void ) { return engineConfig; }
NdBranch *ss_acl_get_user_config( void ) { return userConfig; }

bool ape_initialize( unsigned int argc, char **argv, const char *config )
{
	PL_ZERO_( ape_config_ );

	PlRegisterStandardPackageLoaders( PL_PACKAGE_LOAD_FORMAT_ALL );

	// Call this first, so we can buffer console output
	ape_initialize_console_();

	PRINT( ENGINE_NAME " %d (%s / (%s:%s, %s)), Copyright (C) 2020-2023 SnortySoft, Mark E Sowden\n",
	       VERSION_MAJOR,
	       ENGINE_VERSION_STR,
	       GIT_BRANCH, GIT_COMMIT_COUNT, GIT_COMMIT_HASH );
	PRINT( "Current working directory: \"%s\"\n", PlGetWorkingDirectory() );

	engineTerminalMode = PlHasCommandLineArgument( "cmd" );
	if ( engineTerminalMode )
		PRINT( "Operating in command-line mode!\n" );

	ape_console_register_variables_( engineTerminalMode );
	ape_console_register_commands_( engineTerminalMode );

	// Need to do this before anything else IO related
	ape_fs_mount_base_locations();

	// And now we can fetch the configs that provides mount locations, aliases and more
	engineConfig = com_get_config( config != NULL ? config : "engine" );
	userConfig = com_get_config( "user" );

	ape_fs_setup_config( engineConfig );

	PRINT( "Initializing core services...\n" );

	ape_initialize_scheduler_();
	ape_initialize_memory_manager_();
	ape_initialize_net_();

	ape_initialize_server_();
	ape_initialize_client_();

	ape_initialize_game_();

	PRINT( "Initialization complete!\n" );

	engineInitialized = true;

	execute_launch_commands( argc, argv );

	return true;
}

void ape_shutdown( void )
{
	PRINT( "Shutting down...\n" );

	ss_acl_flush_tasks_();

	ape_shutdown_game_();

	ape_shutdown_client_();
	ape_shutdown_server_();
	ape_shutdown_console_();
	ape_shutdown_memory_manager_();
	ape_shutdown_scheduler_();
	ape_shutdown_net_();

	com_write_config( engineConfig, "engine" );
	ndDestroyBranch( engineConfig );

	com_write_config( userConfig, "user" );
	ndDestroyBranch( userConfig );

	ss_shell_shutdown();

	engineInitialized = false;
}

unsigned int ape_get_num_ticks( void )
{
	return numTicks;
}

void ape_tick_frame( void )
{
	if ( !engineInitialized )
		return;

	COM_PROFILE_FUNCTION_START();

	ss_acl_tick_tasks_();
	ape_tick_client_();
	ape_server_tick_();

	if ( ape_get_capture_state_() )
	{
		ApeViewport *viewport = ss_shell_viewport_get_active();
		if ( viewport != NULL )
			ape_render_frame_( viewport );
	}

	numTicks++;

	COM_PROFILE_FUNCTION_END();
}

bool ape_is_running( void )
{
	/* always running */
	return engineInitialized;
}

void ape_render_frame( ApeViewport *viewport )
{
	assert( viewport != NULL );

	if ( !engineInitialized )
		return;

	// If we're capturing, ignore the request from the
	// caller to render the frame because we'll lock it
	// with the frame tick instead...
	if ( ape_get_capture_state_() )
		return;

	COM_PROFILE_FUNCTION_CALL( ape_render_frame_( viewport ) );
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

void ape_input_handle_mouse_wheel_event( float x, float y )
{
	printf( "%f\n", y );
	Client_Input_HandleMouseWheelEvent( x, y );
}

void ape_input_handle_mouse_motion_event( int x, int y )
{
	Client_Input_HandleMouseMotionEvent( x, y );
}
