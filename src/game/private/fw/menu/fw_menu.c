// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "fw_menu.h"
#include "fw_menu_pie.h"

static Menu mainMenu;

static void QuitOption( void )
{
	ogeShutdown();
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
static OgeMaterial *pieIcons[ FW_MAX_PIEMENU_ICONS ];

void FW_Menu_Initialize( void )
{
	// mmm delicious pie
	interactPie = FW_Menu_CreatePie();
	FW_Menu_AddPieOption( interactPie, "testing 1", YnCore_Material_Cache( "materials/ui/pie/cursor.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false ), NULL );
	FW_Menu_AddPieOption( interactPie, "testing 2", YnCore_Material_Cache( "materials/ui/pie/icon_mouth.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false ), NULL );
	FW_Menu_AddPieOption( interactPie, "testing 3", YnCore_Material_Cache( "materials/ui/pie/icon_tape.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false ), NULL );
	//FW_Menu_SetPieActive( interactPie, true );

	Game_Menu_SetCurrent( &mainMenu );
}

static void DrawHUD( const OgeViewport *viewport )
{
}

void FW_Menu_Tick( void )
{
	FW_Menu_TickPie( interactPie );
}

void FW_Menu_Draw( const OgeViewport *viewport )
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
	if ( YnCore_Input_GetButtonStatus( 0, INPUT_START ) == YN_CORE_INPUT_STATE_PRESSED )
	{
		blah = !blah;
		FW_Menu_SetPieActive( interactPie, blah );
		return true;
	}
	if ( YnCore_Input_GetButtonStatus( 0, INPUT_X ) == YN_CORE_INPUT_STATE_PRESSED )
	{
		FW_Menu_AddPieOption( interactPie, "testing 4", YnCore_Material_Cache( "materials/ui/pie/cursor.mat.n", YN_CORE_CACHE_GROUP_WORLD, true, false ), NULL );
		return true;
	}

	if ( FW_Menu_HandlePieInput( interactPie ) )
		return true;

	return false;
}
