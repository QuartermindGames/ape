// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "ape_private.h"

#include "renderer/renderer.h"
#include "gui/gui_private.h"
#include "audio/audio.h"
#include "world/world.h"
#include "client/client_input.h"
#include "editor/editor.h"
#include "renderer/material/material.h"

static bool consoleIsOpen = false;
static bool drawShadow    = false;

static int   consoleAlpha     = 200;
static float consoleFontScale = 1.0f;

static ApeInputAction *consoleToggleAction;

static char         inputBuffer[ CONSOLE_BUFFER_MAX_LENGTH ];
static unsigned int inputBufferLength;

#define MAX_HISTORY_RESULTS 64
static char         history[ MAX_HISTORY_RESULTS ][ CONSOLE_BUFFER_MAX_LENGTH ];
static unsigned int numHistoryItems;
static unsigned int historySelection;

static float get_console_display_scale()
{
	return consoleFontScale * shell_get_display_scale();
}

/////////////////////////////////////////////////////////////////////////////////////
// Autocomplete
/////////////////////////////////////////////////////////////////////////////////////

static constexpr unsigned int MAX_AUTOCOMPLETE_RESULTS = 16;
static const char            *autoComplete[ MAX_AUTOCOMPLETE_RESULTS ];
static bool                   enableAutoCompleteList;
static unsigned int           autoCompleteSelection;

static void update_auto_complete_result( const char *input )
{
	// just clear it if an empty result is given
	if ( input == NULL || *input == '\0' )
	{
		QM_OS_ZERO( autoComplete, sizeof( const char * ) * MAX_AUTOCOMPLETE_RESULTS );
		return;
	}

	static constexpr unsigned int MAX_OPTIONS = MAX_AUTOCOMPLETE_RESULTS / 2;

	unsigned int numOptions = 0;

	const char  *commands[ MAX_OPTIONS ] = {};
	unsigned int numCommands             = ape_console_cmd_match( input, commands, MAX_OPTIONS );
	for ( unsigned int i = 0; i < numCommands; ++i )
	{
		autoComplete[ numOptions++ ] = commands[ i ];
	}

	const char  *vars[ MAX_OPTIONS ] = {};
	unsigned int numVars             = ape_console_var_match( input, vars, MAX_OPTIONS );
	for ( unsigned int i = 0; i < numVars; ++i )
	{
		autoComplete[ numOptions++ ] = vars[ i ];
	}

	autoComplete[ numOptions ] = nullptr;
	autoCompleteSelection      = 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// Notifications
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ConsoleNotification
{
	char            buffer[ CONSOLE_BUFFER_MAX_LENGTH ];
	QmMathColour4ub colour;
	double          time;
} ConsoleNotification;

static double consoleNotificationFadeThreshold = 0.8;
static double consoleMaxNotificationTime       = 3.0;

static int                  consoleMaxNotifications = 8;
static ConsoleNotification *consoleNotifications;
static unsigned int         consoleNumNotifications;

static void update_notification_limit( ApeConsoleVar * )
{
	if ( consoleMaxNotifications == 0 )
	{
		qm_os_memory_free( consoleNotifications );
		consoleNotifications    = nullptr;
		consoleNumNotifications = 0;
		return;
	}

	if ( consoleNumNotifications > consoleMaxNotifications )
	{
		consoleNumNotifications = consoleMaxNotifications;
	}

	size_t size          = sizeof( ConsoleNotification ) * consoleMaxNotifications;
	consoleNotifications = qm_os_memory_realloc( consoleNotifications, size );
	QM_OS_ZERO( consoleNotifications, size );
}

void ape_console_push_notification_( const char *buffer, QmMathColour4ub colour )
{
	if ( consoleMaxNotifications == 0 )
	{
		return;
	}

	if ( consoleNotifications == nullptr )
	{
		update_notification_limit( nullptr );
	}

	// shuffle everything forward
	if ( consoleNumNotifications > 0 )
	{
		for ( unsigned int i = consoleNumNotifications; i > 0; --i )
		{
			if ( i < consoleMaxNotifications )
			{
				consoleNotifications[ i ] = consoleNotifications[ i - 1 ];
			}
		}
	}

	// and now add the new result to the head
	ConsoleNotification *notification = &consoleNotifications[ 0 ];
	snprintf( notification->buffer, sizeof( notification->buffer ), "%s", buffer );
	notification->colour = colour;
	notification->time   = 0.0;

	consoleNumNotifications = QM_MATH_CLAMP( 0, consoleNumNotifications + 1, consoleMaxNotifications );
}

void ape_console_update_notifications_( double delta )
{
	if ( consoleNumNotifications == 0 )
	{
		return;
	}

	for ( unsigned int i = 0; i < consoleNumNotifications; ++i )
	{
		ConsoleNotification *notification = &consoleNotifications[ i ];
		notification->time += delta;

		if ( notification->time >= consoleMaxNotificationTime )
		{
			for ( unsigned int j = i; j < consoleNumNotifications - 1; ++j )
			{
				consoleNotifications[ j ] = consoleNotifications[ j + 1 ];
			}

			--consoleNumNotifications;
			--i;
		}
	}
}

static void draw_notifications( const ApeViewport *viewport )
{
	if ( consoleNumNotifications == 0 )
	{
		return;
	}

	ApeGuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_MEDIUM );
	assert( font != nullptr );

	const float scale = get_console_display_scale();

	float y = 8.0f;
	for ( unsigned int i = consoleNumNotifications; i > 0; --i )
	{
		if ( y >= viewport->height )
		{
			break;
		}

		ConsoleNotification *notification = &consoleNotifications[ i - 1 ];

		double timeLeft = consoleMaxNotificationTime - notification->time;
		if ( timeLeft > consoleMaxNotificationTime * ( 1.0 - consoleNotificationFadeThreshold ) )
		{
			notification->colour.a = QM_MATH_FTOB( 1.0f );
		}
		else
		{
			float fadeProgress     = timeLeft / ( consoleMaxNotificationTime * ( 1.0 - consoleNotificationFadeThreshold ) );
			notification->colour.a = QM_MATH_FTOB( QM_MATH_CLAMP( 0.0f, fadeProgress, 1.0f ) );
		}

		gui_font_draw_string( font, 8.0f * scale, y, nullptr, &y, scale, &notification->colour, notification->buffer, strlen( notification->buffer ), true );
	}

	gui_font_display( font );
}

/////////////////////////////////////////////////////////////////////////////////////
// Input
/////////////////////////////////////////////////////////////////////////////////////

static void toggle_console( void )
{
	consoleIsOpen = !consoleIsOpen;

	// Release the mouse if the console is open
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook->b_value )
	{
		ss_shell_grab_mouse( !consoleIsOpen );
	}
}

static void toggle_console_command( unsigned int argc, const char *const *argv )
{
	toggle_console();
}

static void toggle_console_action( const ApeInputState state, const char * )
{
	if ( !( state & APE_INPUT_STATE_PRESSED ) )
	{
		return;
	}

	toggle_console();
}

static void clear_history_command( unsigned int argc, const char *const *argv )
{
	numHistoryItems  = 0;
	historySelection = 0;
}

static void scroll_forward( ApeConsoleOutput *output )
{
	output->scrollPos++;
	if ( output->scrollPos > output->numLines - 1 )
	{
		output->scrollPos = output->numLines - 1;
	}
}

static void scroll_backward( ApeConsoleOutput *output )
{
	if ( output->scrollPos == 0 )
	{
		return;
	}

	output->scrollPos--;
}

bool ape_console_handle_mouse_wheel_event_( float x, float y )
{
	if ( !ape_is_console_open() )
	{
		return false;
	}

	ApeConsoleOutput *output = apeGetConsoleOutput();
	if ( y > 0.0f )
	{
		scroll_forward( output );
	}
	else if ( y < 0.0f )
	{
		scroll_backward( output );
	}

	return true;
}

static void clear_input_buffer( void )
{
	memset( inputBuffer, 0, sizeof( inputBuffer ) );
	inputBufferLength = 0;

	update_auto_complete_result( inputBuffer );
}

bool ape_console_handle_key_event_( int key, unsigned int keyState )
{
	/* only do anything if the console is open */
	if ( !consoleIsOpen )
	{
		return false;
	}
	/* but we don't care about these... */
	if ( keyState != APE_INPUT_STATE_PRESSED && keyState != APE_INPUT_STATE_DOWN )
	{
		return true;
	}

	ApeConsoleOutput *output = apeGetConsoleOutput();
	switch ( key )
	{
		default:
			break;
		case KEY_PAGEUP:
			scroll_forward( output );
			break;
		case KEY_PAGEDOWN:
			scroll_backward( output );
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

		case APE_INPUT_KEY_UP:
		{
			if ( autoComplete[ 0 ] == NULL )
			{
				if ( numHistoryItems > 0 )
				{
					historySelection = ( historySelection + 1 ) % numHistoryItems;
					snprintf( inputBuffer, sizeof( inputBuffer ), "%s", history[ historySelection ] );
					inputBufferLength = strlen( inputBuffer );
				}
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
		case APE_INPUT_KEY_DOWN:
		{
			if ( autoComplete[ 0 ] == NULL )
			{
				if ( numHistoryItems > 0 )
				{
					historySelection = ( historySelection - 1 ) % numHistoryItems;
					snprintf( inputBuffer, sizeof( inputBuffer ), "%s", history[ historySelection ] );
					inputBufferLength = strlen( inputBuffer );
				}
				break;
			}

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
				snprintf( inputBuffer, sizeof( inputBuffer ), "%s", autoComplete[ autoCompleteSelection ] );
				inputBufferLength = strlen( autoComplete[ autoCompleteSelection ] );
				update_auto_complete_result( inputBuffer );
				break;
			}

			if ( inputBuffer[ 0 ] != '\0' )
			{
				ape_console_print_( "%s\n", inputBuffer );

				ape_console_parse( inputBuffer );

				// shuffle everything back and then tack it onto our history list
				for ( int i = numHistoryItems - 1; i > 0; i-- )
				{
					strcpy( history[ i ], history[ i - 1 ] );
				}
				// and now add the new result to the head
				snprintf( history[ 0 ], sizeof( history[ 0 ] ), "%s", inputBuffer );
				numHistoryItems = QM_MATH_CLAMP( 0, numHistoryItems + 1, MAX_HISTORY_RESULTS );

				clear_input_buffer();
			}

			break;
		}
		case KEY_BACKSPACE:
		{
			if ( inputBufferLength > 0 )
			{
				inputBuffer[ --inputBufferLength ] = '\0';
			}

			update_auto_complete_result( inputBuffer );
			break;
		}
		case KEY_TAB:
		{ /* autocompletion */
			if ( *inputBuffer == '\0' || autoComplete[ 0 ] == NULL )
				break;

			/* update to match the first result */
			snprintf( inputBuffer, sizeof( inputBuffer ), "%s", autoComplete[ autoCompleteSelection ] );
			inputBufferLength = strlen( autoComplete[ autoCompleteSelection ] );

			update_auto_complete_result( inputBuffer );
			break;
		}
	}

	return consoleIsOpen;
}

bool ape_console_handle_text_event_( const char *key )
{
	if ( !consoleIsOpen )
	{
		return false;
	}

	// discard whatever we've got bound as the console toggle keys, ew
	// (this shouldn't be too bad, as we should only ever have one or two keys bound)
	unsigned int numKeys;
	ApeInputKey *keys = ape_input_action_get_keys( consoleToggleAction, &numKeys );
	for ( unsigned int i = 0; i < numKeys; ++i )
	{
		if ( keys[ i ] != *key )
		{
			continue;
		}

		return false;
	}

	/* check length before appending so we can ensure
     * it's always null terminated */
	if ( inputBufferLength + 1 >= CONSOLE_BUFFER_MAX_LENGTH )
	{
		return true;
	}

	inputBuffer[ inputBufferLength++ ] = *key;
	inputBuffer[ inputBufferLength ]   = '\0';

	update_auto_complete_result( inputBuffer );

	return true;
}

/****************************************
 * RENDERING
 ****************************************/

static void draw_input_field( const ApeViewport *viewport, ApeGuiFont *font )
{
	const float scale = get_console_display_scale();
	const float ch    = gui_font_get_line_spacing( font ) * scale;
	const float cw    = ape_gui_font_get_character_pixel_width( font, scale, '>' );
	gui_font_draw_character( font, scale, ( float ) viewport->height - ch, scale, &PL_COLOUR_LIME, '>' );

	/* cursor blinker */
	static unsigned int v = 0;
	if ( v < ape_get_num_ticks() )
	{
		v = ape_get_num_ticks() + 20;
	}

	float bufPixW;
	gui_font_get_string_pixel_size( font, scale, inputBuffer, inputBufferLength, &bufPixW, nullptr );

	const float x = ( 1.0f + cw );

	// cursor
	const char c = ( v > ape_get_num_ticks() + 10 ) ? '_' : ' ';
	gui_font_draw_character( font, x + bufPixW, ( float ) viewport->height - ch, scale, &PL_COLOUR_LIME, c );

	if ( autoComplete[ 0 ] != NULL )
	{
		size_t autoCompleteLength = strlen( autoComplete[ 0 ] );
		gui_font_draw_string( font, x + bufPixW, ( float ) viewport->height - ch, nullptr, nullptr, scale, &PL_COLOUR_GREEN, autoComplete[ 0 ] + inputBufferLength, autoCompleteLength - inputBufferLength, false );
		if ( enableAutoCompleteList )
		{
			unsigned int i = 1;
			while ( autoComplete[ i ] != NULL )
			{
				autoCompleteLength = strlen( autoComplete[ i ] );
				gui_font_draw_string( font, x, ( float ) viewport->height - ( ch * ( ( float ) i + 1 ) ), nullptr, nullptr, scale, &PL_COLOUR_LIME, inputBuffer, inputBufferLength, false );
				gui_font_draw_string( font, x + bufPixW, ( float ) viewport->height - ( ch * ( ( float ) i + 1 ) ), nullptr, nullptr, scale, &PL_COLOUR_GREEN, autoComplete[ i ] + inputBufferLength, autoCompleteLength - inputBufferLength, false );
				++i;
			}
		}
	}

	gui_font_draw_string( font, 1.0f + cw, ( float ) viewport->height - ch, nullptr, nullptr, scale, &PL_COLOUR_LIME, inputBuffer, inputBufferLength, false );

	gui_font_display( font );
}

bool ape_is_console_open( void ) { return consoleIsOpen; }

static constexpr float consoleScrollBarWidth = 8.0f;

/**
 * Draw the console panel.
 */
void ape_console_draw_( const ApeViewport *viewport )
{
	if ( !ape_is_console_open() )
	{
		draw_notifications( viewport );
		return;
	}

	ApeGuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_TINY );
	if ( font == NULL )
	{
		return;
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	qm_gfx_texture_set( nullptr, 0 );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

#define CON_SIDE_COLOUR      QM_MATH_COLOUR4UB_RGB( 128, 128, 128 )
#define CON_BACK_COLOUR      qm_math_colour4ub( 0, 0, 0, consoleAlpha )
#define CON_INDICATOR_COLOUR PL_COLOUR_DARK_BLUE
#define CON_INPUT_COLOUR     qm_math_colour4ub( 0, 0, 0, 255 )

	const float scale = get_console_display_scale();

	const float lineSpacing   = gui_font_get_line_spacing( font ) * scale;
	const float width         = ( float ) viewport->width;
	const float height        = ( float ) viewport->height;
	const float consoleHeight = height - lineSpacing;

	PlgSetBlendMode( PLG_BLEND_DEFAULT );

	PlgDrawRectangle( 0.0f, 0.0f, width, height - lineSpacing, CON_BACK_COLOUR );
	PlgDrawRectangle( 0.0f, height - lineSpacing, width, lineSpacing, CON_INPUT_COLOUR );
	PlgDrawRectangle( 0.0f, 0.0f, consoleScrollBarWidth, consoleHeight, CON_SIDE_COLOUR );

	PlgSetBlendMode( PLG_BLEND_DISABLE );

	const ApeConsoleOutput *output = apeGetConsoleOutput();
	if ( output->numLines > 0 )
	{
		/* draw the indicator at the side of the console */
		float cH = ( ( ( lineSpacing * ( float ) output->numLines ) / consoleHeight ) + 1.0f );
		float cY = consoleHeight - ( ( ( float ) output->numLines / consoleHeight ) + ( float ) output->scrollPos ) - cH;
		PlgDrawRectangle( 0.0f, cY, 8.0f * scale, cH, CON_INDICATOR_COLOUR );

		float y = consoleHeight - 20.0f;
		for ( unsigned int i = ( output->numLines - 1 ) - output->scrollPos; i > 0; --i )
		{
			/* draw the line we're currently at */
			gui_font_draw_string( font, 12.0f, y, nullptr, nullptr, scale, &output->lines[ i ].colour, output->lines[ i ].buffer, strlen( output->lines[ i ].buffer ), drawShadow );

			y -= lineSpacing;
			if ( y < 0 )
			{
				break;
			}
		}
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT );

	qm_gfx_texture_set( nullptr, 0 );

	gui_font_display( font );

	// auto-completion list
	if ( enableAutoCompleteList && ( autoComplete[ 0 ] != NULL ) )
	{
		float autoCompleteHeight = 0.0f;
		float autoCompleteWidth  = 0.0f;

		QmMathVector2f selectionBox;

		//HACK: because the height depends on a newline, we'll have to call this... fuck
		const float ch = gui_font_get_line_spacing( font ) * scale;

		// determine the box size
		for ( unsigned int i = 1; autoComplete[ i ] != nullptr; ++i )
		{
			float w;
			gui_font_get_string_pixel_size( font, scale, autoComplete[ i ], strlen( autoComplete[ i ] ), &w, nullptr );

			if ( w > autoCompleteWidth )
			{
				autoCompleteWidth = w;
			}

			autoCompleteHeight += ch;

			if ( autoCompleteSelection == i )
			{
				selectionBox.x = autoCompleteHeight;
				selectionBox.y = w;
			}
		}

		ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

		PlgDrawRectangle( consoleScrollBarWidth, ( height - autoCompleteHeight ) - ch, autoCompleteWidth, autoCompleteHeight, CON_INPUT_COLOUR );
		if ( autoCompleteSelection > 0 )
		{
			// draw the selection box on top
			PlgDrawRectangle( consoleScrollBarWidth, ( height - selectionBox.x ) - ch, selectionBox.y, ch, CON_INDICATOR_COLOUR );
		}
	}

	draw_input_field( viewport, font );

	/* draw version info */
	ApeGuiFont *tinyFont = gui_get_default_font( GUI_FONT_DEFAULT_TINY );
	if ( tinyFont != NULL )
	{
		static char buf[] = "v" ENGINE_VERSION_STR " [" GIT_BRANCH "." GIT_COMMIT_COUNT "]\n";

		float strW, strH;
		gui_font_get_string_pixel_size( tinyFont, scale, buf, sizeof( buf ), &strW, &strH );

		float x = width - strW;
		float y = height - strH;
		gui_font_draw_string( tinyFont, x, y, nullptr, nullptr, scale, &QM_MATH_COLOUR4UB_RGB( 0, 255, 0 ), buf, sizeof( buf ), false );

		gui_font_display( tinyFont );
	}

	PlPopMatrix();
}

/****************************************
 * CLIENT CONSOLE INIT
 ****************************************/

static void input_mlook_command( ApeConsoleVar *consoleVariable )
{
	if ( !consoleVariable->b_value )
	{
		return;
	}

	ape_input_center_mouse();
}

void ape_console_register_cl_commands_( void )
{
	ape_console_cmd_register( "console_toggle",
	                          "Toggle the console.", 0, toggle_console_command );
	ape_console_cmd_register( "console_clear_history",
	                          "Clear the console input history. Not to be confused with the \"clear\" command.",
	                          0, clear_history_command );
}

void ape_register_renderer_console_variables_( void );
void ape_renderer_world_register_console_variables_();

void ape_decal_manager_register_console_();

void ape_console_register_cl_variables_( void )
{
	ape_console_var_register( "local_name", "Set the name of the local player.", "unnamed", PL_VAR_STRING, NULL, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );

	ape_console_var_register( "input/mlook", "Toggle mouse look. If enabled, mouse is captured.", "1", PL_VAR_BOOL, NULL, input_mlook_command, APE_CONSOLE_VAR_FLAG_ARCHIVE );

	ape_console_var_register( "debug/profilerFrequency",
	                          "Set frequency at which profile graph updates.",
	                          "32", PL_VAR_I32, NULL, nullptr, 0 );

	ape_console_var_register( "console.autoCompleteList",
	                          "Enable/disable list of options that are presented for auto-completion.",
	                          "true", PL_VAR_BOOL, &enableAutoCompleteList, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
	ape_console_var_register( "console.alpha",
	                          "Level of transparency to use for the console background.",
	                          "200", PL_VAR_I32, &consoleAlpha, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
	ape_console_var_register( "console.drawShadow",
	                          "Shadow for text, which will improve legibility. Disabling might yield a slight performance boost on slower machines.",
	                          "false", PL_VAR_BOOL, &drawShadow, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
	ape_console_var_register( "console.maxNotifications",
	                          "Maximum number of notifications to show from the console buffer.",
	                          "8", PL_VAR_I32, &consoleMaxNotifications, update_notification_limit, APE_CONSOLE_VAR_FLAG_ARCHIVE );
	ape_console_var_register( "console.fontScale", "Set the font scale for the console.", "1.0", PL_VAR_F32, &consoleFontScale, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );

	ape_register_renderer_console_variables_();
	ape_decal_manager_register_console_();
	ape_renderer_world_register_console_variables_();

	ape_audio_register_console_variables_();
	ape_register_world_console_variables_();
	ape_editor_register_console_();

	consoleToggleAction = ape_client_input_register_action( "console", nullptr, 0, ( ApeInputKey[] ) { '`', '~' }, 2, toggle_console_action, APE_INPUT_ACTION_FLAG_GLOBAL );
}
