// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Gateway menu implementation.
// Author:  Mark E. Sowden

#include "gateway.h"

#include "menu/menu.h"

static constexpr char TITLE_FONT[] = "guis/fonts/zurich_bt_64.fnt";

static ApeGuiFont *menuFont;

static GameMenu mainMenu;

static GameMenuOption quitMenuOptions[] = {
        GAME_MENU_OPTION_BUTTON( "Yes\n", nullptr, nullptr, "quit" ),
        GAME_MENU_OPTION_BUTTON_SIMPLE( "No\n", &mainMenu ),
};
GAME_MENU_IMPLEMENT( confirmQuitMenu, "Are you sure?\n", quitMenuOptions, &mainMenu, 0 );

static GameMenuOption mainMenuOptions[] = {
        GAME_MENU_OPTION_BUTTON( "Quit\n", &confirmQuitMenu, nullptr, nullptr ),
};
GAME_MENU_IMPLEMENT_ROOT( mainMenu, "Main Menu\n", mainMenuOptions, 0 );

void gway_menu_initialize()
{
	game_menu_initialize();
	game_menu_set_title( "GATEWAY" );

	// setup the main menu
	game_menu_set_font( gui_get_default_font( GUI_FONT_DEFAULT_MEDIUM ) );

	menuFont = gui_font_load( TITLE_FONT, gui_get_default_font( GUI_FONT_DEFAULT_LARGE ) );
	game_menu_set_title_font( menuFont );

	game_menu_set_active( &mainMenu );
}

void gway_menu_draw( ApeViewport *viewport )
{
	game_menu_draw_( viewport );
}
