// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"

#include "renderer/renderer.h"
#include "gui/gui_private.h"
#include "client/audio/audio.h"
#include "world/world.h"
#include "ape_client_input.h"

static bool consoleIsOpen = false;
static bool drawShadow = false;

static int consoleAlpha = 200;

/****************************************
 * CONSOLE INPUT BUFFER
 ****************************************/

static char conInputBuffer[ CONSOLE_BUFFER_MAX_LENGTH ] = { '\0' };
static unsigned int conInputBufferLength = 0;

#define MAX_HISTORY_RESULTS 64
static char history[ MAX_HISTORY_RESULTS ][ 64 ] = { { '\0' } };
static unsigned int numHistoryItems = 0;
static unsigned int historySelection = 0;

/////////////////////////////////////////////////////////////////
// AUTOCOMPLETE

#define MAX_AUTOCOMPLETE_RESULTS 8
static const char *autoComplete[ MAX_AUTOCOMPLETE_RESULTS ] = { NULL };
static bool enableAutoCompleteList;
static unsigned int autoCompleteSelection = 0;

static void UpdateAutoCompleteResult( const char *input ) {
	// just clear it if an empty result is given
	if ( input == NULL || *input == '\0' ) {
		PL_ZERO( autoComplete, sizeof( const char * ) * MAX_AUTOCOMPLETE_RESULTS );
		return;
	}

	// fetch all matching results
	unsigned int numOptions;
	const char **list = PlAutocompleteConsoleString( input, &numOptions );
	if ( numOptions >= MAX_AUTOCOMPLETE_RESULTS ) {
		numOptions = MAX_AUTOCOMPLETE_RESULTS - 1;
	}

	// fill the list, leaving the last item null so we know where it ends
	for ( unsigned int i = 0; i < numOptions; ++i ) {
		autoComplete[ i ] = list[ i ];
	}
	autoComplete[ numOptions ] = NULL;

	autoCompleteSelection = 0;
}

/////////////////////////////////////////////////////////////////

bool apeHandleConsoleTextEvent_( const char *key ) {
	// todo y3: allow this key to be customised
	if ( !consoleIsOpen || *key == '`' || *key == '~' ) {
		return false;
	}

	/* check length before appending so we can ensure
     * it's always null terminated */
	if ( conInputBufferLength + 1 >= CONSOLE_BUFFER_MAX_LENGTH ) {
		return true;
	}

	conInputBuffer[ conInputBufferLength++ ] = *key;
	conInputBuffer[ conInputBufferLength ] = '\0';

	UpdateAutoCompleteResult( conInputBuffer );

	return true;
}

/****************************************
 * GENERAL INPUT
 ****************************************/

static void ToggleConsole( void ) {
	consoleIsOpen = !consoleIsOpen;

	// Release the mouse if the console is open
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value ) {
		apeShellInterface_GrabMouse( !consoleIsOpen );
	}
}

static void ToggleConsoleCommand( unsigned int argc, char **argv ) {
	( void ) ( argc );
	( void ) ( argv );
	ToggleConsole();
}

static void ScrollForward( ApeConsoleOutput *output ) {
	output->scrollPos++;
	if ( output->scrollPos > output->numLines - 1 ) {
		output->scrollPos = output->numLines - 1;
	}
}

static void ScrollBackward( ApeConsoleOutput *output ) {
	if ( output->scrollPos == 0 ) {
		return;
	}

	output->scrollPos--;
}

bool Client_Console_HandleMouseWheelEvent( float x, float y ) {
	if ( !apeIsConsoleOpen() ) {
		return false;
	}

	ApeConsoleOutput *output = apeGetConsoleOutput();
	if ( y > 0.0f ) {
		ScrollForward( output );
	} else if ( y < 0.0f ) {
		ScrollBackward( output );
	}

	return true;
}

static void ClearInputBuffer( void ) {
	memset( conInputBuffer, 0, sizeof( conInputBuffer ) );
	conInputBufferLength = 0;

	UpdateAutoCompleteResult( conInputBuffer );
}

bool Client_Console_HandleKeyboardEvent( int key, unsigned int keyState ) {
	if ( keyState == APE_INPUT_STATE_DOWN && ( key == '`' || key == '~' ) ) {
		ToggleConsole();
		return true;
	}

	/* only do anything if the console is open */
	if ( !consoleIsOpen ) {
		return false;
	}
	/* but we don't care about these... */
	if ( keyState != APE_INPUT_STATE_PRESSED && keyState != APE_INPUT_STATE_DOWN ) {
		return true;
	}

	ApeConsoleOutput *output = apeGetConsoleOutput();
	switch ( key ) {
		default:
			break;
		case KEY_PAGEUP:
			ScrollForward( output );
			break;
		case KEY_PAGEDOWN:
			ScrollBackward( output );
			break;
		case KEY_END: {
			output->scrollPos = 0;
			break;
		}
		case KEY_HOME: {
			output->scrollPos = output->numLines - 1;
			break;
		}

		case KEY_UP: {
			if ( autoComplete[ 0 ] == NULL ) {
				// in this case, cycle the history

				break;
			}

			unsigned int nextSlot = autoCompleteSelection + 1;
			if ( nextSlot >= MAX_AUTOCOMPLETE_RESULTS || autoComplete[ nextSlot ] == NULL ) {
				autoCompleteSelection = 0;
				break;
			}

			autoCompleteSelection++;
			break;
		}
		case KEY_DOWN: {
			if ( autoComplete[ 0 ] == NULL ) {
				break;
			}

			if ( autoCompleteSelection == 0 ) {
				autoCompleteSelection = MAX_AUTOCOMPLETE_RESULTS - 1;
				while ( autoComplete[ autoCompleteSelection ] == NULL ) { autoCompleteSelection--; }
				break;
			}

			autoCompleteSelection--;
			break;
		}

		case KEY_ENTER: {
			if ( autoComplete[ 0 ] != NULL && autoCompleteSelection > 0 ) {
				snprintf( conInputBuffer, sizeof( conInputBuffer ), "%s", autoComplete[ autoCompleteSelection ] );
				conInputBufferLength = strlen( autoComplete[ autoCompleteSelection ] );
				UpdateAutoCompleteResult( conInputBuffer );
				break;
			} else if ( conInputBuffer[ 0 ] != '\0' ) {
				PlParseConsoleString( conInputBuffer );
				ClearInputBuffer();
			}
			break;
		}
		case KEY_BACKSPACE: {
			if ( conInputBufferLength > 0 ) {
				conInputBuffer[ --conInputBufferLength ] = '\0';
			}

			UpdateAutoCompleteResult( conInputBuffer );
			break;
		}
		case KEY_TAB: { /* autocompletion */
			if ( *conInputBuffer == '\0' || autoComplete[ 0 ] == NULL )
				break;

			/* update to match the first result */
			snprintf( conInputBuffer, sizeof( conInputBuffer ), "%s", autoComplete[ autoCompleteSelection ] );
			conInputBufferLength = strlen( autoComplete[ autoCompleteSelection ] );

			UpdateAutoCompleteResult( conInputBuffer );
			break;
		}
	}

	return consoleIsOpen;
}

/****************************************
 * RENDERING
 ****************************************/

static void DrawInputField( const ApeViewport *viewport, GuiFont *font ) {
	const float ch = guiGetFontLineSpacing( font );
	float cw = guiGetCharacterPixelWidth( font, 1.0f, '>' );
	guiDrawFontCharacter( font, 1.0f, ( float ) viewport->height - ch, 1.0f, &PL_COLOUR_LIME, '>' );

	/* cursor blinker */
#define SPACER 4.0f
	static unsigned int v = 0;
	if ( v < apeGetNumTicks() ) {
		v = apeGetNumTicks() + 20;
	}

	float bufPixW;
	guiGetStringPixelSize( font, 1.0f, conInputBuffer, conInputBufferLength, &bufPixW, NULL );

	const float x = ( 1.0f + cw );

	// cursor
	char c = ( v > apeGetNumTicks() + 10 ) ? '_' : ' ';
	guiDrawFontCharacter( font, x + bufPixW, ( float ) viewport->height - ch, 1.0f, &PL_COLOUR_LIME, c );

	if ( autoComplete[ 0 ] != NULL ) {
		size_t autoCompleteLength = strlen( autoComplete[ 0 ] );
		guiDrawFontString( font, x + bufPixW, ( float ) viewport->height - ch, NULL, NULL, 1.0f, &PL_COLOUR_GREEN, autoComplete[ 0 ] + conInputBufferLength, autoCompleteLength - conInputBufferLength, false );
		if ( enableAutoCompleteList ) {
			unsigned int i = 1;
			while ( autoComplete[ i ] != NULL ) {
				autoCompleteLength = strlen( autoComplete[ i ] );
				guiDrawFontString( font, x, ( float ) viewport->height - ( ch * ( ( float ) i + 1 ) ), NULL, NULL, 1.0f, &PL_COLOUR_LIME, conInputBuffer, conInputBufferLength, false );
				guiDrawFontString( font, x + bufPixW, ( float ) viewport->height - ( ch * ( ( float ) i + 1 ) ), NULL, NULL, 1.0f, &PL_COLOUR_GREEN, autoComplete[ i ] + conInputBufferLength, autoCompleteLength - conInputBufferLength, false );
				++i;
			}
		}
	}

	guiDrawFontString( font, 1.0f + cw, ( float ) viewport->height - ch, NULL, NULL, 1.0f, &PL_COLOUR_LIME, conInputBuffer, conInputBufferLength, false );
}

bool apeIsConsoleOpen( void ) { return consoleIsOpen; }

static const float consoleScrollBarWidth = 8.0f;

/**
 * Draw the console panel.
 */
void apeDrawConsole_( const ApeViewport *viewport ) {
	if ( !apeIsConsoleOpen() ) {
		return;
	}

	GuiFont *font = guiGetDefaultFont( GUI_FONT_DEFAULT_SMALL );
	assert( font != NULL );
	if ( font == NULL ) {
		return;
	}

	PlgSetTexture( NULL, 0 );
	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

#define CON_SIDE_COLOUR      PLColourRGB( 128, 128, 128 )
#define CON_BACK_COLOUR      PLColour( 0, 0, 0, consoleAlpha )
#define CON_INDICATOR_COLOUR PL_COLOUR_DARK_BLUE
#define CON_INPUT_COLOUR     PLColour( 0, 0, 0, 255 )

	float lineSpacing = guiGetFontLineSpacing( font );
	float width = ( float ) viewport->width;
	float height = ( float ) viewport->height;
	float consoleHeight = height - lineSpacing;

	PlgSetBlendMode( PLG_BLEND_DEFAULT );

	PlgDrawRectangle( 0.0f, 0.0f, width, height - lineSpacing, CON_BACK_COLOUR );
	PlgDrawRectangle( 0.0f, height - lineSpacing, width, lineSpacing, CON_INPUT_COLOUR );
	PlgDrawRectangle( 0.0f, 0.0f, consoleScrollBarWidth, consoleHeight, CON_SIDE_COLOUR );

	PlgSetBlendMode( PLG_BLEND_DISABLE );

	const ApeConsoleOutput *output = apeGetConsoleOutput();
	if ( output->numLines > 0 ) {
		/* draw the indicator at the side of the console */
		float cH = ( ( ( lineSpacing * ( float ) output->numLines ) / consoleHeight ) + 1.0f );
		float cY = consoleHeight - ( ( ( float ) output->numLines / consoleHeight ) + ( float ) output->scrollPos ) - cH;
		PlgDrawRectangle( 0.0f, cY, 8.0f, cH, CON_INDICATOR_COLOUR );

		float y = consoleHeight - 20.0f;
		for ( unsigned int i = ( output->numLines - 1 ) - output->scrollPos; i > 0; --i ) {
			/* draw the line we're currently at */
			guiDrawFontString( font, 12.0f, y, NULL, NULL, 1.0f, &output->lines[ i ].colour, output->lines[ i ].buffer, strlen( output->lines[ i ].buffer ), drawShadow );

			y -= lineSpacing;
			if ( y < 0 ) {
				break;
			}
		}
	}

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );
	PlgSetTexture( NULL, 0 );

	guiDisplayFont( font );

	// auto-completion list
	if ( enableAutoCompleteList && ( autoComplete[ 0 ] != NULL ) ) {
		float autoCompleteHeight = 0.0f;
		float autoCompleteWidth = 0.0f;

		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

		// iterate over the options to determine height, width
		unsigned int i = 1;
		while ( autoComplete[ i ] != NULL ) {
			float w, h;
			guiGetStringPixelSize( font, 1.0f, autoComplete[ i ], strlen( autoComplete[ i ] ), &w, &h );
			if ( w > autoCompleteWidth ) { autoCompleteWidth = w; }
			autoCompleteHeight += h;
			PlgDrawRectangle( consoleScrollBarWidth, ( height - ( float ) h ) - autoCompleteHeight, w, h, ( autoCompleteSelection == i ) ? CON_INDICATOR_COLOUR : CON_INPUT_COLOUR );
			++i;
		}
	}

	DrawInputField( viewport, font );

	/* draw version info */
	GuiFont *tinyFont = guiGetDefaultFont( GUI_FONT_DEFAULT_TINY );
	if ( tinyFont != NULL ) {
		static char buf[] = "v" ENGINE_VERSION_STR " [" GIT_BRANCH "." GIT_COMMIT_COUNT "]";

		float strW, strH;
		guiGetStringPixelSize( tinyFont, 1.0f, buf, sizeof( buf ), &strW, &strH );

		float x = width - strW - 2.0f;
		float y = height - strH - 2.0f;
		guiDrawFontString( tinyFont, x, y, NULL, NULL, 1.0f, &PLColourRGB( 0, 255, 0 ), buf, sizeof( buf ), false );

		guiDisplayFont( tinyFont );
	}

	PlPopMatrix();
}

/****************************************
 * CLIENT CONSOLE INIT
 ****************************************/

static void input_mlook_command( const PLConsoleVariable *consoleVariable )
{
	if ( !consoleVariable->b_value )
		return;

	acl_input_center_mouse();
}

void apeRegisterClientConsoleCommands_( void ) {
	PlRegisterConsoleCommand( "ape/console/toggle", "Toggle the console.", 0, ToggleConsoleCommand );

	//PlRegisterConsoleCommand( "ape/connect", NULL, "Connect to the specified server." );
	//PlRegisterConsoleCommand( "ape/reconnect", NULL, "Reconnect to the current server." );
	//PlRegisterConsoleCommand( "ape/disconnect", NULL, "Disconnect from the current server." );
}

void apeRegisterRendererConsoleVariables_( void );
void apeRegisterClientConsoleVariables_( void ) {
	PlRegisterConsoleVariable( "ape/client/name", "Set the name of the local player.", "unnamed", PL_VAR_STRING, NULL, NULL, true );

	PlRegisterConsoleVariable( "input/mlook", "Toggle mouse look. If enabled, mouse is captured.", "0", PL_VAR_BOOL, NULL, input_mlook_command, true );

	PlRegisterConsoleVariable( "debug/overlay", "Enable/disable debug overlays.", "0", PL_VAR_I32, NULL, NULL, false );
	PlRegisterConsoleVariable( "debug/profilerFrequency", "Set frequency at which profile graph updates.", "16", PL_VAR_I32, NULL, NULL, false );

	PlRegisterConsoleVariable( "ape/console/autoList", "Enable/disable list of options that are presented for auto-completion.", "true", PL_VAR_BOOL, &enableAutoCompleteList, NULL, true );
	PlRegisterConsoleVariable( "ape/console/alpha", "Level of transparency to use for the console background.", "200", PL_VAR_I32, &consoleAlpha, NULL, true );
	PlRegisterConsoleVariable( "ape/console/drawShadow", "Shadow for text, which will improve legibility. "
	                                                     "Disabling might yield a slight performance boost on slower machines.",
	                           "false", PL_VAR_BOOL, &drawShadow, NULL, true );

	apeRegisterRendererConsoleVariables_();

	// Register variables which we'll use for post-processing. Uh, this also inits... Sorry!
	void R_PP_RegisterConsoleVariables( void );
	R_PP_RegisterConsoleVariables();

	apeRegisterAudioConsoleVariables_();
	apeRegisterWorldConsole_();
}
