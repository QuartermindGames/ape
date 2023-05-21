// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"

#include "renderer/renderer.h"
#include "renderer/renderer_font.h"
#include "gui/gui_private.h"

static bool consoleIsOpen = false;
static bool drawShadow    = false;

/****************************************
 * CONSOLE INPUT BUFFER
 ****************************************/

#define CON_BUFFER_MAX_LENGTH 256
static char conInputBuffer[ CONSOLE_BUFFER_MAX_LENGTH ] = { '\0' };
static unsigned int conInputBufferLength                = 0;

#define MAX_HISTORY_RESULTS 64
static char history[ MAX_HISTORY_RESULTS ][ 64 ] = { { '\0' } };
static unsigned int numHistoryItems              = 0;
static unsigned int historySelection             = 0;

/////////////////////////////////////////////////////////////////
// AUTOCOMPLETE

#define MAX_AUTOCOMPLETE_RESULTS 8
static const char *autoComplete[ MAX_AUTOCOMPLETE_RESULTS ] = { NULL };
static bool enableAutoCompleteList;
static unsigned int autoCompleteSelection = 0;

static void UpdateAutoCompleteResult( const char *input )
{
	// just clear it if an empty result is given
	if ( input == NULL || *input == '\0' )
	{
		PL_ZERO( autoComplete, sizeof( const char * ) * MAX_AUTOCOMPLETE_RESULTS );
		return;
	}

	// fetch all matching results
	unsigned int numOptions;
	const char **list = PlAutocompleteConsoleString( input, &numOptions );
	if ( numOptions >= MAX_AUTOCOMPLETE_RESULTS )
	{
		numOptions = MAX_AUTOCOMPLETE_RESULTS - 1;
	}

	// fill the list, leaving the last item null so we know where it ends
	for ( unsigned int i = 0; i < numOptions; ++i )
	{
		autoComplete[ i ] = list[ i ];
	}
	autoComplete[ numOptions ] = NULL;

	autoCompleteSelection = 0;
}

/////////////////////////////////////////////////////////////////

bool ogeConsole_HandleTextEvent( const char *key )
{
	// todo y3: allow this key to be customised
	if ( !consoleIsOpen || *key == '`' || *key == '~' )
		return false;

	/* check length before appending so we can ensure
     * it's always null terminated */
	if ( conInputBufferLength + 1 >= CONSOLE_BUFFER_MAX_LENGTH )
		return true;

	conInputBuffer[ conInputBufferLength++ ] = *key;
	conInputBuffer[ conInputBufferLength ]   = '\0';

	UpdateAutoCompleteResult( conInputBuffer );

	return true;
}

/****************************************
 * GENERAL INPUT
 ****************************************/

static void ToggleConsole( void )
{
	consoleIsOpen = !consoleIsOpen;

	// Release the mouse if the console is open
	PL_GET_CVAR( "input.mlook", mouseLook );
	if ( mouseLook != NULL && mouseLook->b_value )
		YnCore_ShellInterface_GrabMouse( !consoleIsOpen );
}

static void ToggleConsoleCommand( unsigned int argc, char **argv )
{
	( void ) ( argc );
	( void ) ( argv );
	ToggleConsole();
}

static void ScrollForward( ConsoleOutput *output )
{
	output->scrollPos++;
	if ( output->scrollPos > output->numLines - 1 )
		output->scrollPos = output->numLines - 1;
}

static void ScrollBackward( ConsoleOutput *output )
{
	if ( output->scrollPos == 0 )
		return;

	output->scrollPos--;
}

bool Client_Console_HandleMouseWheelEvent( float x, float y )
{
	if ( !Client_Console_IsOpen() )
		return false;

	ConsoleOutput *output = Console_GetOutput();
	if ( y > 0.0f )
		ScrollForward( output );
	else if ( y < 0.0f )
		ScrollBackward( output );

	return true;
}

static void ClearInputBuffer( void )
{
	memset( conInputBuffer, 0, sizeof( conInputBuffer ) );
	conInputBufferLength = 0;

	UpdateAutoCompleteResult( conInputBuffer );
}

bool Client_Console_HandleKeyboardEvent( int key, unsigned int keyState )
{
	if ( keyState == YN_CORE_INPUT_STATE_DOWN && ( key == '`' || key == '~' ) )
	{
		ToggleConsole();
		return true;
	}

	/* only do anything if the console is open */
	if ( !consoleIsOpen )
		return false;
	/* but we don't care about these... */
	if ( keyState != YN_CORE_INPUT_STATE_PRESSED && keyState != YN_CORE_INPUT_STATE_DOWN )
		return true;

	ConsoleOutput *output = Console_GetOutput();
	switch ( key )
	{
		default:
			break;
		case KEY_PAGEUP:
			ScrollForward( output );
			break;
		case KEY_PAGEDOWN:
			ScrollBackward( output );
			break;
		case KEY_END:
		{
			output->scrollPos = 0;
			break;
		}
		case KEY_HOME:
		{
			output->scrollPos = output->numLines - 1;
			break;
		}

		case KEY_UP:
		{
			if ( autoComplete[ 0 ] == NULL )
			{
				// in this case, cycle the history

				break;
			}

			unsigned int nextSlot = autoCompleteSelection + 1;
			if ( nextSlot >= MAX_AUTOCOMPLETE_RESULTS || autoComplete[ nextSlot ] == NULL )
			{
				autoCompleteSelection = 0;
				break;
			}

			autoCompleteSelection++;
			break;
		}
		case KEY_DOWN:
		{
			if ( autoComplete[ 0 ] == NULL )
				break;

			if ( autoCompleteSelection == 0 )
			{
				autoCompleteSelection = MAX_AUTOCOMPLETE_RESULTS - 1;
				while ( autoComplete[ autoCompleteSelection ] == NULL ) { autoCompleteSelection--; }
				break;
			}

			autoCompleteSelection--;
			break;
		}

		case KEY_ENTER:
		{
			if ( autoComplete[ 0 ] != NULL && autoCompleteSelection > 0 )
			{
				snprintf( conInputBuffer, sizeof( conInputBuffer ), "%s", autoComplete[ autoCompleteSelection ] );
				conInputBufferLength = strlen( autoComplete[ autoCompleteSelection ] );
				UpdateAutoCompleteResult( conInputBuffer );
				break;
			}
			else if ( conInputBuffer[ 0 ] != '\0' )
			{
				PlParseConsoleString( conInputBuffer );
				ClearInputBuffer();
			}
			break;
		}
		case KEY_BACKSPACE:
		{
			if ( conInputBufferLength > 0 )
				conInputBuffer[ --conInputBufferLength ] = '\0';

			UpdateAutoCompleteResult( conInputBuffer );
			break;
		}
		case KEY_TAB:
		{ /* autocompletion */
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

static void Client_Console_DrawInputField( const OgeViewport *viewport )
{
	BitmapFont *font = Font_GetDefault();

	/* cursor */
	Font_AddBitmapCharacterToPass( font, 1.0f, ( float ) viewport->height - font->ch, 1.0f, PL_COLOUR_LIME, '>' );

	/* cursor blinker */
#define SPACER 4.0f
	static unsigned int v = 0;
	if ( v < ogeGetNumTicks() )
		v = ogeGetNumTicks() + 20;

	char c = ( v > ogeGetNumTicks() + 10 ) ? '_' : ' ';
	Font_AddBitmapCharacterToPass( font, ( float ) ( font->cw + SPACER + ( font->cw * conInputBufferLength ) ), ( float ) viewport->height - font->ch, 1.0f, PL_COLOUR_LIME, c );

	/* input buffer */

	if ( autoComplete[ 0 ] != NULL )
	{
		size_t autoCompleteLength = strlen( autoComplete[ 0 ] );
		float x                   = ( font->cw + SPACER ) + ( font->cw * conInputBufferLength );
		Font_AddBitmapStringToPass( font, x, ( float ) viewport->height - font->ch, 1.0f, PL_COLOUR_GREEN, autoComplete[ 0 ] + conInputBufferLength, autoCompleteLength - conInputBufferLength, false );

		if ( enableAutoCompleteList )
		{
			unsigned int i = 1;
			while ( autoComplete[ i ] != NULL )
			{
				autoCompleteLength = strlen( autoComplete[ i ] );
				Font_AddBitmapStringToPass( font, font->cw + SPACER, ( float ) viewport->height - ( font->ch * ( i + 1 ) ), 1.0f, PL_COLOUR_LIME, conInputBuffer, conInputBufferLength, false );
				Font_AddBitmapStringToPass( font, x, ( float ) viewport->height - ( font->ch * ( i + 1 ) ), 1.0f, PL_COLOUR_GREEN, autoComplete[ i ] + conInputBufferLength, autoCompleteLength - conInputBufferLength, false );
				++i;
			}
		}
	}

	Font_AddBitmapStringToPass( font, font->cw + SPACER, ( float ) viewport->height - font->ch, 1.0f, PL_COLOUR_LIME, conInputBuffer, conInputBufferLength, false );
}

bool Client_Console_IsOpen( void ) { return consoleIsOpen; }

static const float consoleScrollBarWidth = 8.0f;

/**
 * Draw the console panel.
 */
void Client_Console_Draw( const OgeViewport *viewport )
{
	if ( !consoleIsOpen )
		return;

	GUIFont *font = GUI_Font_GetDefault( GUI_FONT_DEFAULT_SMALL );
	assert( font != NULL );
	if ( font == NULL )
	{
		return;
	}

	PlgSetTexture( NULL, 0 );
	PlgSetBlendMode( PLG_BLEND_DEFAULT );
	PlgSetShaderProgram( oge_defaultShaderPrograms[ OGE_SHADER_DEFAULT_VERTEX ] );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

#define CON_SIDE_COLOUR      PLColourRGB( 128, 128, 128 )
#define CON_BACK_COLOUR      PLColour( 0, 0, 0, 128 )
#define CON_INDICATOR_COLOUR PL_COLOUR_DARK_BLUE
#define CON_INPUT_COLOUR     PLColour( 0, 0, 0, 255 )

	float width         = ( float ) viewport->width;
	float height        = ( float ) viewport->height;
	float consoleHeight = height - 12.0f;

	//PlgDrawRectangle( 0.0f, 0.0f, width, height - font->ch, CON_BACK_COLOUR );
	//PlgDrawRectangle( 0.0f, height - ( float ) font->ch, width, ( float ) font->ch, CON_INPUT_COLOUR );
	PlgDrawRectangle( 0.0f, 0.0f, consoleScrollBarWidth, consoleHeight, CON_SIDE_COLOUR );

	const ConsoleOutput *output = Console_GetOutput();
	if ( output->numLines > 0 )
	{
#if 0
		/* draw the indicator at the side of the console */
		float cH = ( ( font->ch * output->numLines ) / consoleHeight ) + 1.0f;
		float cY = consoleHeight - ( ( output->numLines / consoleHeight ) + output->scrollPos ) - cH;
		PlgDrawRectangle( 0.0f, cY, 8.0f, cH, CON_INDICATOR_COLOUR );
#endif

		float y = consoleHeight - 20.0f;
		for ( unsigned int i = ( output->numLines - 1 ) - output->scrollPos; i > 0; --i )
		{
			/* draw the line we're currently at */
			float oy;
			GUI_Font_DrawString( font, 12.0f, y, NULL, &oy, 1.0f, &output->lines[ i ].colour, output->lines[ i ].buffer, strlen( output->lines[ i ].buffer ), drawShadow );
			y -= 20;
		}
	}

	PlgSetShaderProgram( oge_defaultShaderPrograms[ OGE_SHADER_DEFAULT_VERTEX ] );
	PlgSetTexture( NULL, 0 );

	// auto-completion list
#if 0
	if ( enableAutoCompleteList && ( autoComplete[ 0 ] != NULL ) )
	{
		float autoCompleteHeight = 0.0f;
		float autoCompleteWidth  = 0.0f;

		// iterate over the options to determine height, width
		unsigned int i = 1;
		while ( autoComplete[ i ] != NULL )
		{
			float w = ( float ) ( font->cw * ( strlen( autoComplete[ i ] ) + 1 ) );
			if ( w > autoCompleteWidth ) autoCompleteWidth = w;
			autoCompleteHeight += font->ch;
			PlgDrawRectangle( consoleScrollBarWidth, ( height - ( float ) font->ch ) - autoCompleteHeight, w, font->ch, ( autoCompleteSelection == i ) ? CON_INDICATOR_COLOUR : CON_INPUT_COLOUR );
			++i;
		}
	}
#endif

	PlgSetShaderProgram( oge_defaultShaderPrograms[ OGE_SHADER_DEFAULT ] );
	PlgSetTexture( NULL, 0 );

	GUI_Font_Display( font );

	Client_Console_DrawInputField( viewport );

	/* draw version info */
	GUIFont *tinyFont = GUI_Font_GetDefault( GUI_FONT_DEFAULT_TINY );
	if ( tinyFont != NULL )
	{
		static char buf[] = "v" ENGINE_VERSION_STR " [" GIT_BRANCH "." GIT_COMMIT_COUNT "]";

		float strW, strH;
		GUI_Font_GetStringPixelSize( tinyFont, 1.0f, buf, sizeof( buf ), &strW, &strH );

		float x = width - strW - 2.0f;
		float y = height - strH - 2.0f;
		GUI_Font_DrawString( tinyFont, x, y, NULL, NULL, 1.0f, &PLColourRGB( 0, 255, 0 ), buf, sizeof( buf ), false );

		GUI_Font_Display( tinyFont );
	}

	PlPopMatrix();

	PlgSetBlendMode( PLG_BLEND_DISABLE );
}

/****************************************
 * CLIENT CONSOLE INIT
 ****************************************/

static void CreateViewportCommand( unsigned int argc, char **argv )
{
	int width  = strtol( argv[ 1 ], NULL, 10 );
	int height = strtol( argv[ 2 ], NULL, 10 );

	if ( !ogeShellInterface_CreateWindow( "Yin Viewport", width, height, false, 0 ) )
	{
		PRINT_WARNING( "Failed to create viewport!\n" );
		return;
	}
}

void Client_Console_RegisterConsoleCommands( void )
{
	PlRegisterConsoleCommand( "Toggle", "Toggle the console.", 0, ToggleConsoleCommand );
	PlRegisterConsoleCommand( "client.create_viewport", "Create a new viewport / window", 2, CreateViewportCommand );

	//PlRegisterConsoleCommand( "Connect", NULL, "Connect to the specified server." );
	//PlRegisterConsoleCommand( "Reconnect", NULL, "Reconnect to the current server." );
	//PlRegisterConsoleCommand( "Disconnect", NULL, "Disconnect from the current server." );
}

void Renderer_RegisterConsoleVariables( void );

void Client_Console_RegisterConsoleVariables( void )
{
	PlRegisterConsoleVariable( "client.name", "Set the name of the local player.", "unnamed", PL_VAR_STRING, NULL, NULL, true );

	PlRegisterConsoleVariable( "input.mlook", "Toggle mouse look. If enabled, mouse is captured.", "0", PL_VAR_BOOL, NULL, NULL, true );

	// editor
	PlRegisterConsoleVariable( "edit.gridSize", "Set the maximum grid size.", "128", PL_VAR_I32, NULL, NULL, true );

	PlRegisterConsoleVariable( "debug.overlay", "Enable/disable debug overlays.", "0", PL_VAR_I32, NULL, NULL, false );
	PlRegisterConsoleVariable( "debug.profilerFrequency", "Set frequency at which profile graph updates.", "16", PL_VAR_I32, NULL, NULL, false );

	PlRegisterConsoleVariable( "console.autocomplete_list", "Enable/disable list of options that are presented for auto-completion.", "true", PL_VAR_BOOL, &enableAutoCompleteList, NULL, true );
	PlRegisterConsoleVariable( "console.alpha", "Level of transparency to use for the console background.", "200", PL_VAR_I32, NULL, NULL, true );
	PlRegisterConsoleVariable( "console.drawBackground", "Whether or not to display background.", "true", PL_VAR_BOOL, NULL, NULL, true );
	PlRegisterConsoleVariable( "console.drawShadow", "Shadow for text, which will improve legibility. "
	                                                 "Disabling might yield a slight performance boost on slower machines.",
	                           "true", PL_VAR_BOOL, &drawShadow, NULL, true );

	Renderer_RegisterConsoleVariables();

	// Register variables which we'll use for post-processing. Uh, this also inits... Sorry!
	void R_PP_RegisterConsoleVariables( void );
	R_PP_RegisterConsoleVariables();

	void Audio_RegisterConsoleVariables( void );
	Audio_RegisterConsoleVariables();

	PlRegisterConsoleVariable( "world.drawSectorVolumes", "Toggle rendering of sector volumes.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world.drawSectors", "Toggle rendering of sectors.", "true", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world.drawSubMeshes", "Toggle rendering of sub-meshes within sectors.", "true", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world.forceSimple", "Force simple render pass of world.", "false", PL_VAR_BOOL, NULL, NULL, false );
}
