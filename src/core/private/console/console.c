// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Bulk of implementation.
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "client/ape_client_input.h"
#include "yin/core_fs.h"
#include "node/node_entity.h"

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

static void clear_console_command( unsigned int argc, char **argv )
{
	( void ) ( argc );
	( void ) ( argv );
	clear_output_buffer();
}

static void output_callback( int level, const char *message, QmMathColour4ub colour )
{
	ape_console_push_notification_( message, colour );

	size_t l = strlen( message );
	if ( l >= CONSOLE_BUFFER_MAX_LENGTH )
	{
		ape_warning_( "Attempting to push message to console with an unexpected length!\n" );
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

	ss_shell_push_message( level, message, &colour );
}

/* CONSOLE COMMANDS */

#define CMD_CALLBACK( NAME ) static void Cmd_##NAME( unsigned int argc, char **argv )

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
	ape_print_( "Version:  v" ENGINE_VERSION_STR " [" GIT_BRANCH "." GIT_COMMIT_COUNT "]\n"
	            "Compiled: " __DATE__ "\n" );
}

/*------------------------------------------------------------------*/

static void save_user_config( void );
static void load_user_config( void )
{
	AcmBranch *root = com_acm_load_file( ss_acl_fs_get_user_config_location(), "config" );
	if ( root == NULL )
		return;

	/* now iterate through the list and update all our children */
	AcmBranch *child = acm_get_first_child( root );
	while ( child != NULL )
	{
		const char *cvarName = acm_branch_get_name( child );
		char        cvarValue[ PL_SYSTEM_MAX_PATH ];
		if ( acm_branch_get_string( child, cvarValue, sizeof( cvarValue ) ) == ND_ERROR_SUCCESS )
			PlSetConsoleVariableByName( cvarName, cvarValue );
		else
			PRINT_WARNING( "Failed to fetch value: %s\n", cvarName );

		child = acm_get_next_child( child );
	}

	ape_deserialize_input_config_( root );

	PRINT( "User config loaded.\n" );
}

static void save_user_config( void )
{
	char path[ PL_SYSTEM_MAX_PATH ];
	snprintf( path, sizeof( path ), "%s", ss_acl_fs_get_user_config_location() );
	PRINT_DEBUG( "Saving user config: \"%s\"\n", path );

	PLConsoleVariable **cvars;
	size_t              numVars;
	PlGetConsoleVariables( &cvars, &numVars );

	AcmBranch *root = acm_push_object( nullptr, "config" );
	for ( unsigned int i = 0; i < numVars; ++i )
	{
		if ( !cvars[ i ]->archive )
			continue;
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

	PRINT( "User config saved.\n" );
}

static void toggle_command( unsigned int, char **argv )
{
	PLConsoleVariable *variable = PlGetConsoleVariable( argv[ 1 ] );
	if ( variable == nullptr )
	{
		ape_warning_( "Failed to find the specified variable (%s)!\n" );
		return;
	}
	if ( variable->type != PL_VAR_BOOL )
	{
		ape_warning_( "Console variable is not a boolean type!\n" );
		return;
	}

	PlSetConsoleVariable( variable, variable->b_value ? "false" : "true" );
}

void ape_test_register_commands_();
void ape_console_register_commands_( bool isDedicated )
{
	PlRegisterConsoleCommand( "quit", "Shutdown any existing server and terminate the application.", 0, Cmd_Quit );
	PlRegisterConsoleCommand( "exit", "Shutdown any existing server and terminate the application.", 0, Cmd_Quit );
	PlRegisterConsoleCommand( "version", "Prints out the current engine version.", 0, Cmd_Version );
	PlRegisterConsoleCommand( "clear", "Clear the console buffer.", 0, clear_console_command );
	PlRegisterConsoleCommand( "toggle", "Toggle a specific variable.", 1, toggle_command );

	ape_entity_register_commands_();
	ape_test_register_commands_();

	if ( !isDedicated )
	{
		ape_console_register_cl_commands_();
	}
}

static void validate_tick_frequency( PLConsoleVariable *variable )
{
	if ( variable->i_value > 0 )
	{
		return;
	}

	ape_warning_( "Invalid value specified for tick frequency (%i), resetting to default!\n", variable->i_value );

	char tmp[ 64 ];
	snprintf( tmp, sizeof( tmp ), "%u", APE_DEFAULT_TICK_RATE );
	PlSetConsoleVariable( variable, tmp );
}

void ape_server_register_console_variables_();
void ape_client_register_console_variables_();
void ape_model_register_console_variables_();
void ape_console_register_variables_( bool isDedicated )
{
	char tmp[ 64 ];
	snprintf( tmp, sizeof( tmp ), "%u", APE_DEFAULT_TICK_RATE );
	PlRegisterConsoleVariable( "tickFrequency", "Frequency of the tick rate in ms.", tmp, PL_VAR_I32, nullptr, validate_tick_frequency, false );
	PlRegisterConsoleVariable( "renderTimeLock", "Will only render a frame on tick.", "true", PL_VAR_BOOL, nullptr, nullptr, true );

	ape_server_register_console_variables_();
	ape_model_register_console_variables_();

	// Client variables
	if ( !isDedicated )
	{
		ape_client_register_console_variables_();
		ape_console_register_cl_variables_();
	}
}

static int logLevels[ APE_LOG_LEVELS ];

int Console_GetLogLevel( ApeConsoleLogLevel level )
{
	return logLevels[ level ];
}

void Console_Print( ApeConsoleLogLevel level, const char *message, ... )
{
	va_list args;
	va_start( args, message );

	int length = pl_vscprintf( message, args ) + 1;
	if ( length <= 0 )
		return;

	char *buf = QM_OS_MEMORY_NEW_( char, length );
	vsnprintf( buf, length, message, args );

	va_end( args );

	PlLogMessage( logLevels[ level ], buf );

	qm_os_memory_free( buf );
}

/**
 * Set the console up.
 */
void ape_initialize_console_( void )
{
	PlSetConsoleOutputCallback( output_callback );

	logLevels[ APE_LOG_ERROR ]       = PlAddLogLevel( "ape/error", PL_COLOUR_RED, true );
	logLevels[ APE_LOG_WARNING ]     = PlAddLogLevel( "ape/warning", PL_COLOUR_YELLOW, true );
	logLevels[ APE_LOG_INFORMATION ] = PlAddLogLevel( "ape", PL_COLOUR_WHITE, true );
	logLevels[ ACL_LOG_DEBUG ]       = PlAddLogLevel( "ape/debug", PL_COLOUR_ORCHID,
#if !defined( NDEBUG )
	                                            true
#else
	                                            false
#endif
	);
	logLevels[ APE_LOG_VERBOSE ] = PlAddLogLevel( "ape/verbose", PL_COLOUR_BLUE, false );

	logLevels[ APE_LOG_CLIENT_ERROR ]       = PlAddLogLevel( "ape/client/error", PL_COLOUR_RED, true );
	logLevels[ APE_LOG_CLIENT_WARNING ]     = PlAddLogLevel( "ape/client/warning", PL_COLOUR_YELLOW, true );
	logLevels[ APE_LOG_CLIENT_INFORMATION ] = PlAddLogLevel( "ape/client", PL_COLOUR_WHITE, true );

	logLevels[ APE_LOG_SERVER_ERROR ]       = PlAddLogLevel( "ape/server/error", PL_COLOUR_RED, true );
	logLevels[ APE_LOG_SERVER_WARNING ]     = PlAddLogLevel( "ape/server/warning", PL_COLOUR_YELLOW, true );
	logLevels[ APE_LOG_SERVER_INFORMATION ] = PlAddLogLevel( "ape/server", PL_COLOUR_WHITE, true );
}

void ape_shutdown_console_( void )
{
	clear_output_buffer();

	//save_user_config();
}
