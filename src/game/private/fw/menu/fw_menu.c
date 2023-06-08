// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "fw_menu.h"
#include "fw_menu_pie.h"

static Menu mainMenu;

static void QuitOption( void )
{
	apeShutdown();
}

static MenuOption quitMenuOptions[] = {
        {"Yes", NULL,      QuitOption, MENU_OPTION_TYPE_BUTTON},
        { "No", &mainMenu, NULL,       MENU_OPTION_TYPE_BUTTON}
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

static FWPieMenu *interactPie;

typedef enum FWPieMenuIcon
{
	FW_PIEMENU_ICON_USE,

	FW_MAX_PIEMENU_ICONS
} FWPieMenuIcon;
static ApeMaterial *pieIcons[ FW_MAX_PIEMENU_ICONS ];

void FW_Menu_Initialize( void )
{
	// mmm delicious pie
	interactPie = FW_Menu_CreatePie();
	FW_Menu_AddPieOption( interactPie, "testing 1", apeCacheMaterial( "materials/ui/pie/cursor.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false ), NULL );
	FW_Menu_AddPieOption( interactPie, "testing 2", apeCacheMaterial( "materials/ui/pie/icon_mouth.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false ), NULL );
	FW_Menu_AddPieOption( interactPie, "testing 3", apeCacheMaterial( "materials/ui/pie/icon_tape.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false ), NULL );
	//FW_Menu_SetPieActive( interactPie, true );

	Game_Menu_SetCurrent( &mainMenu );
}

static void DrawHUD( const ApeViewport *viewport )
{
}

void FW_Menu_Tick( void )
{
	FW_Menu_TickPie( interactPie );
}

void FW_Menu_Draw( const ApeViewport *viewport )
{
	// get the centre of the screen
	int w, h;
	YnCore_Viewport_GetSize( viewport, &w, &h );
	int cx = w / 2;
	int cy = h / 2;

	switch ( Game_GetMenuState() )
	{
		default:
			break;
		case MENU_STATE_HUD:
			DrawHUD( viewport );
			break;
	}

	// draw our fancy little pie menu for interactions
	FW_Menu_DrawPie( interactPie, cx, cy );
}

bool FW_Menu_HandleInput( void )
{
	static bool blah = true;
	if ( apeGetButtonStatus( 0, INPUT_START ) == OGE_INPUT_STATE_PRESSED )
	{
		blah = !blah;
		FW_Menu_SetPieActive( interactPie, blah );
		return true;
	}
	if ( apeGetButtonStatus( 0, INPUT_X ) == OGE_INPUT_STATE_PRESSED )
	{
		FW_Menu_AddPieOption( interactPie, "testing 4", apeCacheMaterial( "materials/ui/pie/cursor.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false ), NULL );
		return true;
	}

	if ( FW_Menu_HandlePieInput( interactPie ) )
	{
		return true;
	}

	return false;
}
