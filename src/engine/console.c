/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "renderer/renderer.h"
#include "renderer/font.h"

static bool isConsoleOpen = false;

static float consoleHeight = 0.0f;
static unsigned int scrollPos = 0;

static Material *backgroundMaterial = NULL;

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
    size_t l = strlen( msg );
    if ( l >= CON_BUFFER_MAX_LENGTH ) {
        PrintWarn( "Attempting to push message to console with an unexpected length!\n" );
        l = CON_BUFFER_MAX_LENGTH - 2;
    }

    strncpy( outputBuffer.lines[ outputBuffer.numLines ].buffer, msg, l );
    outputBuffer.lines[ outputBuffer.numLines ].buffer[ l ] = '\0';

	PLColour lineColour;
	switch( level ) {
		default:
			lineColour = PLColourRGB( 200, 200, 200 );
			break;
		case LOG_LEVEL_ERROR:
			lineColour = PL_COLOUR_RED;
			break;
		case LOG_LEVEL_WARN:
			lineColour = PL_COLOUR_ORANGE;
			break;
		case LOG_LEVEL_INFO:
			lineColour = CON_TEXT_COLOUR;
			break;
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

static void Con_UpdateBackground( const PLConsoleVariable *var ) {
	RM_DestroyMaterial( backgroundMaterial, false );
	backgroundMaterial = RM_CacheMaterial( var->s_value, CACHE_GROUP_STATIC, false );
	if ( backgroundMaterial == NULL ) {
		PrintMsg( "Please provide a valid background path!\n" );
	}
}

/*------------------------------------------------------------------*/

/**
 * Set the console up.
 */
void Con_Initialize( void ) {
	plSetConsoleOutputCallback( Con_OutputCallback );

	plRegisterConsoleVariable( "player.name", "unnamed", pl_string_var, NULL, "Set the name of the local player." );

	plRegisterConsoleVariable( "map.sky.material", "materials/sky/cloudlayer00.mat", pl_string_var, NULL, "Sets the sky material." );

	plRegisterConsoleVariable( "console.background", "", pl_string_var, Con_UpdateBackground, "Background to use for the console." );
	plRegisterConsoleVariable( "console.alpha", "128", pl_int_var, NULL, "Level of transparency to use for the console background." );
	plRegisterConsoleVariable( "console.height", "512", pl_int_var, NULL, "Set the height of the console." );

	plRegisterConsoleCommand( "console.clear", Cmd_ClearConsole, "Clear the console buffer." );
	plRegisterConsoleCommand( "console.toggle", Cmd_ToggleConsole, "Toggle the console." );
}

void Con_Shutdown( void ) {
	Con_ClearBuffer();
}

static void Con_Animate( void *userData, double delta ) {
	u_unused( delta );
	u_unused( userData );

#define SPEED 16.0f
	if ( isConsoleOpen && ( consoleHeight < 512.0f )  ) {
		consoleHeight += SPEED;
		Sch_PushTask( "conanim", Con_Animate, NULL, 1 );
	} else if ( !isConsoleOpen && consoleHeight > 0.0f ) {
		consoleHeight -= SPEED;
		Sch_PushTask( "conanim", Con_Animate, NULL, 1 );
	}

	if ( consoleHeight > 512.0f ) { consoleHeight = 512.0f; }
	else if ( consoleHeight < 0.0f ) { consoleHeight = 0.0f; }
}

/**
 * Toggle the console state.
 */
void Con_Toggle( void ) {
	static unsigned int toggleTime = 0;
	if ( toggleTime > Engine_GetNumTicks() ) {
		return;
	}

	isConsoleOpen = !isConsoleOpen;

	Sch_PushTask( "conanim", Con_Animate, NULL, 2 );

	toggleTime = Engine_GetNumTicks() + 16;
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

bool Con_HandleKeyboardEvent( int key, bool isDown ) {
	/* only do anything if the console is open */
    if ( !Con_GetState() ) {
		return false;
	}

	if ( !isDown ) {
		return true;
	}

	if ( key == KEY_ENTER ) {
		if ( inputBuffer[ 0 ] != '\0' ) {
			plParseConsoleString( inputBuffer );
			inputBuffer[ 0 ] = '\0';
			curInputBufferLength = 0;
		}
		return true;
	}

	if ( key == KEY_BACKSPACE ) {
		if ( curInputBufferLength > 0 ) {
			inputBuffer[ --curInputBufferLength ] = '\0';
		}
		return true;
	}

	/* autocompletion */
	if ( key == KEY_TAB ) {
		unsigned int numOptions;
		const char **list = plAutocompleteConsoleString( inputBuffer, &numOptions );
		if ( numOptions == 0 ) {
			PrintMsg( "No matches found\n" );
			return true;
		}

		/* print out all the options */
		for ( unsigned int i = 0; i < numOptions; ++i ) {
			PrintMsg( " %s\n", list[ i ] );
		}

        /* update to match the first result */
        snprintf( inputBuffer, sizeof( inputBuffer ), "%s", list[ 0 ] );
		curInputBufferLength = strlen( list[ 0 ] );
		return true;
	}

	/* check length before appending so we can ensure
	 * it's always null terminated */
	if ( curInputBufferLength + 1 >= CON_BUFFER_MAX_LENGTH ) {
		return true;
	}
	inputBuffer[ curInputBufferLength++ ] = ( char ) key;
	inputBuffer[ curInputBufferLength ] = '\0';
}

/**
 * Returns the current console state, e.g. is it open?
 */
bool Con_GetState( void ) {
	return isConsoleOpen;
}

/**
 * Draw the console panel.
 */
void Con_Draw( const PLViewport *viewport ) {
	static float oldConsoleHeight = 0.0f;

	if ( consoleHeight <= 0.0f ) {
		return;
	}

	static PLConsoleVariable *alpha = NULL;
	if ( alpha == NULL ) { alpha = plGetConsoleVariable( "console.alpha" ); }

	plSetBlendMode( PL_BLEND_DEFAULT );

	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();

	plLoadIdentityMatrix();

	float w = viewport->w - 4.0f;

	plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT_VERTEX ] );

#define CON_SIDE_COLOUR         PLColourRGB( 128, 128, 128 )
#define CON_BACK_COLOUR         PLColour( 0, 0, 0, alpha->i_value )
#define CON_INDICATOR_COLOUR    PLColourRGB( 255, 255, 255 )

	plDrawRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 2.0f, 2.0f, w, consoleHeight, CON_BACK_COLOUR );
	plDrawRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 2.0f, 2.0f, 8, consoleHeight, CON_SIDE_COLOUR );
	plDrawRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 0.0f, viewport->h - 12.0f, viewport->w, 12.0f, CON_BACK_COLOUR );

	/* todo: update viewport in platform lib to console dimensions so we don't draw outside
	 *       the console space. */

	if ( outputBuffer.numLines > 0 ) {
		/* draw the indicator at the side of the console */
		float cH = ( outputBuffer.numLines / consoleHeight ) + 1.0f;
		float cY = consoleHeight - ( ( outputBuffer.numLines / consoleHeight ) + scrollPos ) - cH;
		plDrawRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 2.0f, cY, 8.0f, cH, CON_INDICATOR_COLOUR );

		plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT ] );

		float y = consoleHeight - 20.0f;
		for ( unsigned int i = ( outputBuffer.numLines - 1 ) - scrollPos; i > 0; --i ) {
			/* draw the line we're currently at */
			Font_DrawBitmapString( 12.0f, y, 1.0f, 1.0f, outputBuffer.lines[ i ].colour, outputBuffer.lines[ i ].buffer, true );

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

	/* draw input field */
	Font_DrawBitmapCharacter( 1.0f, viewport->h - 12.0f, 1.0f, CON_TEXT_COLOUR, '>' );
	Font_DrawBitmapString( 4.0f, viewport->h - 12.0f, 1.0f, 1.0f, CON_TEXT_COLOUR, inputBuffer, 0 );

	plPopMatrix();

	plSetBlendMode( PL_BLEND_DISABLE );
}
