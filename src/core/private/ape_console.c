// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"

#include "client/ape_client_input.h"
#include "yin/core_fs.h"

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

static void output_callback( int level, const char *message, PLColour colour )
{
	size_t l = strlen( message );
	if ( l >= CONSOLE_BUFFER_MAX_LENGTH )
	{
		PRINT_WARNING( "Attempting to push message to console with an unexpected length!\n" );
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
	PRINT( "Version: v" ENGINE_VERSION_STR " [" GIT_BRANCH "." GIT_COMMIT_COUNT "]\n" );
}

/*------------------------------------------------------------------*/

static void save_user_config( void );
static void LoadUserConfig( void )
{
	NdBranch *root = ndLoadFile( ss_acl_fs_get_user_config_location(), "config" );
	if ( root == NULL )
		return;

	/* now iterate through the list and update all our children */
	NdBranch *child = ndGetFirstChild( root );
	while ( child != NULL )
	{
		const char *cvarName = ndGetName( child );
		char cvarValue[ PL_SYSTEM_MAX_PATH ];
		if ( ndGetStr( child, cvarValue, sizeof( cvarValue ) ) == ND_ERROR_SUCCESS )
			PlSetConsoleVariableByName( cvarName, cvarValue );
		else
			PRINT_WARNING( "Failed to fetch value: %s\n", cvarName );

		child = ndGetNextChild( child );
	}

	apeDeserializeInputConfig_( root );

	PRINT( "User config loaded.\n" );
}

static void save_user_config( void )
{
	char path[ PL_SYSTEM_MAX_PATH ];
	snprintf( path, sizeof( path ), "%s", ss_acl_fs_get_user_config_location() );
	PRINT_DEBUG( "Saving user config: \"%s\"\n", path );

	PLConsoleVariable **cvars;
	size_t numVars;
	PlGetConsoleVariables( &cvars, &numVars );

	NdBranch *root = ndPushBackObject( NULL, "config" );
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
				ndPushBackF32( root, cvars[ i ]->name, cvars[ i ]->f_value );
				break;
			case PL_VAR_I32:
				ndPushBackI32( root, cvars[ i ]->name, cvars[ i ]->i_value );
				break;
			case PL_VAR_BOOL:
				ndPushBackBool( root, cvars[ i ]->name, cvars[ i ]->b_value );
				break;
			default:
				ndPushBackString( root, cvars[ i ]->name, cvars[ i ]->s_value );
				break;
		}
	}

	apeSerializeInputConfig_( root );

	ndWriteFile( path, root, ND_FILE_UTF8 );
	ndDestroyBranch( root );

	PRINT( "User config saved.\n" );
}

void ss_acl_console_register_commands_( bool isDedicated )
{
	PlRegisterConsoleCommand( "quit", "Shutdown any existing server and terminate the application.", 0, Cmd_Quit );
	PlRegisterConsoleCommand( "exit", "Shutdown any existing server and terminate the application.", 0, Cmd_Quit );
	PlRegisterConsoleCommand( "version", "Prints out the current engine version.", 0, Cmd_Version );
	PlRegisterConsoleCommand( "clear", "Clear the console buffer.", 0, clear_console_command );

	if ( !isDedicated )
		ss_acl_console_register_cl_commands_();
}

void ss_acl_console_register_variables_( bool isDedicated )
{
	// server
	PlRegisterConsoleVariable( "server/name", "Name to use for the server.", "unnamed", PL_VAR_STRING, NULL, NULL, false );
	PlRegisterConsoleVariable( "server/password", "Password to access server functions.", "", PL_VAR_STRING, NULL, NULL, false );

	// Client variables
	if ( !isDedicated )
		ss_acl_console_register_cl_variables_();
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

	char *buf = PL_NEW_( char, length );
	vsnprintf( buf, length, message, args );

	va_end( args );

	PlLogMessage( logLevels[ level ], buf );

	PL_DELETE( buf );
}

/**
 * Set the console up.
 */
void ss_acl_initialize_console_( void )
{
	PlSetConsoleOutputCallback( output_callback );

	logLevels[ APE_LOG_ERROR ] = PlAddLogLevel( "yin/error", PL_COLOUR_RED, true );
	logLevels[ APE_LOG_WARNING ] = PlAddLogLevel( "yin/warning", PL_COLOUR_YELLOW, true );
	logLevels[ APE_LOG_INFORMATION ] = PlAddLogLevel( "yin", PL_COLOUR_WHITE, true );
	logLevels[ ACL_LOG_DEBUG ] = PlAddLogLevel( "yin/debug", PL_COLOUR_ORCHID,
#if !defined( NDEBUG )
	                                            true
#else
	                                            false
#endif
	);

	logLevels[ APE_LOG_CLIENT_ERROR ] = PlAddLogLevel( "yin/client/error", PL_COLOUR_RED, true );
	logLevels[ APE_LOG_CLIENT_WARNING ] = PlAddLogLevel( "yin/client/warning", PL_COLOUR_YELLOW, true );
	logLevels[ APE_LOG_CLIENT_INFORMATION ] = PlAddLogLevel( "yin/client", PL_COLOUR_WHITE, true );

	logLevels[ APE_LOG_SERVER_ERROR ] = PlAddLogLevel( "yin/server/error", PL_COLOUR_RED, true );
	logLevels[ APE_LOG_SERVER_WARNING ] = PlAddLogLevel( "yin/server/warning", PL_COLOUR_YELLOW, true );
	logLevels[ APE_LOG_SERVER_INFORMATION ] = PlAddLogLevel( "yin/server", PL_COLOUR_WHITE, true );
}

void ss_acl_shutdown_console_( void )
{
	clear_output_buffer();
}
