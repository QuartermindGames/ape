// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"

#include "yin/core_fs.h"

#include "client/ape_client.h"
#include "client/ape_client_input.h"
#include "client/renderer/renderer.h"

#include "server/server.h"
#include "net/net.h"
#include "editor/editor.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static unsigned int numTicks = 0;

static AcmBranch *engineConfig;
static AcmBranch *userConfig;

static bool engineTerminalMode = false;
static bool engineInitialized  = false;

static void execute_launch_commands( unsigned int argc, char **argv )
{
	// First, go over any command-line arguments here...
	for ( unsigned int i = 0; i < argc; ++i )
	{
		if ( *argv[ i ] != '+' )
		{
			continue;
		}

		char commandBuf[ 1024 ];
		snprintf( commandBuf, sizeof( commandBuf ), "%s", argv[ i ] + 1 );

		PlParseConsoleString( commandBuf );
	}

	if ( engineConfig == NULL )
	{
		return;
	}

	AcmBranch *branch = acm_branch_get_child_by_name( engineConfig, "launchCommands" );
	if ( branch == NULL )
	{
		return;
	}

	static const unsigned int MAX_COMMANDS = 256;
	unsigned int              numCommands  = acm_branch_get_num_of_children( branch );
	if ( numCommands == 0 )
	{
		return;
	}
	else if ( numCommands >= MAX_COMMANDS )
	{
		ape_warning_( "Excessive number of launch commands (%u >= %u), some commands will be ignored!\n", numCommands, MAX_COMMANDS );
		numCommands = ( MAX_COMMANDS - 1 );
	}

	char *commands[ MAX_COMMANDS ];
	if ( acm_branch_get_string_array( branch, commands, numCommands ) != ND_ERROR_SUCCESS )
	{
		return;
	}

	for ( unsigned int i = 0; i < numCommands; ++i )
	{
		if ( commands[ i ] == NULL )
		{
			continue;
		}

		PlParseConsoleString( commands[ i ] );
		PL_DELETE( commands[ i ] );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeConfig ape_config_;

AcmBranch *ape_get_config( void ) { return engineConfig; }
AcmBranch *ape_get_user_config( void ) { return userConfig; }

void ape_print_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( Console_GetLogLevel( APE_LOG_INFORMATION ), buf );
}

void ape_warning_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( Console_GetLogLevel( APE_LOG_WARNING ), buf );
}

void ape_error_( bool die, const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( Console_GetLogLevel( APE_LOG_ERROR ), buf );

	if ( die )
	{
		abort();
	}
}

void ape_initialize_world_();
void ape_shutdown_world_();

bool ape_initialize( unsigned int argc, char **argv, const char *config )
{
	PL_ZERO_( ape_config_ );

	PlRegisterStandardPackageLoaders( PL_PACKAGE_LOAD_FORMAT_ALL );

	// Call this first, so we can buffer console output
	ape_initialize_console_();

	ape_print_( ENGINE_NAME " %d (%s / (%s:%s, %s)), Copyright (C) 2020-2024 SnortySoft, Mark E. Sowden\n",
	            VERSION_MAJOR,
	            ENGINE_VERSION_STR,
	            GIT_BRANCH, GIT_COMMIT_COUNT, GIT_COMMIT_HASH );
	ape_print_( "Current working directory: \"%s\"\n", PlGetWorkingDirectory() );

	engineTerminalMode = PlHasCommandLineArgument( "cmd" );
	if ( engineTerminalMode )
	{
		ape_print_( "Operating in command-line mode!\n" );
	}

	ape_console_register_variables_( engineTerminalMode );
	ape_console_register_commands_( engineTerminalMode );

	// Need to do this before anything else IO related
	ape_fs_mount_base_locations();

	// And now we can fetch the configs that provide mount locations, aliases, and more
	engineConfig = com_get_config( config != nullptr ? config : "engine" );
	userConfig   = com_get_config( "user" );

	ape_fs_setup_config( engineConfig );

	ape_print_( "Initializing core services...\n" );

	ape_initialize_scheduler_();
	ape_initialize_memory_manager_();
	ape_initialize_net_();
	ape_initialize_server_();
	ape_initialize_client_();
	ape_initialize_world_();
	ape_initialize_game_();
	ape_initialize_editor_();

	ape_print_( "Initialization complete!\n" );

	engineInitialized = true;

	execute_launch_commands( argc, argv );

	return true;
}

void ape_shutdown( void )
{
	ape_print_( "Shutting down...\n" );

	ss_acl_flush_tasks_();

	ape_shutdown_editor_();
	ape_shutdown_game_();
	ape_shutdown_world_();
	ape_shutdown_client_();
	ape_shutdown_server_();
	ape_shutdown_console_();
	ape_shutdown_memory_manager_();
	ape_shutdown_scheduler_();
	ape_shutdown_net_();

	com_write_config( engineConfig, "engine" );
	acm_branch_destroy( engineConfig );

	com_write_config( userConfig, "user" );
	acm_branch_destroy( userConfig );

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
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	ape_tick_tasks_();
	//TODO: what order should these be?
	ape_tick_server_();
	ape_tick_client_();

	if ( ape_get_capture_state_() )
	{
		ApeViewport *viewport = ss_shell_viewport_get_active();
		if ( viewport != nullptr )
		{
			ape_render_frame_( viewport );
		}
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
	assert( viewport != nullptr );

	if ( !engineInitialized )
	{
		return;
	}

	// If we're capturing, ignore the request from the
	// caller to render the frame because we'll lock it
	// with the frame tick instead...
	if ( ape_get_capture_state_() )
	{
		return;
	}

	COM_PROFILE_FUNCTION_CALL( ape_render_frame_( viewport ) );
}

void ape_input_handle_keyboard_event( int key, bool isPressed )
{
	ape_client_input_handle_key_event_( key, isPressed );
}

bool ape_console_handle_text_event_( const char *key );
void ape_input_handle_text_event( const char *key )
{
	if ( ape_console_handle_text_event_( key ) )
	{
		return;
	}
}

void ape_input_handle_mouse_button_event( int button, ApeInputState buttonState )
{
	ape_client_input_handle_mouse_button_event_( button, buttonState );
}

void ape_input_handle_mouse_wheel_event( float x, float y )
{
	ape_client_input_handle_mouse_wheel_event( x, y );
}

void ape_input_handle_mouse_motion_event( int x, int y )
{
	Client_Input_HandleMouseMotionEvent( x, y );
}
