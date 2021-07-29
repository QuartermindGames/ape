/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"

#include "client/renderer/renderer.h"
#include "client/renderer/font.h"

static bool isConsoleOpen = false;

static unsigned int scrollPos = 0;

#define CON_TEXT_COLOUR PLColourRGB( 0, 255, 0 )

/* console buffer methods */

#define CON_BUFFER_MAX_LENGTH 256
#define CON_BUFFER_MAX_LINES  4096

static char         inputBuffer[ CON_BUFFER_MAX_LENGTH ] = { '\0' };
static unsigned int curInputBufferLength                 = 0;

static struct ConBuffer
{
	struct
	{
		char     buffer[ CON_BUFFER_MAX_LENGTH ];
		PLColour colour;
	} lines[ CON_BUFFER_MAX_LINES ];
	unsigned int numLines;
} outputBuffer = {
        .numLines = 0,
};
static void ClearBuffer( void ) { outputBuffer.numLines = 0; }
static void OutputCallback( int level, const char *msg )
{
	/*msg += 20;*/
	size_t l = strlen( msg );
	if ( l >= CON_BUFFER_MAX_LENGTH )
	{
		PrintWarn( "Attempting to push message to console with an unexpected length!\n" );
		l = CON_BUFFER_MAX_LENGTH - 2;
	}

	if ( outputBuffer.numLines >= CON_BUFFER_MAX_LINES )
	{
#define CON_JUMP 256
		memmove( outputBuffer.lines, &outputBuffer.lines[ CON_JUMP ], CON_BUFFER_MAX_LINES - CON_JUMP );
		outputBuffer.numLines -= CON_JUMP;
	}

	strncpy( outputBuffer.lines[ outputBuffer.numLines ].buffer, msg, l );
	outputBuffer.lines[ outputBuffer.numLines ].buffer[ l ] = '\0';

	PLColour lineColour;
	if ( level == LOG_LEVEL_WARN )
		lineColour = PL_COLOUR_RED;
	else if ( level == LOG_LEVEL_INFO )
		lineColour = CON_TEXT_COLOUR;
	else
		lineColour = PLColourRGB( 200, 200, 200 );

	outputBuffer.lines[ outputBuffer.numLines ].colour = lineColour;
	outputBuffer.numLines++;
}

/* CONSOLE COMMANDS */

#define CMD_CALLBACK( NAME ) static void Cmd_##NAME( unsigned int argc, char **argv )

CMD_CALLBACK( ClearConsole )
{
	u_unused( argc );
	u_unused( argv );
	ClearBuffer();
}

static void ToggleConsole();
CMD_CALLBACK( ToggleConsole )
{
	u_unused( argc );
	u_unused( argv );
	ToggleConsole();
}

CMD_CALLBACK( Quit )
{
	u_unused( argc );
	u_unused( argv );
	Engine_Shutdown();
}

/**
 * Pipes a command to either the shell or command.
 */
CMD_CALLBACK( OSCommand )
{
	if ( argc == 1 )
	{
		PrintWarn( "Usage: oscmd echo \"Hello World!\"\n" );
		return;
	}

	if ( system( argv[ 1 ] ) == -1 )
		PrintWarn( "Failed to issue command, an error occurred!\n" );
}

CMD_CALLBACK( Version )
{
	u_unused( argc );
	u_unused( argv );
	Print( "Version: v" ENGINE_VERSION_STR " [" GIT_BRANCH "." GIT_COMMIT_COUNT "]\n" );
}

/*------------------------------------------------------------------*/

#include "common/node.h"

#define USER_CONFIG "user" NL_DEFAULT_EXTENSION
static char configPath[ PL_SYSTEM_MAX_PATH ];

static void SaveUserConfig( void );
static void LoadUserConfig( void )
{
	NLNode *root = NL_LoadFile( configPath, "config" );
	if ( root == NULL )
	{
		Print( "No existing user config, generating default.\n" );
		SaveUserConfig();
		return;
	}

#if !defined( NDEBUG )
	//NL_PrintNodeTree( root, 0 );
#endif

	/* now iterate through the list and update all our children */
	NLNode *child = NL_GetFirstChild( root );
	while ( child != NULL )
	{
		const char *cvarName = NL_GetName( child );
		char cvarValue[ PL_SYSTEM_MAX_PATH ];
		if ( NL_GetStr( child, cvarValue, sizeof( cvarValue ) ) == NL_ERROR_SUCCESS )
			PlSetConsoleVariableByName( cvarName, cvarValue );
		else
			PrintWarn( "Failed to fetch value: %s\nPossibly invalid type?", cvarName );

		child = NL_GetNextChild( child );
	}

	Print( "User config loaded.\n" );
}

static void SaveUserConfig( void )
{
	PLConsoleVariable **cvars;
	size_t              numVars;
	PlGetConsoleVariables( &cvars, &numVars );

	NLNode *root = NL_PushBackObj( NULL, "config" );
	for ( unsigned int i = 0; i < numVars; ++i )
	{
		/* don't bother storing it if it matches the default */
		if ( strcmp( cvars[ i ]->value, cvars[ i ]->default_value ) == 0 )
			continue;

		switch ( cvars[ i ]->type )
		{
			case pl_float_var:
				NL_PushBackF32( root, cvars[ i ]->var, cvars[ i ]->f_value );
				break;
			case pl_int_var:
				NL_PushBackI32( root, cvars[ i ]->var, cvars[ i ]->i_value );
				break;
			case pl_bool_var:
				NL_PushBackBool( root, cvars[ i ]->var, cvars[ i ]->b_value );
				break;
			default:
				NL_PushBackStr( root, cvars[ i ]->var, cvars[ i ]->s_value );
				break;
		}
	}

#if !defined( NDEBUG )
	//NL_PrintNodeTree( root, 0 );
#endif

	char path[ PL_SYSTEM_MAX_PATH ];
	snprintf( path, sizeof( path ), "%s%s", ComFS_GetDataDirectory(), USER_CONFIG );
	DebugMsg( "Saving user config: \"%s\"\n", path );
	NL_WriteFile( path, root, NL_FILE_ASCII );
	NL_DestroyNode( root );

	Print( "User config saved.\n" );
}

/**
 * Set the console up.
 */
void Con_Initialize( void )
{
	Print( "Initializing console/config\n" );

	PlSetConsoleOutputCallback( OutputCallback );

	PlRegisterConsoleCommand( "quit", Cmd_Quit, "Shutdown any existing server and terminate the application." );
	PlRegisterConsoleCommand( "exit", Cmd_Quit, "Shutdown any existing server and terminate the application." );
	PlRegisterConsoleCommand( "oscmd", Cmd_OSCommand, "Pipes the given command to the host platform." );
	PlRegisterConsoleCommand( "version", Cmd_Version, "Prints out the current engine version." );

	/* debugging */
	PlRegisterConsoleVariable( "debug.overlay", "1", pl_int_var, NULL, "Enable/disable debug overlays." );
	PlRegisterConsoleVariable( "debug.profilerFrequency", "16", pl_int_var, NULL, "Set frequency at which profile graph updates." );

	PlRegisterConsoleVariable( "game.playerName", "unnamed", pl_string_var, NULL, "Set the name of the local player." );

	PlRegisterConsoleVariable( "console.alpha", "128", pl_int_var, NULL, "Level of transparency to use for the console background." );
	PlRegisterConsoleVariable( "console.height", "512", pl_int_var, NULL, "Set the height of the console." );
	PlRegisterConsoleCommand( "console.clear", Cmd_ClearConsole, "Clear the console buffer." );
	PlRegisterConsoleCommand( "console.toggle", Cmd_ToggleConsole, "Toggle the console." );

	/* networking */
	PlRegisterConsoleVariable( "net.serverName", "unnamed", pl_string_var, NULL, "Name to use for the server." );
	PlRegisterConsoleVariable( "net.password", "", pl_string_var, NULL, "Password to access server functions." );
	PlRegisterConsoleCommand( "connect", NULL, "Connect to the specified server." );
	PlRegisterConsoleCommand( "reconnect", NULL, "Reconnect to the current server." );
	PlRegisterConsoleCommand( "disconnect", NULL, "Disconnect from the current server." );

	/* rendering */
	PlRegisterConsoleVariable( "graphics.fxaa", "1", pl_bool_var, NULL, "Enable FXAA anti-aliasing." );
	PlRegisterConsoleVariable( "graphics.superSampling", "1", pl_int_var, NULL, "Resolution multiplier. "
	                                                                            "Anything higher than 1 essentially enables supersampling." );
	PlRegisterConsoleVariable( "graphics.wireframe", "0", pl_bool_var, NULL, "Enable wireframe mode." );

	PlRegisterConsoleVariable( "world.drawSectorVolumes", "false", pl_bool_var, NULL, "Toggle rendering of sector volumes." );
	PlRegisterConsoleVariable( "world.drawSectors", "true", pl_bool_var, NULL, "Toggle rendering of sectors." );
	PlRegisterConsoleVariable( "world.drawSubMeshes", "true", pl_bool_var, NULL, "Toggle rendering of sub-meshes within sectors." );
	PlRegisterConsoleVariable( "world.forceSimple", "false", pl_bool_var, NULL, "Force simple render pass of world." );

	// Figure out where to load/store the config
	const char *p = PlGetApplicationDataDirectory( ENGINE_APP_NAME, configPath, sizeof( configPath ) - ( strlen( USER_CONFIG ) + 1 ) );
	if ( p == NULL )
	{
		PrintWarn( "Failed to fetch application data directory, config may not be saved upon closing!\n" );
		snprintf( configPath, sizeof( configPath ), "./%s", USER_CONFIG );
	}
	else
	{
		if ( !PlCreateDirectory( p ) )
			PrintWarn( "Failed to create application data directory: %s\n", p );

		p = &p[ strlen( p ) - 1 ];
		if ( *p == '\\' || *p == '/' )
			strcat( configPath, USER_CONFIG );
		else
			strcat( configPath, "/" USER_CONFIG );
	}

	Print( "Config: %s\n", configPath );

	LoadUserConfig();
}

void Con_Shutdown( void )
{
	ClearBuffer();
	SaveUserConfig();
}

/**
 * Toggle the console state.
 */
static void ToggleConsole( void )
{
	isConsoleOpen = !isConsoleOpen;
}

static void ScrollForward( void )
{
	scrollPos++;
	if ( scrollPos > outputBuffer.numLines - 1 )
		scrollPos = outputBuffer.numLines - 1;
}

static void ScrollBackward( void )
{
	if ( scrollPos == 0 )
		return;

	scrollPos--;
}

/**
 * Returns the current console state, e.g. is it open?
 */
bool Con_GetState( void )
{
	return isConsoleOpen;
}

bool Con_HandleTextEvent( const char *key )
{
	// todo y3: allow this key to be customised
	if ( !Con_GetState() || *key == '`' || *key == '~' )
		return false;

	/* check length before appending so we can ensure
     * it's always null terminated */
	if ( curInputBufferLength + 1 >= CON_BUFFER_MAX_LENGTH )
		return true;

	inputBuffer[ curInputBufferLength++ ] = *key;
	inputBuffer[ curInputBufferLength ]   = '\0';

	return true;
}

bool Con_HandleKeyboardEvent( int key, unsigned int keyState )
{
	if ( keyState == INPUT_STATE_PRESSED && ( key == '`' || key == '~' ) )
	{
		ToggleConsole();
		return true;
	}

	/* only do anything if the console is open */
	if ( !Con_GetState() || keyState != INPUT_STATE_PRESSED && keyState != INPUT_STATE_DOWN )
		return false;

	switch ( key )
	{
		default:
			break;

		case KEY_PAGEUP:
			ScrollForward();
			return true;
		case KEY_PAGEDOWN:
			ScrollBackward();
			return true;

		case KEY_ENTER:
			if ( inputBuffer[ 0 ] != '\0' )
			{
				PlParseConsoleString( inputBuffer );
				inputBuffer[ 0 ]     = '\0';
				curInputBufferLength = 0;
			}
			return true;
		case KEY_BACKSPACE:
			if ( curInputBufferLength > 0 )
				inputBuffer[ --curInputBufferLength ] = '\0';

			return true;
		case KEY_TAB:
		{ /* autocompletion */
			unsigned int numOptions;
			const char **list = PlAutocompleteConsoleString( inputBuffer, &numOptions );
			if ( numOptions == 0 )
			{
				Print( "No matches found\n" );
				return true;
			}

			/* print out all the options */
			for ( unsigned int i = 0; i < numOptions; ++i )
				Print( " %s\n", list[ i ] );

			/* update to match the first result */
			snprintf( inputBuffer, sizeof( inputBuffer ), "%s", list[ 0 ] );
			curInputBufferLength = strlen( list[ 0 ] );
			return true;
		}
	}

	return false;
}

static void DrawInput( const PLGViewport *viewport )
{
	if ( !Con_GetState() )
		return;

	PlgSetTexture( NULL, 0 );

    BitmapFont *font = Font_GetDefault();
	PlgDrawRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), 0.0f, ( float ) viewport->h - font->ch, ( float ) viewport->w, font->ch, PLColourRGB( 0, 0, 0 ) );

	/* draw input field */
	Font_BeginDraw( font );

	Font_AddBitmapCharacterToPass( font, 1.0f, ( float ) viewport->h - font->ch, 1.0f, CON_TEXT_COLOUR, '>' );
    Font_AddBitmapCharacterToPass( font, ( float ) ( font->cw + ( font->cw * curInputBufferLength ) ), ( float ) viewport->h - font->ch, 1.0f, CON_TEXT_COLOUR, '_' );

	Font_AddBitmapStringToPass( Font_GetDefault(), font->cw, ( float ) viewport->h - font->ch, 1.0f, CON_TEXT_COLOUR, inputBuffer, curInputBufferLength );

	Font_Draw( font );
}

/**
 * Draw the console panel.
 */
void Con_Draw( const PLGViewport *viewport )
{
	if ( !Con_GetState() )
		return;

	CVar( "console.alpha", alpha );

	PlgSetBlendMode( PLG_BLEND_DEFAULT );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_VERTEX ] );

#define CON_SIDE_COLOUR      PLColourRGB( 128, 128, 128 )
#define CON_BACK_COLOUR      PLColour( 0, 0, 0, alpha->i_value )
#define CON_INDICATOR_COLOUR PLColourRGB( 255, 255, 255 )

	float consoleHeight = ( float ) ( viewport->h - 12 );

	PlgDrawRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), 0.0f, 0.0f, ( float ) viewport->w, consoleHeight, CON_BACK_COLOUR );
	PlgDrawRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), 0.0f, 0.0f, 8, consoleHeight, CON_SIDE_COLOUR );

	/* todo: update viewport in platform lib to console dimensions so we don't draw outside
	 *       the console space. */

	if ( outputBuffer.numLines > 0 )
	{
		/* draw the indicator at the side of the console */
		float cH = ( outputBuffer.numLines / consoleHeight ) + 1.0f;
		float cY = consoleHeight - ( ( outputBuffer.numLines / consoleHeight ) + scrollPos ) - cH;
		PlgDrawRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), 2.0f, cY, 8.0f, cH, CON_INDICATOR_COLOUR );

		BitmapFont *consoleFont = Font_GetDefault();
		Font_BeginDraw( consoleFont );

		float y = consoleHeight - 20.0f;
		for ( unsigned int i = ( outputBuffer.numLines - 1 ) - scrollPos; i > 0; --i )
		{
			/* draw the line we're currently at */
			Font_AddBitmapStringToPass( consoleFont, 12.0f, y, 1.0f, outputBuffer.lines[ i ].colour, outputBuffer.lines[ i ].buffer, strlen( outputBuffer.lines[ i ].buffer ) );

			/* now decrement our y pos for as many new lines there were */
			if ( i > 0 )
			{
				unsigned int nl = pl_strncnt( outputBuffer.lines[ i - 1 ].buffer, '\n', CON_BUFFER_MAX_LENGTH );
				for ( unsigned int j = 0; j < nl; ++j )
					y -= consoleFont->ch;
			}

			/* and make sure we don't go off screen */
			if ( y <= -consoleFont->ch )
				break;
		}

		Font_Draw( consoleFont );
	}

	DrawInput( viewport );

	PlPopMatrix();

	PlgSetBlendMode( PLG_BLEND_DISABLE );
}
