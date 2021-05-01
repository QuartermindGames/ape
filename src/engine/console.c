/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "renderer/renderer.h"
#include "renderer/font.h"

static bool isConsoleOpen = false;

static unsigned int scrollPos = 0;

#define CON_TEXT_COLOUR PLColourRGB( 0, 255, 0 )

/* console buffer methods */

#define CON_BUFFER_MAX_LENGTH 256
#define CON_BUFFER_MAX_LINES 4096

static char inputBuffer[ CON_BUFFER_MAX_LENGTH ] = { '\0' };
static unsigned int curInputBufferLength = 0;

static struct ConBuffer {
	struct {
		char buffer[ CON_BUFFER_MAX_LENGTH ];
		PLColour colour;
	} lines[ CON_BUFFER_MAX_LINES ];
	unsigned int numLines;
} outputBuffer = {
        .numLines = 0,
};
static void Con_ClearBuffer( void ) { outputBuffer.numLines = 0; }
static void Con_OutputCallback( int level, const char *msg ) {
	/*msg += 20;*/
	size_t l = strlen( msg );
	if ( l >= CON_BUFFER_MAX_LENGTH ) {
		PrintWarn( "Attempting to push message to console with an unexpected length!\n" );
		l = CON_BUFFER_MAX_LENGTH - 2;
	}

	strncpy( outputBuffer.lines[ outputBuffer.numLines ].buffer, msg, l );
	outputBuffer.lines[ outputBuffer.numLines ].buffer[ l ] = '\0';

	PLColour lineColour;
	if ( level == LOG_LEVEL_ERROR ) {
        lineColour = PL_COLOUR_RED;
	} else if ( level == LOG_LEVEL_WARN ) {
        lineColour = PL_COLOUR_ORANGE;
	} else if ( level == LOG_LEVEL_INFO ) {
        lineColour = CON_TEXT_COLOUR;
	} else {
		lineColour = PLColourRGB( 200, 200, 200 );
	}

	outputBuffer.lines[ outputBuffer.numLines ].colour = lineColour;

	/* this is when we do what is probably going to be,
     * a dumb and expensive operation... */
	outputBuffer.numLines++;
	if ( outputBuffer.numLines >= CON_BUFFER_MAX_LINES ) {
		//memmove_s()
	}
}

/* CONSOLE COMMANDS */

#define CMD_CALLBACK( NAME ) static void Cmd_##NAME( unsigned int argc, char **argv )

CMD_CALLBACK( ClearConsole ) {
	u_unused( argc );
	u_unused( argv );
	Con_ClearBuffer();
}

CMD_CALLBACK( ToggleConsole ) {
	u_unused( argc );
	u_unused( argv );
	Con_Toggle();
}

CMD_CALLBACK( Quit ) {
	u_unused( argc );
	u_unused( argv );
	Engine_Shutdown();
}

/*------------------------------------------------------------------*/

PLConsoleVariable *gVarGraphicsFXAA;
PLConsoleVariable *gVarGraphicsSupersampling;

#include "common/node.h"

#define USER_CONFIG "user" NL_DEFAULT_EXTENSION

static void LoadUserConfig( void ) {
	DebugMsg( "Loading user config: \"%s\"\n", USER_CONFIG );

    NLNode *root = NL_LoadFile( USER_CONFIG, "config" );
	if ( root == NULL ) {
		PrintWarn( "Failed to load user config: %s!\n", NL_GetErrorMessage() );
		return;
	}

#if !defined( NDEBUG )
    NL_PrintNodeTree( root, 0 );
#endif

	/* now iterate through the list and update all our children */
	NLNode *child = NL_GetFirstChild( root );
	while ( child != NULL ) {
		const char *cvarName = NL_GetName( child );
		PLConsoleVariable *cvar = PlGetConsoleVariable( cvarName );
		if ( cvar != NULL ) {
			PlSetConsoleVariable( cvar, NL_GetString( child ) );
		} else {
            PrintWarn( "Failed to find console variable, \"%s\"!\n", cvarName );
		}

		child = NL_GetNextChild( child );
	}

	Print( "User config loaded.\n" );
}

static void SaveUserConfig( void ) {
	PLConsoleVariable **cvars;
	size_t numVars;
	PlGetConsoleVariables( &cvars, &numVars );

    NLNode *root = NL_PushBackObj( NULL, "config" );
	for ( unsigned int i = 0; i < numVars; ++i ) {
		/* don't bother storing it if it matches the default */
		if ( strcmp( cvars[ i ]->value, cvars[ i ]->default_value ) == 0 ) {
			continue;
		}

		switch( cvars[ i ]->type ) {
			case pl_float_var:
				NL_PushBackFloat( root, cvars[ i ]->var, cvars[ i ]->f_value );
                break;
            case pl_int_var:
				NL_PushBackInt( root, cvars[ i ]->var, cvars[ i ]->i_value );
				break;
			case pl_bool_var:
				NL_PushBackBool( root, cvars[ i ]->var, cvars[ i ]->b_value );
				break;
			default:
				NL_PushBackString( root, cvars[ i ]->var, cvars[ i ]->s_value );
				break;
		}
	}

#if !defined( NDEBUG )
	NL_PrintNodeTree( root, 0 );
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
void Con_Initialize( void ) {
	PlSetConsoleOutputCallback( Con_OutputCallback );

	/* debugging */
	PlRegisterConsoleVariable( "debug.overlay", "1", pl_int_var, NULL, "Enable/disable debug overlays." );

	PlRegisterConsoleCommand( "quit", Cmd_Quit, "Shutdown any existing server and terminate the application." );
    PlRegisterConsoleCommand( "exit", Cmd_Quit, "Shutdown any existing server and terminate the application." );

    PlRegisterConsoleVariable( "game.projectPath", "scripts/project.node", pl_string_var, NULL, "Sets the default path to load a GS project." );
    PlRegisterConsoleVariable( "game.playerName", "unnamed", pl_string_var, NULL, "Set the name of the local player." );

    PlRegisterConsoleVariable( "console.alpha", "128", pl_int_var, NULL, "Level of transparency to use for the console background." );
    PlRegisterConsoleVariable( "console.height", "512", pl_int_var, NULL, "Set the height of the console." );
    PlRegisterConsoleCommand( "console.clear", Cmd_ClearConsole, "Clear the console buffer." );
    PlRegisterConsoleCommand( "console.toggle", Cmd_ToggleConsole, "Toggle the console." );

	/* networking */
    PlRegisterConsoleVariable( "net.serverName", "unnamed", pl_string_var, NULL, "Name to use for the server." );
    PlRegisterConsoleVariable( "net.password", "", pl_string_var, NULL, "Password to access server functions." );
    PlRegisterConsoleCommand( "net.connect", NULL, "Connect to the specified server." );
    PlRegisterConsoleCommand( "net.disconnect", NULL, "Disconnect from the current server." );

	/* rendering */
	gVarGraphicsFXAA = PlRegisterConsoleVariable( "graphics.fxaa", "1", pl_bool_var, NULL, "Enable FXAA anti-aliasing." );
	gVarGraphicsSupersampling = PlRegisterConsoleVariable( "graphics.supersampling", "1", pl_int_var, NULL, "Resolution multiplier. Anything higher than 1 essentially enables supersampling." );
    PlRegisterConsoleVariable( "graphics.wireframe", "0", pl_bool_var, NULL, "Enable wireframe mode." );

	LoadUserConfig();
}

void Con_Shutdown( void ) {
	Con_ClearBuffer();
    SaveUserConfig();
}

/**
 * Toggle the console state.
 */
void Con_Toggle( void ) {
	isConsoleOpen = !isConsoleOpen;
}

void Con_ScrollForward( void ) {
	scrollPos++;
	if ( scrollPos > outputBuffer.numLines - 1 ) {
		scrollPos = outputBuffer.numLines - 1;
	}
}

void Con_ScrollBackward( void ) {
	if ( scrollPos == 0 ) {
		return;
	}
	scrollPos--;
}

/**
 * Returns the current console state, e.g. is it open?
 */
bool Con_GetState( void ) {
	return isConsoleOpen;
}

bool Con_HandleTextEvent( const char *key ) {
	if ( !Con_GetState() || *key == '`' || *key == '~' ) {
		return false;
	}

	/* check length before appending so we can ensure
     * it's always null terminated */
	if ( curInputBufferLength + 1 >= CON_BUFFER_MAX_LENGTH ) {
		return true;
	}

	inputBuffer[ curInputBufferLength++ ] = *key;
	inputBuffer[ curInputBufferLength ] = '\0';

	return true;
}

bool Con_HandleKeyboardEvent( int key, unsigned int keyState ) {
    if ( keyState == INPUT_STATE_DOWN && ( key == '`' || key == '~' ) ) {
        Con_Toggle();
		return true;
    }

	/* only do anything if the console is open */
	if ( !Con_GetState() || keyState != INPUT_STATE_DOWN && keyState != INPUT_STATE_PRESSING ) {
		return false;
	}

	switch ( key ) {
		default:
			break;

		case KEY_PAGEUP:
            Con_ScrollForward();
			return true;
		case KEY_PAGEDOWN:
            Con_ScrollBackward();
			return true;

		case KEY_ENTER:
			if ( inputBuffer[ 0 ] != '\0' ) {
				PlParseConsoleString( inputBuffer );
				inputBuffer[ 0 ] = '\0';
				curInputBufferLength = 0;
			}
			return true;
		case KEY_BACKSPACE:
			if ( curInputBufferLength > 0 ) {
				inputBuffer[ --curInputBufferLength ] = '\0';
			}
			return true;
		case KEY_TAB: { /* autocompletion */
			unsigned int numOptions;
			const char **list = PlAutocompleteConsoleString( inputBuffer, &numOptions );
			if ( numOptions == 0 ) {
				Print( "No matches found\n" );
				return true;
			}

			/* print out all the options */
			for ( unsigned int i = 0; i < numOptions; ++i ) {
				Print( " %s\n", list[ i ] );
			}

			/* update to match the first result */
			snprintf( inputBuffer, sizeof( inputBuffer ), "%s", list[ 0 ] );
			curInputBufferLength = strlen( list[ 0 ] );
			return true;
		}
	}

	return false;
}

static void Con_DrawInput( const PLGViewport *viewport ) {
	if ( !Con_GetState() ) {
		return;
	}

	PlgSetTexture( NULL, 0 );

    PlgDrawRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), 0.0f, ( float ) viewport->h - 12.0f, ( float ) viewport->w, 12.0f, PLColourRGB( 0, 0, 0 ) );

	/* draw input field */
	Font_DrawBitmapCharacter( Font_GetDefault(), 1.0f, ( float ) viewport->h - 12, 1.0f, CON_TEXT_COLOUR, '>' );
	Font_DrawBitmapCharacter( Font_GetDefault(), ( float ) ( 12 + ( 8 * curInputBufferLength ) ), ( float ) viewport->h - 12, 1.0f, CON_TEXT_COLOUR, '_' );
	Font_DrawBitmapString( Font_GetDefault(), 12.0f, ( float ) viewport->h - 12, 1.0f, 1.0f, CON_TEXT_COLOUR, inputBuffer, 0 );
}

/**
 * Draw the console panel.
 */
void Con_Draw( const PLGViewport *viewport ) {
	if ( !Con_GetState() ) {
		return;
	}

	CVar( "console.alpha", alpha );

	PlgSetBlendMode( PLG_BLEND_DEFAULT );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT_VERTEX ] );

#define CON_SIDE_COLOUR PLColourRGB( 128, 128, 128 )
#define CON_BACK_COLOUR PLColour( 0, 0, 0, alpha->i_value )
#define CON_INDICATOR_COLOUR PLColourRGB( 255, 255, 255 )

	float consoleHeight = ( float ) ( viewport->h - 12 );

	PlgDrawRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), 0.0f, 0.0f, ( float ) viewport->w, consoleHeight, CON_BACK_COLOUR );
	PlgDrawRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), 0.0f, 0.0f, 8, consoleHeight, CON_SIDE_COLOUR );

	/* todo: update viewport in platform lib to console dimensions so we don't draw outside
	 *       the console space. */

	if ( outputBuffer.numLines > 0 ) {
		/* draw the indicator at the side of the console */
		float cH = ( outputBuffer.numLines / consoleHeight ) + 1.0f;
		float cY = consoleHeight - ( ( outputBuffer.numLines / consoleHeight ) + scrollPos ) - cH;
		PlgDrawRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), 2.0f, cY, 8.0f, cH, CON_INDICATOR_COLOUR );

		float y = consoleHeight - 20.0f;
		for ( unsigned int i = ( outputBuffer.numLines - 1 ) - scrollPos; i > 0; --i ) {
			/* draw the line we're currently at */
			Font_DrawBitmapString( Font_GetDefault(), 12.0f, y, 1.0f, 1.0f, outputBuffer.lines[ i ].colour, outputBuffer.lines[ i ].buffer, true );

			/* now decrement our y pos for as many new lines there were */
			if ( i > 0 ) {
				unsigned int nl = pl_strncnt( outputBuffer.lines[ i - 1 ].buffer, '\n', CON_BUFFER_MAX_LENGTH );
				for ( unsigned int j = 0; j < nl; ++j ) {
					y -= 12.0f;
				}
			}

			/* and make sure we don't go off screen */
			if ( y <= -12.0f ) {
				break;
			}
		}
	}

	Con_DrawInput( viewport );

	PlPopMatrix();

	PlgSetBlendMode( PLG_BLEND_DISABLE );
}
