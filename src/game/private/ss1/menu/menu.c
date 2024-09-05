// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "menu.h"
#include "../../shared/game_menu_pie.h"

static const char *menuFontPath = "guis/fonts/dejavu_sans_mono_bold_24.fnt";
static GuiFont    *menuFont;

static const char *menuTitleFontPath = "guis/fonts/six_caps_48.fnt";
static GuiFont    *menuTitleFont;

static bool isMainMenuOpen = true;

static Menu  mainMenu;
static Menu *currentMenu       = &mainMenu;
static uint  currentMenuOption = 0;

static void capture_screenshot_callback( const MenuOption * )
{
	isMainMenuOpen = false;
}

static MenuOption debugMenuOptions[] = {
        { "Test Model\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, .button = { "test_model" } },
        { "Test Net\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, .button = { "test_net" } },
        { nullptr, nullptr, nullptr, MENU_OPTION_TYPE_SEPERATOR },
        { "FPS Counter\n", nullptr, nullptr, MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showFps" } },
        { "Show Lights\n", nullptr, nullptr, MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showLights" } },
        { "Show Node Volumes\n", nullptr, nullptr, MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "world.showNodeVolumes" } },
        { "Show Portals\n", nullptr, nullptr, MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "world.showPortals" } },
        { "Wireframe\n", nullptr, nullptr, MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.wireframe" } },
        { "Shadow Wireframe\n", nullptr, nullptr, MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showShadowWireframe" } },
        { "Post-Processing\n", nullptr, nullptr, MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "postfx" } },
        { nullptr, nullptr, nullptr, MENU_OPTION_TYPE_SEPERATOR },
        { "Capture\n", nullptr, capture_screenshot_callback, MENU_OPTION_TYPE_BUTTON, .button = { "capture" } },
        { "Screenshot\n", nullptr, capture_screenshot_callback, MENU_OPTION_TYPE_BUTTON, .button = { "screenshot" } },
};
static Menu debugMenu = {
        "Debug Menu\n",
        debugMenuOptions,
        PL_ARRAY_ELEMENTS( debugMenuOptions ),
        &mainMenu,
};

static MenuOption quitMenuOptions[] = {
        { "Yes\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, .button = { "quit" } },
        { "No\n", &mainMenu, nullptr, MENU_OPTION_TYPE_BUTTON },
};
static Menu confirmQuitMenu = {
        "Are you sure?\n",
        quitMenuOptions,
        PL_ARRAY_ELEMENTS( quitMenuOptions ),
        &mainMenu,
};

static MenuOption startMenuOptions[] = {
        {"test_room\n",   nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, .button = { "world test_room" }  },
        {"zoo_shaders\n", nullptr, nullptr, MENU_OPTION_TYPE_BUTTON, .button = { "world zoo_shaders" }},
};
static Menu startMenu = {
        "Start Server\n",
        startMenuOptions,
        PL_ARRAY_ELEMENTS( startMenuOptions ),
        &mainMenu,
};

static MenuOption mainMenuOptions[] = {
#if !defined( NDEBUG )
        {"Debug\n",        &debugMenu,       nullptr, MENU_OPTION_TYPE_BUTTON},
#endif
        {"Start Server\n", &startMenu,       nullptr, MENU_OPTION_TYPE_BUTTON},
        {"Quit\n",         &confirmQuitMenu, nullptr, MENU_OPTION_TYPE_BUTTON},
};
static Menu mainMenu = {
        "Main Menu\n",
        mainMenuOptions,
        PL_ARRAY_ELEMENTS( mainMenuOptions ),
};

static GamePieMenu *interactPie;

static void initialize_menu( Menu *menu )
{
	for ( uint i = 0; i < menu->numOptions; ++i )
	{
		if ( menu->options[ i ].nextMenu != nullptr )
		{
			//initialize_menu( menu->options[ i ].nextMenu );
		}

		if ( menu->options[ i ].type == MENU_OPTION_TYPE_CHECKBOX )
		{
			menu->options[ i ].checkbox.var = PlGetConsoleVariable( menu->options[ i ].checkbox.varName );
			assert( menu->options[ i ].checkbox.var != nullptr );
		}
	}
}

void ss1_menu_initialize( void )
{
	menuFont = guiLoadFontFile( menuFontPath );
	if ( menuFont == nullptr )
	{
		game_error_( "Failed to load menu font (%s)!\n", menuFontPath );
	}

	menuTitleFont = guiLoadFontFile( menuTitleFontPath );
	if ( menuTitleFont == nullptr )
	{
		game_error_( "Failed to load title font (%s)!\n", menuTitleFontPath );
	}

	// mmm delicious pie
	interactPie = menu_pie_create();
	menu_pie_add_option( interactPie, "testing 1", ape_material_cache( "materials/ui/pie/cursor.mat.n", APE_CACHE_GROUP_WORLD, true, false ), NULL );
	menu_pie_add_option( interactPie, "testing 2", ape_material_cache( "materials/ui/pie/icon_mouth.mat.n", APE_CACHE_GROUP_WORLD, true, false ), NULL );
	menu_pie_add_option( interactPie, "testing 3", ape_material_cache( "materials/ui/pie/icon_tape.mat.n", APE_CACHE_GROUP_WORLD, true, false ), NULL );
	menu_pie_make_active( interactPie, true );

	// iterate over and init the menus
	initialize_menu( &mainMenu );
	initialize_menu( &debugMenu );

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
		float y = 64.0f;

		static const char *title    = "embrace";
		static const char *subtitle = "INC.\n";

		gui_font_draw_string( menuTitleFont, x, y, &x, nullptr, 1.0f, &PL_COLOUR_CRIMSON, title, strlen( title ), true );
		gui_font_draw_string( menuTitleFont, x, y + ( guiGetFontLineSpacing( menuTitleFont ) / 2.0f ), nullptr, nullptr, 0.5f, &PL_COLOUR_CRIMSON, subtitle, strlen( subtitle ), true );

		y = 200.0f;
		x = 80.0f;

		gui_font_draw_string( menuFont, x, y, nullptr, &y, 1.0f, &PL_COLOUR_WHITE, G_STR_( currentMenu->heading ), strlen( currentMenu->heading ), true );
		x += 30.0f;
		for ( uint i = 0; i < currentMenu->numOptions; ++i )
		{
			if ( currentMenu->options[ i ].type == MENU_OPTION_TYPE_SEPERATOR )
			{
				y += guiGetFontLineSpacing( menuFont ) / 2.0f;
				continue;
			}

			char tmp[ 128 ];
			if ( currentMenu->options[ i ].type == MENU_OPTION_TYPE_CHECKBOX )
			{
				snprintf( tmp, sizeof( tmp ), "[%s] %s", currentMenu->options[ i ].checkbox.var->b_value ? "X" : " ", currentMenu->options[ i ].string );
			}
			else
			{
				snprintf( tmp, sizeof( tmp ), "%s", currentMenu->options[ i ].string );
			}

			size_t len = strlen( tmp );

			static constexpr float SCALE = 0.7f;
			if ( currentMenuOption == i )
			{
				float w;
				gui_font_get_string_pixel_size( menuFont, SCALE, tmp, len, &w, nullptr );
				gui_font_draw_string( menuFont, x + w, y, nullptr, nullptr, SCALE, &PL_COLOUR_GOLD, "<", strlen( "<" ), true );
			}
			gui_font_draw_string( menuFont, x, y, nullptr, &y, SCALE, &PL_COLOUR_WHITE, tmp, len, true );
		}

		gui_font_display( menuFont );
		gui_font_display( menuTitleFont );
		return;
	}

	// draw our fancy little pie menu for interactions
	//menu_pie_draw( interactPie, ( float ) viewport->width / 2, ( float ) viewport->height / 2 );
}

static void next_menu_option()
{
	currentMenuOption++;
	if ( currentMenuOption >= currentMenu->numOptions )
	{
		currentMenuOption = 0;
	}
}

static void prev_menu_option()
{
	if ( currentMenuOption == 0 )
	{
		currentMenuOption = currentMenu->numOptions - 1;
	}
	else
	{
		currentMenuOption--;
	}
}

bool ss1_menu_handle_input( void )
{
	if ( isMainMenuOpen )
	{
		if ( ape_client_input_get_button_state( 0, APE_INPUT_DOWN ) == APE_INPUT_STATE_PRESSED )
		{
			do
			{
				next_menu_option();
			} while ( currentMenu->options[ currentMenuOption ].type == MENU_OPTION_TYPE_SEPERATOR );
			return true;
		}
		else if ( ape_client_input_get_button_state( 0, APE_INPUT_UP ) == APE_INPUT_STATE_PRESSED )
		{
			do
			{
				prev_menu_option();
			} while ( currentMenu->options[ currentMenuOption ].type == MENU_OPTION_TYPE_SEPERATOR );
			return true;
		}
		else if ( ape_client_input_get_button_state( 0, INPUT_A ) == APE_INPUT_STATE_PRESSED )
		{
			const MenuOption *option = &currentMenu->options[ currentMenuOption ];
			if ( option->callback != nullptr )
			{
				option->callback( option );
			}

			switch ( option->type )
			{
				default:
					break;
				case MENU_OPTION_TYPE_BUTTON:
				{
					if ( option->button.command != nullptr )
					{
						PlParseConsoleString( option->button.command );
					}
					break;
				}
				case MENU_OPTION_TYPE_CHECKBOX:
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
