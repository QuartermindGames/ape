// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Bulk of implementation.
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "yin/core_fs.h"

#include "client/client_input.h"
#include "node/node_entity.h"

#include "console.h"

static int logLevels[ APE_LOG_LEVELS ];

static constexpr unsigned int CONSOLE_MAX_ARGS = 8;

void ape_console_push_notification_( const char *buffer, QmMathColour4ub colour );

/****************************************
 * CONSOLE OUTPUT BUFFER
 ****************************************/

static ApeConsoleOutput conOutputBuffer;

ApeConsoleOutput *apeGetConsoleOutput( void )
{
	return &conOutputBuffer;
}

static void clear_output_buffer( void )
{
	conOutputBuffer.numLines = 0;
}

static void clear_console_command( unsigned int argc, const char *const *argv )
{
	( void ) ( argc );
	( void ) ( argv );
	clear_output_buffer();
}

void ape_console_push_message_( const char *message, const QmMathColour4ub colour )
{
	ape_console_push_notification_( message, colour );

	size_t l = strlen( message );
	if ( l >= CONSOLE_BUFFER_MAX_LENGTH )
	{
		ape_console_warning_( "Attempting to push message to console with an unexpected length!\n" );
		l = CONSOLE_BUFFER_MAX_LENGTH - 2;
	}

	if ( conOutputBuffer.numLines >= CONSOLE_BUFFER_MAX_LINES )
	{
#define CON_JUMP 256
		memmove( conOutputBuffer.lines, &conOutputBuffer.lines[ CON_JUMP ], CONSOLE_BUFFER_MAX_LINES - CON_JUMP );
		conOutputBuffer.numLines -= CON_JUMP;
	}

	strncpy( conOutputBuffer.lines[ conOutputBuffer.numLines ].buffer, message, l );
	conOutputBuffer.lines[ conOutputBuffer.numLines ].buffer[ l ] = '\0';

	conOutputBuffer.lines[ conOutputBuffer.numLines ].colour = colour;
	conOutputBuffer.numLines++;
}

/* CONSOLE COMMANDS */

#define CMD_CALLBACK( NAME ) static void Cmd_##NAME( unsigned int argc, const char *const *argv )

CMD_CALLBACK( Quit )
{
	( void ) ( argc );
	( void ) ( argv );
	ape_shutdown();
}

CMD_CALLBACK( Version )
{
	( void ) ( argc );
	( void ) ( argv );
	ape_console_print_( "Version:  v" ENGINE_VERSION_STR " [" GIT_BRANCH "." GIT_COMMIT_COUNT "]\n"
	                    "Compiled: " __DATE__ "\n" );
}

/*------------------------------------------------------------------*/

#if 0
static void save_user_config( void );
static void load_user_config( void )
{
	AcmBranch *root = com_acm_load_file( ape_fs_get_user_config_location(), "config" );
	if ( root == nullptr )
		return;

	/* now iterate through the list and update all our children */
	AcmBranch *child = acm_get_first_child( root );
	while ( child != nullptr )
	{
		const char *cvarName = acm_branch_get_name( child );
		char        cvarValue[ PL_SYSTEM_MAX_PATH ];
		if ( acm_branch_get_string( child, cvarValue, sizeof( cvarValue ) ) == ND_ERROR_SUCCESS )
			ape_console_var_set( cvarName, cvarValue );
		else
			ape_console_warning_( "Failed to fetch value: %s\n", cvarName );

		child = acm_get_next_child( child );
	}

	ape_deserialize_input_config_( root );

	ape_console_print_( "User config loaded.\n" );
}

static void save_user_config( void )
{
	char path[ PL_SYSTEM_MAX_PATH ];
	snprintf( path, sizeof( path ), "%s", ape_fs_get_user_config_location() );
	ape_console_verbose_( "Saving user config: \"%s\"\n", path );

	ApeConsoleVar **cvars;
	size_t          numVars;
	PlGetConsoleVariables( &cvars, &numVars );

	AcmBranch *root = acm_push_object( nullptr, "config" );
	for ( unsigned int i = 0; i < numVars; ++i )
	{
		if ( !( cvars[ i ]->flags & APE_CONSOLE_VAR_FLAG_ARCHIVE ) )
		{
			continue;
		}

		/* don't bother storing it if it matches the default */
		if ( strcmp( cvars[ i ]->value, cvars[ i ]->default_value ) == 0 )
			continue;

		switch ( cvars[ i ]->type )
		{
			case PL_VAR_F32:
				acm_push_f32( root, cvars[ i ]->name, cvars[ i ]->f_value );
				break;
			case PL_VAR_I32:
				acm_push_i32( root, cvars[ i ]->name, cvars[ i ]->i_value );
				break;
			case PL_VAR_BOOL:
				acm_push_bool( root, cvars[ i ]->name, cvars[ i ]->b_value );
				break;
			default:
				acm_push_string( root, cvars[ i ]->name, cvars[ i ]->s_value, false );
				break;
		}
	}

	ape_serialize_input_config_( root );

	acm_write_file( path, root, ACM_FILE_TYPE_UTF8 );
	acm_branch_destroy( root );

	ape_console_print_( "User config saved.\n" );
}
#endif

static void toggle_command( unsigned int argc, const char *const *argv )
{
	ApeConsoleVar *variable = PlGetConsoleVariable( argv[ 1 ] );
	if ( variable == nullptr )
	{
		ape_console_warning_( "Failed to find the specified variable (%s)!\n" );
		return;
	}
	if ( variable->type != PL_VAR_BOOL )
	{
		ape_console_warning_( "Console variable is not a boolean type!\n" );
		return;
	}

	ape_console_var_set_( variable, variable->b_value ? "false" : "true" );
}

void ape_test_register_commands_();
void ape_console_register_commands_( bool isDedicated )
{
	ape_console_cmd_register( "quit", "Shutdown any existing server and terminate the application.", 0, Cmd_Quit );
	ape_console_cmd_register( "exit", "Shutdown any existing server and terminate the application.", 0, Cmd_Quit );
	ape_console_cmd_register( "version", "Prints out the current engine version.", 0, Cmd_Version );
	ape_console_cmd_register( "clear", "Clear the console buffer.", 0, clear_console_command );
	ape_console_cmd_register( "toggle", "Toggle a specific variable.", 1, toggle_command );

	ape_entity_register_commands_();
	ape_test_register_commands_();

	if ( !isDedicated )
	{
		ape_console_register_cl_commands_();
	}
}

void ape_server_register_console_variables_();
void ape_client_register_console_variables_();
void ape_model_register_console_variables_();
void ape_console_register_variables_( bool isDedicated )
{
	ape_console_var_register( "renderTimeLock", "Will only render a frame on tick.", "true", PL_VAR_BOOL, nullptr, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );

	ape_server_register_console_variables_();
	ape_model_register_console_variables_();

	// Client variables
	if ( !isDedicated )
	{
		ape_client_register_console_variables_();
		ape_console_register_cl_variables_();
	}
}

void ape_console_print_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	aux_log_push_message( logLevels[ APE_LOG_INFORMATION ], buf );
}

void ape_console_verbose_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	aux_log_push_message( logLevels[ APE_LOG_VERBOSE ], buf );
}

void ape_console_warning_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	aux_log_push_message( logLevels[ APE_LOG_WARNING ], "$cFF0000FFWARNING: $cFFFFFFFF%s", buf );
}

void ape_console_error_( const bool die, const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	aux_log_push_message( logLevels[ APE_LOG_ERROR ], "$cFF0000FFERROR: $cFFFFFFFF%s", buf );

	if ( die )
	{
		abort();
	}
}

static void console_help_command( unsigned int argc, const char *const *argv )
{
	if ( ape_console_var_help_( argv[ 1 ] ) || ape_console_cmd_help_( argv[ 1 ] ) )
	{
		return;
	}

	ape_console_print_( "Unknown variable/command, %s!\n", argv[ 1 ] );
}

static void console_find_command( unsigned int argc, const char *const *argv )
{
	const char *term = argv[ 1 ];

	bool findVars     = true;
	bool findCommands = true;
	if ( argc > 2 )
	{
		if ( strcmp( term, "var" ) == 0 )
		{
			findVars     = true;
			findCommands = false;
			term         = argv[ 2 ];
		}
		else if ( strcmp( term, "cmd" ) == 0 )
		{
			findVars     = false;
			findCommands = true;
			term         = argv[ 2 ];
		}
	}
	else
	{
		findVars = findCommands = true;
	}

	if ( term == nullptr )
	{
		ape_console_warning_( "Invalid arguments provided!\n" );
		return;
	}

	if ( findVars )
	{
		ape_console_var_find_( term );
	}

	if ( findCommands )
	{
		ape_console_cmd_find_( term );
	}
}

/**
 * Set the console up.
 */
void ape_initialize_console_( void )
{
	aux_log_set_callback( ape_console_push_message_ );

	ape_console_var_initialize_();
	ape_console_cmd_initialize_();

	ape_console_cmd_register( "help",
	                          "Returns information regarding specified command or variable.",
	                          1, console_help_command );
	ape_console_cmd_register( "find",
	                          "Find the specific command or variable. You can specify 'cmd' or 'vars' to filter.",
	                          1, console_find_command );

	logLevels[ APE_LOG_ERROR ]       = aux_log_register_source( "ape/error", PL_COLOUR_RED, true );
	logLevels[ APE_LOG_WARNING ]     = aux_log_register_source( "ape/warning", PL_COLOUR_YELLOW, true );
	logLevels[ APE_LOG_INFORMATION ] = aux_log_register_source( "ape", PL_COLOUR_WHITE, true );
	logLevels[ APE_LOG_VERBOSE ]     = aux_log_register_source( "ape/verbose", PL_COLOUR_BLUE, false );
	logLevels[ ACL_LOG_DEBUG ]       = aux_log_register_source( "ape/debug", PL_COLOUR_ORCHID,
#if !defined( NDEBUG )
	                                                             true
#else
	                                                             false
#endif
	);

	logLevels[ APE_LOG_CLIENT_ERROR ]       = aux_log_register_source( "ape/client/error", PL_COLOUR_RED, true );
	logLevels[ APE_LOG_CLIENT_WARNING ]     = aux_log_register_source( "ape/client/warning", PL_COLOUR_YELLOW, true );
	logLevels[ APE_LOG_CLIENT_INFORMATION ] = aux_log_register_source( "ape/client", PL_COLOUR_WHITE, true );

	logLevels[ APE_LOG_SERVER_ERROR ]       = aux_log_register_source( "ape/server/error", PL_COLOUR_RED, true );
	logLevels[ APE_LOG_SERVER_WARNING ]     = aux_log_register_source( "ape/server/warning", PL_COLOUR_YELLOW, true );
	logLevels[ APE_LOG_SERVER_INFORMATION ] = aux_log_register_source( "ape/server", PL_COLOUR_WHITE, true );
}

void ape_shutdown_console_( void )
{
	clear_output_buffer();

	//save_user_config();

	ape_console_cmd_shutdown_();
	ape_console_var_shutdown_();
}

void ape_console_parse( const char *string )
{
	if ( string == nullptr || *string == '\0' )
	{
		ape_console_verbose_( "Invalid string passed to ParseConsoleString!\n" );
		return;
	}

	static char **argv;
	if ( argv == nullptr )
	{
		argv = APE_MEMORY_NEW_C( char *, CONSOLE_MAX_ARGS );
		for ( char **arg = argv; arg < argv + CONSOLE_MAX_ARGS; ++arg )
		{
			*arg = APE_MEMORY_NEW_C( char, 1024 );
		}
	}

	unsigned int argc = 0;
	for ( const char *pos = string; *pos; )
	{
		size_t arglen = strcspn( pos, " " );
		if ( arglen > 0 )
		{
			strncpy( argv[ argc ], pos, arglen );
			argv[ argc ][ arglen ] = '\0';
			++argc;
		}
		pos += arglen;
		pos += strspn( pos, " " );
	}

	// need to cast for magical C reasons, wheeee
	if ( !ape_console_cmd_parse_( argv[ 0 ], argc, ( const char *const * ) argv ) && !ape_console_var_parse_( argv[ 0 ], argc, ( const char *const * ) argv ) )
	{
		ape_console_print_( "Unknown variable/command, %s!\n", argv[ 0 ] );
	}
}
