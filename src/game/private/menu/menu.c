// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Generic menu system.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_string.h"
#include "aux/public/aux_project.h"

#include "menu.h"

#include "qmos/public/qm_os_random.h"

static GameMenu    *defaultMenu;
static GameMenu    *currentMenu;
static unsigned int currentMenuOption;

static char menuTitle[ 128 ];

static ApeGuiFont *menuFont;
static ApeGuiFont *menuTitleFont;

static void next_menu_option( const GameMenu *menu )
{
	currentMenuOption++;
	if ( currentMenuOption >= menu->numOptions )
	{
		currentMenuOption = 0;
	}
}

static void prev_menu_option( const GameMenu *menu )
{
	if ( currentMenuOption == 0 )
	{
		currentMenuOption = menu->numOptions - 1;
	}
	else
	{
		currentMenuOption--;
	}
}

static void handle_menu_action( ApeInputState state, const char *id )
{
	if ( !( state & APE_INPUT_STATE_PRESSED ) )
	{
		return;
	}

	if ( strcmp( id, "menu_toggle" ) == 0 )
	{
		GameMenu *menu = game_menu_get_active() ? nullptr : defaultMenu;
		game_menu_set_active( menu );
		currentMenuOption = 0;
		return;
	}

	GameMenu *menu = game_menu_get_active();
	if ( menu == nullptr )
	{
		return;
	}

	if ( strcmp( id, "menu_down" ) == 0 )
	{
		do
		{
			next_menu_option( menu );
		} while ( menu->options[ currentMenuOption ].type == GAME_MENU_OPTION_TYPE_SEPERATOR );
	}
	else if ( strcmp( id, "menu_up" ) == 0 )
	{
		do
		{
			prev_menu_option( menu );
		} while ( menu->options[ currentMenuOption ].type == GAME_MENU_OPTION_TYPE_SEPERATOR );
	}
	else if ( strcmp( id, "menu_select" ) == 0 )
	{
		const GameMenuOption *option = &menu->options[ currentMenuOption ];
		if ( option->callback != nullptr )
		{
			option->callback( option );
		}

		switch ( option->type )
		{
			default:
				break;
			case GAME_MENU_OPTION_TYPE_BUTTON:
			{
				if ( option->button.command != nullptr )
				{
					PlParseConsoleString( option->button.command );
				}
				break;
			}
			case GAME_MENU_OPTION_TYPE_CHECKBOX:
			{
				if ( option->checkbox.var != nullptr )
				{
					PlSetConsoleVariable( option->checkbox.var, option->checkbox.var->b_value ? "0" : "1" );
				}
				break;
			}
		}

		if ( option->nextMenu != nullptr )
		{
			menu->lastOption  = currentMenuOption;
			currentMenuOption = option->nextMenu->lastOption;
			game_menu_set_active( option->nextMenu );
		}
	}
	else if ( strcmp( id, "menu_back" ) == 0 )
	{
		if ( menu->parent != nullptr )
		{
			menu->lastOption  = currentMenuOption;
			currentMenuOption = menu->parent->lastOption;
			game_menu_set_active( menu->parent );
		}
	}
}

static constexpr char    MENU_BACKGROUND_SCRIPT_PATH[] = "scripts/menu_backgrounds.acm";
static constexpr uint8_t MENU_BACKGROUND_MAX           = 8;

static uint8_t numMenuBackgrounds;
static char   *menuBackgrounds[ MENU_BACKGROUND_MAX ];

void game_menu_load_background()
{
	unsigned int seed      = qm_os_random_seed_initialize();
	uint8_t      randomMap = qm_os_random_int( &seed ) % numMenuBackgrounds;
	const char  *path      = menuBackgrounds[ randomMap ];
}

void game_menu_unload_background()
{
}

void game_menu_initialize()
{
	// load in our backgrounds list, for fancy 3d source-like background stoof
	AcmBranch *root = com_acm_load_file( MENU_BACKGROUND_SCRIPT_PATH, "menuBackgrounds" );
	if ( root != nullptr )
	{
		ACM_ITERATE_BRANCH( root, i )
		{
			if ( numMenuBackgrounds >= MENU_BACKGROUND_MAX - 1 )
			{
				game_print_( "More backgrounds than supported! Maximum is 8.\n" );
				break;
			}

			menuBackgrounds[ numMenuBackgrounds ] = qm_os_string_alloc( "%s", acm_branch_get_value( i, nullptr ) );
			if ( menuBackgrounds[ numMenuBackgrounds ] == nullptr )
			{
				break;
			}

			numMenuBackgrounds++;
		}

		acm_branch_destroy( root );
	}

	menuFont      = gui_get_default_font( GUI_FONT_DEFAULT_MEDIUM );
	menuTitleFont = gui_get_default_font( GUI_FONT_DEFAULT_LARGE );

	ape_client_input_register_action( "menu_up", &( ApeInputButton ) { APE_INPUT_UP }, 1, &( ApeInputKey ) { APE_INPUT_KEY_UP }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_down", &( ApeInputButton ) { APE_INPUT_DOWN }, 1, &( ApeInputKey ) { APE_INPUT_KEY_DOWN }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_left", &( ApeInputButton ) { INPUT_LEFT }, 1, &( ApeInputKey ) { APE_INPUT_KEY_LEFT }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_right", &( ApeInputButton ) { INPUT_RIGHT }, 1, &( ApeInputKey ) { APE_INPUT_KEY_RIGHT }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_select", &( ApeInputButton ) { INPUT_A }, 1, &( ApeInputKey ) { KEY_ENTER }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_back", &( ApeInputButton ) { INPUT_B }, 1, &( ApeInputKey ) { APE_INPUT_KEY_LEFT }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_toggle", &( ApeInputButton ) { INPUT_START }, 1, &( ApeInputKey ) { APE_INPUT_KEY_ESCAPE }, 1, handle_menu_action );
}

void game_menu_shutdown()
{
	for ( uint8_t i = 0; i < numMenuBackgrounds; ++i )
	{
		qm_os_memory_free( menuBackgrounds[ i ] );
	}
}

void game_menu_setup( GameMenu *self )
{
	for ( unsigned int i = 0; i < self->numOptions; ++i )
	{
		if ( self->options[ i ].type == GAME_MENU_OPTION_TYPE_CHECKBOX )
		{
			self->options[ i ].checkbox.var = PlGetConsoleVariable( self->options[ i ].checkbox.varName );
			assert( self->options[ i ].checkbox.var != nullptr );
		}
	}

	if ( defaultMenu == nullptr )
	{
		defaultMenu = self;
	}
}

void game_menu_set_active( GameMenu *menu )
{
	currentMenu = menu;
}

GameMenu *game_menu_get_active( void )
{
	return currentMenu;
}

bool game_menu_is_open()
{
	return game_menu_get_active() != nullptr;
}

void game_menu_draw_( const ApeViewport *viewport )
{
	GameMenu *menu = game_menu_get_active();
	if ( menu == nullptr )
	{
		return;
	}

	float scale = shell_get_display_scale();

	float x = 128.0f;
	float y = 128.0f;

	if ( *menuTitle != '\0' )
	{
		assert( menuTitleFont != nullptr );
		gui_font_set_shadow_offset( 2.0f, 2.0f );
		gui_font_draw_string( menuTitleFont, x, y, &x, &y, scale, &PL_COLOUR_WHITE, menuTitle, strlen( menuTitle ), true );
	}

	assert( menuFont != nullptr );

	if ( menu->flags & GAME_MENU_FLAG_PROMPT )
	{
		float w;
		gui_font_get_string_pixel_size( menuFont, scale, G_STR_( menu->heading ), strlen( menu->heading ), &w, nullptr );
		x = ( viewport->width - w ) / 2.0f;
		y = ( viewport->height - gui_font_get_line_spacing( menuFont ) * 2.0f ) / 2.0f;
	}

	gui_font_set_shadow_offset( GUI_FONT_SHADOW_DEFAULT );
	gui_font_draw_string( menuFont, x, y, nullptr, &y, scale, &PL_COLOUR_WHITE, G_STR_( menu->heading ), strlen( menu->heading ), true );
	x += 30.0f;
	for ( unsigned int i = 0; i < menu->numOptions; ++i )
	{
		if ( menu->options[ i ].type == GAME_MENU_OPTION_TYPE_SEPERATOR )
		{
			y += gui_font_get_line_spacing( menuFont ) / 2.0f;
			continue;
		}

		char tmp[ 128 ];
		if ( menu->options[ i ].type == GAME_MENU_OPTION_TYPE_CHECKBOX )
		{
			snprintf( tmp, sizeof( tmp ), "[%s] %s", menu->options[ i ].checkbox.var->b_value ? "X" : " ", menu->options[ i ].string );
		}
		else
		{
			snprintf( tmp, sizeof( tmp ), "%s", menu->options[ i ].string );
		}

		size_t len = strlen( tmp );

		const float optionScale = scale * 0.7f;
		if ( currentMenuOption == i )
		{
			float w;
			gui_font_get_string_pixel_size( menuFont, optionScale, tmp, len, &w, nullptr );
			gui_font_draw_string( menuFont, x + w, y, nullptr, nullptr, optionScale, &PL_COLOUR_GOLD, "<", strlen( "<" ), true );
		}
		gui_font_draw_string( menuFont, x, y, nullptr, &y, optionScale, &PL_COLOUR_WHITE, tmp, len, true );
	}

	if ( menu->flags & GAME_MENU_FLAG_BACKGROUND )
	{
		ape_draw_textured_quad( ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX ),
		                        0.0f, 0.0f,
		                        viewport->width, viewport->height,
		                        &PL_COLOUR_BLACK, 0 );
	}

	gui_font_display( menuFont );
	gui_font_display( menuTitleFont );
}

void game_menu_set_title( const char *title )
{
	snprintf( menuTitle, sizeof( menuTitle ), "%s\n", G_STR_( title ) );
}

void game_menu_set_font( ApeGuiFont *font )
{
	menuFont = font;
}

void game_menu_set_title_font( ApeGuiFont *font )
{
	menuTitleFont = font;
}
