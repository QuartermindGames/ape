// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "qmos/public/qm_os_time.h"

#include "ape_private.h"

#include "yin/core_fs.h"

#include "client/client.h"
#include "client/client_input.h"
#include "renderer/renderer.h"

#include "server/server.h"
#include "net/net.h"
#include "editor/editor.h"
#include "script/script.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static uint64_t numTicks = 0;

static AcmBranch *engineConfig;
static AcmBranch *userConfig;

static bool engineTerminalMode;
static bool engineInitialized;

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

		ape_console_parse( commandBuf );
	}

	if ( engineConfig == NULL )
	{
		return;
	}

	AcmBranch *branch = acm_get_child_by_name( engineConfig, "launchCommands" );
	if ( branch == NULL )
	{
		return;
	}

	static constexpr unsigned int MAX_COMMANDS = 256;
	unsigned int                  numCommands  = acm_get_num_of_children( branch );
	if ( numCommands == 0 )
	{
		return;
	}
	else if ( numCommands >= MAX_COMMANDS )
	{
		ape_console_warning_( "Excessive number of launch commands (%u >= %u), some commands will be ignored!\n", numCommands, MAX_COMMANDS );
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

		ape_console_parse( commands[ i ] );
		qm_os_memory_free( commands[ i ] );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeConfig ape_config_;

AcmBranch *ape_get_config_( void )
{
	return engineConfig;
}

AcmBranch *ape_get_user_config_( void )
{
	return userConfig;
}

bool ape_is_dedicated()
{
	return engineTerminalMode;
}

static double lastTime;

static int  tickFrequency;
static void validate_tick_frequency( ApeConsoleVar *variable )
{
	if ( variable->i_value > 0 )
	{
		return;
	}

	ape_console_warning_( "Invalid value specified for tick frequency (%i), resetting to default!\n", variable->i_value );

	char tmp[ 64 ];
	snprintf( tmp, sizeof( tmp ), "%u", APE_DEFAULT_TICK_RATE );
	ape_console_var_set_( variable, tmp );
}

unsigned int ape_get_tick_frequency()
{
	return tickFrequency;
}

bool ape_initialize( unsigned int argc, char **argv, const char *config )
{
	ape_config_ = ( ApeConfig ) {};

	if ( config != nullptr && strcmp( config, "editor" ) == 0 )
	{
		extern bool ape_editorStatus_;
		ape_editorStatus_ = true;
	}

	PlRegisterStandardPackageLoaders( PL_PACKAGE_LOAD_FORMAT_ALL );

	// Call this first, so we can buffer console output
	ape_initialize_console_();

	ape_console_print_( ENGINE_NAME " %d (%s / (%s:%s, %s)), " COM_COPYRIGHT "\n",
	                    VERSION_MAJOR,
	                    ENGINE_VERSION_STR,
	                    GIT_BRANCH, GIT_COMMIT_COUNT, GIT_COMMIT_HASH );
	ape_console_print_( "Current working directory: \"%s\"\n", PlGetWorkingDirectory() );

	engineTerminalMode = PlHasCommandLineArgument( "cmd" );
	if ( engineTerminalMode )
	{
		ape_console_print_( "Operating in command-line mode!\n" );
	}

	// Need to do this before anything else IO related
	ape_fs_mount_base_locations();

	// And now we can fetch the configs that provide mount locations, aliases, and more
	engineConfig = com_get_config( config != nullptr ? config : "engine" );
	userConfig   = com_get_config( "user" );

	ape_console_register_variables_( engineTerminalMode );
	ape_console_register_commands_( engineTerminalMode );

	char tmp[ 64 ];
	snprintf( tmp, sizeof( tmp ), "%u", APE_DEFAULT_TICK_RATE );
	ape_console_var_register( "tickFrequency", "Frequency of the tick rate in ms.", tmp, PL_VAR_I32, &tickFrequency, validate_tick_frequency, 0 );

	ape_fs_setup_config( engineConfig );

	ape_console_print_( "Initializing core services...\n" );

	ape_scheduler_initialize_();
	ape_memory_initialize_();
	ape_initialize_net_();
	ape_script_manager_initialize_();
	ape_initialize_server_();
	ape_initialize_client_();
	ape_initialize_game_();
	ape_initialize_editor_();

	ape_console_print_( "Initialization complete!\n" );

	lastTime = qm_os_time_get_seconds();

	engineInitialized = true;

	execute_launch_commands( argc, argv );

	return true;
}

void ape_shutdown( void )
{
	ape_console_print_( "Shutting down...\n" );

	ape_shutdown_editor_();
	ape_shutdown_game_();
	ape_shutdown_client_();
	ape_shutdown_server_();
	ape_shutdown_console_();
	ape_script_manager_shutdown_();
	ape_memory_shutdown_();
	ape_scheduler_shutdown_();
	ape_shutdown_net_();

	com_write_config( engineConfig, "engine" );
	acm_branch_destroy( engineConfig );

	com_write_config( userConfig, "user" );
	acm_branch_destroy( userConfig );

	ss_shell_shutdown();

	engineInitialized = false;
}

uint64_t ape_get_num_ticks( void )
{
	return numTicks;
}

void ape_tick_frame()
{
	if ( !engineInitialized )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	const double now   = qm_os_time_get_seconds();
	const double delta = QM_MATH_CLAMP( 0.0, now - lastTime, 0.1 );
	lastTime += delta;

	ape_draw_debug_clear_();

	ape_scheduler_tick_();

	//TODO: what order should these be?
	ape_tick_server_( delta );
	ape_tick_client_( delta );

	if ( ape_get_capture_state_() )
	{
		ApeViewport *viewport = ss_shell_viewport_get_active();
		if ( viewport != nullptr )
		{
			ape_render_frame_( viewport );
		}
	}

	// we would have to run for an infinitely long time to hit this,
	// but regardless, it makes me feel better having it here...
	if ( numTicks == INT64_MAX )
	{
		ape_console_warning_( "Hit maximum tick limit, resetting!\n" );
		numTicks = 0;
	}
	else
	{
		numTicks++;
	}

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

	ape_render_frame_( viewport );
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
