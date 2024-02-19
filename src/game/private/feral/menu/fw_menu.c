// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "fw_menu.h"
#include "../../shared/game_menu_pie.h"

static Menu mainMenu;

static void quit_option( void )
{
}

static MenuOption quitMenuOptions[] = {
        {"Yes", NULL,      quit_option, MENU_OPTION_TYPE_BUTTON},
        { "No", &mainMenu, NULL,        MENU_OPTION_TYPE_BUTTON}
};

static Menu confirmQuitMenu = {
        "ARE YOU SURE?",
        quitMenuOptions,
        PL_ARRAY_ELEMENTS( quitMenuOptions ),
};

static MenuOption mainMenuOptions[] = {
        {"Settings", NULL,             NULL, MENU_OPTION_TYPE_BUTTON},
        { "Quit",    &confirmQuitMenu, NULL, MENU_OPTION_TYPE_BUTTON}
};

static Menu mainMenu = {
        "FERAL WARFARE",
        mainMenuOptions,
        PL_ARRAY_ELEMENTS( mainMenuOptions ),
};

static GamePieMenu *interactPie;

typedef enum FWPieMenuIcon
{
	FW_PIEMENU_ICON_USE,

	FW_MAX_PIEMENU_ICONS
} FWPieMenuIcon;
static ApeMaterial *pieIcons[ FW_MAX_PIEMENU_ICONS ];

void fw_menu_initialize( void )
{
	// mmm delicious pie
	interactPie = menu_pie_create();
	menu_pie_add_option( interactPie, "testing 1", ss_arl_material_cache( "materials/ui/pie/cursor.mat.n", APE_CACHE_WORLD, true, false ), NULL );
	menu_pie_add_option( interactPie, "testing 2", ss_arl_material_cache( "materials/ui/pie/icon_mouth.mat.n", APE_CACHE_WORLD, true, false ), NULL );
	menu_pie_add_option( interactPie, "testing 3", ss_arl_material_cache( "materials/ui/pie/icon_tape.mat.n", APE_CACHE_WORLD, true, false ), NULL );
	//FW_Menu_SetPieActive( interactPie, true );

	Game_Menu_SetCurrent( &mainMenu );
}

static void DrawHUD( const ApeViewport *viewport )
{
}

void fw_menu_tick( void )
{
	menu_pie_tick( interactPie );
}

void fw_menu_draw( const ApeViewport *viewport )
{
	DrawHUD( viewport );

	int w, h;
	ape_viewport_get_size( viewport, &w, &h );

	// draw our fancy little pie menu for interactions
	menu_pie_draw( interactPie, ( float ) w / 2, ( float ) h / 2 );
}

bool fw_menu_handle_input( void )
{
	static bool blah = true;
	if ( ape_client_input_get_button_state( 0, INPUT_START ) == APE_INPUT_STATE_PRESSED )
	{
		blah = !blah;
		menu_pie_make_active( interactPie, blah );
		return true;
	}
	if ( ape_client_input_get_button_state( 0, INPUT_X ) == APE_INPUT_STATE_PRESSED )
	{
		menu_pie_add_option( interactPie, "testing 4", ss_arl_material_cache( "materials/ui/pie/cursor.mat.n", APE_CACHE_WORLD, true, false ), NULL );
		return true;
	}

	if ( menu_pie_handle_input( interactPie ) )
	{
		return true;
	}

	return false;
}
