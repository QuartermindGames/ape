// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ss1_menu.h"
#include "../../shared/game_menu_pie.h"

static const char *menuFontPath = "guis/fonts/uroob_42.fnt";
static GuiFont    *menuFont;

static bool isMainMenuOpen = true;

static Menu  mainMenu;
static Menu *currentMenu       = &mainMenu;
static uint  currentMenuOption = 0;

static MenuOption debugMenuOptions[] = {
        { "Enable postfx\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, "postfx 1" },
        { "Disable postfx\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, "postfx 0" },
};
static Menu debugMenu = {
        "Debug Menu\n",
        debugMenuOptions,
        PL_ARRAY_ELEMENTS( debugMenuOptions ),
        &mainMenu,
};

static MenuOption quitMenuOptions[] = {
        { "Yes\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, "quit" },
        { "No\n", &mainMenu, nullptr, MENU_OPTION_TYPE_BUTTON },
};
static Menu confirmQuitMenu = {
        "Are you sure?\n",
        quitMenuOptions,
        PL_ARRAY_ELEMENTS( quitMenuOptions ),
        &mainMenu,
};

static MenuOption startMenuOptions[] = {
        { "test_room\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, "world test_room" },
        { "zoo_shaders\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, "world zoo_shaders" },
};
static Menu startMenu = {
        "Start Server\n",
        startMenuOptions,
        PL_ARRAY_ELEMENTS( startMenuOptions ),
        &mainMenu,
};

static MenuOption mainMenuOptions[] = {
#if !defined( NDEBUG )
        { "Debug\n", &debugMenu, nullptr, MENU_OPTION_TYPE_BUTTON },
#endif
        { "Start Server\n", &startMenu, nullptr, MENU_OPTION_TYPE_BUTTON },
        { "Join Server\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON },
        { "Settings\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON },
        { "Quit\n", &confirmQuitMenu, nullptr, MENU_OPTION_TYPE_BUTTON },
};
static Menu mainMenu = {
        "Project SS1\n",
        mainMenuOptions,
        PL_ARRAY_ELEMENTS( mainMenuOptions ),
};

static GamePieMenu *interactPie;

void ss1_menu_initialize( void )
{
	menuFont = guiLoadFontFile( menuFontPath );
	if ( menuFont == nullptr )
	{
		game_error_( "Failed to load menu font (%s)!\n", menuFontPath );
	}

	// mmm delicious pie
	interactPie = menu_pie_create();
	menu_pie_add_option( interactPie, "testing 1", ape_material_cache( "materials/ui/pie/cursor.mat.n", APE_CACHE_GROUP_WORLD, true, false ), NULL );
	menu_pie_add_option( interactPie, "testing 2", ape_material_cache( "materials/ui/pie/icon_mouth.mat.n", APE_CACHE_GROUP_WORLD, true, false ), NULL );
	menu_pie_add_option( interactPie, "testing 3", ape_material_cache( "materials/ui/pie/icon_tape.mat.n", APE_CACHE_GROUP_WORLD, true, false ), NULL );
	menu_pie_make_active( interactPie, true );

	Game_Menu_SetCurrent( &mainMenu );
}

void ss1_menu_shutdown()
{
	guiDestroyFont( menuFont );
}

void ss1_menu_tick( void )
{
	menu_pie_tick( interactPie );
}

void ss1_menu_draw( const ApeViewport *viewport )
{
	if ( isMainMenuOpen )
	{
		assert( currentMenu != nullptr );

		float x = 50.0f;
		float y = ( float ) viewport->height / 2.0f;
		gui_font_draw_string( menuFont, x, y, nullptr, &y, 1.0f, &PL_COLOUR_WHITE, currentMenu->heading, strlen( currentMenu->heading ), true );
		x += 30.0f;
		for ( uint i = 0; i < currentMenu->numOptions; ++i )
		{
			static constexpr float SCALE = 0.7f;
			if ( currentMenuOption == i )
			{
				float w;
				gui_font_get_string_pixel_size( menuFont, SCALE, currentMenu->options[ i ].string, strlen( currentMenu->options[ i ].string ), &w, nullptr );
				gui_font_draw_string( menuFont, x + w, y, nullptr, nullptr, SCALE, &PL_COLOUR_GOLD, "<", strlen( "<" ), true );
			}
			gui_font_draw_string( menuFont, x, y, nullptr, &y, SCALE, &PL_COLOUR_WHITE, currentMenu->options[ i ].string, strlen( currentMenu->options[ i ].string ), true );
		}

		gui_font_display( menuFont );
		return;
	}

	// draw our fancy little pie menu for interactions
	//menu_pie_draw( interactPie, ( float ) viewport->width / 2, ( float ) viewport->height / 2 );
}

bool ss1_menu_handle_input( void )
{
	if ( isMainMenuOpen )
	{
		if ( ape_client_input_get_button_state( 0, APE_INPUT_DOWN ) == APE_INPUT_STATE_PRESSED )
		{
			currentMenuOption++;
			if ( currentMenuOption >= currentMenu->numOptions )
			{
				currentMenuOption = 0;
			}

			return true;
		}
		else if ( ape_client_input_get_button_state( 0, APE_INPUT_UP ) == APE_INPUT_STATE_PRESSED )
		{
			if ( currentMenuOption == 0 )
			{
				currentMenuOption = currentMenu->numOptions - 1;
			}
			else
			{
				currentMenuOption--;
			}

			return true;
		}
		else if ( ape_client_input_get_button_state( 0, INPUT_A ) == APE_INPUT_STATE_PRESSED )
		{
			const MenuOption *option = &currentMenu->options[ currentMenuOption ];
			if ( option->callback != nullptr )
			{
				option->callback( option );
			}
			if ( option->command != nullptr )
			{
				PlParseConsoleString( option->command );
			}
			if ( option->nextMenu != nullptr )
			{
				currentMenu->lastOption = currentMenuOption;
				currentMenu             = option->nextMenu;
				currentMenuOption       = currentMenu->lastOption;
			}
			return true;
		}
		else if ( ape_client_input_get_button_state( 0, INPUT_B ) == APE_INPUT_STATE_PRESSED )
		{
			if ( currentMenu->parent != nullptr )
			{
				currentMenu       = currentMenu->parent;
				currentMenuOption = currentMenu->lastOption;
			}
			return true;
		}
	}

	if ( ape_client_input_get_button_state( 0, INPUT_START ) == APE_INPUT_STATE_PRESSED )
	{
		isMainMenuOpen    = !isMainMenuOpen;
		currentMenuOption = 0;
		return true;
	}

#if 0
	// pie menu crap...
	static bool blah = true;
	if ( ape_client_input_get_button_state( 0, INPUT_X ) == APE_INPUT_STATE_PRESSED )
	{
		menu_pie_add_option( interactPie, "testing 4", ape_material_cache( "materials/ui/pie/cursor.mat.n", APE_CACHE_GROUP_WORLD, true, false ), NULL );
		return true;
	}
	if ( menu_pie_handle_input( interactPie ) )
	{
		return true;
	}
#endif

	return false;
}
