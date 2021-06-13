/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 *
 * Purpose: SGUI; Simple Graphical User Interface
 */

#include "sgui.h"
#include "game_interface.h"

typedef enum SGUIWidgetType
{
	SGUI_WIDGETTYPE_PANEL,
} SGUIWidgetType;

typedef struct SGUIWidget
{
	int x, y;

} SGUIWidget;

static PLLinkedList *sguiWidgets;

void SGUI_Initialize( void )
{
	sguiWidgets = PlCreateLinkedList();
	if ( sguiWidgets == NULL )
		PrintError( "Failed to create sgui list!\nPL: %s\n", PlGetError() );
}

void SGUI_Tick( void )
{
}

void SGUI_Draw( void )
{
}

/****************************************
 * Temporary Menu System ...
 ****************************************/

#define MAX_MENU_ITEMS 32

typedef void ( *MenuCallback )( void );

typedef struct MenuOption
{
	const char * string;
	struct Menu *nextMenu;
	MenuCallback callback;
} MenuOption;

typedef struct Menu
{
	const char *     heading;
	const MenuOption options[ MAX_MENU_ITEMS ];
	uint8_t          numMenuOptions;

	uint8_t curSelection;
} Menu;

static Menu newGameMenu;
static Menu settingsMenu;
static Menu quitMenu;
static Menu mainMenu = {
        "Main Menu",
        {
                { "New Game", &newGameMenu, NULL },
                { "Settings", &settingsMenu, NULL },
                { "Quit", &quitMenu, NULL },
        },
        3,
};

static Menu newGameMenu = {
        "New Game",
        {
                { "Easy" },
                { "Normal" },
                { "Hard" },
                { "Back...", &mainMenu },
        },
        4,
};

static Menu settingsMenu = {
        "Settings",
        {
                { "Resolution" },
        },
        1,
};

static Menu resolutionMenu = {
        "Resolution",
        {
                { "1920x1080" },
                { "1280x720" },
                { "1024x768" },
                { "Back...", &settingsMenu },
        },
        4,
};

// Quit Menu

static void Menu_Callback_Quit( void )
{
	Engine_Shutdown();
}

static Menu quitMenu = {
        "Are you sure?",
        {
                { "Yes", NULL, Menu_Callback_Quit },
                { "No", &mainMenu, NULL },
        },
        plArrayElements( quitMenu.options ),
};

static Menu *currentMenu = &mainMenu;

void Menu_Initialize( void )
{
	currentMenu = &mainMenu;
}

void Menu_Tick( void )
{
	if ( Game_GetMenuState() != MENU_STATE_START )
		return;

	if ( globalSystem.GetKeyState( KEY_DOWN ) || globalSystem.GetButtonState( INPUT_DOWN ) )
	{
		currentMenu->curSelection++;
		if ( currentMenu->curSelection > currentMenu->numMenuOptions )
			currentMenu->curSelection = 0;

		return;
	}

	if ( globalSystem.GetKeyState( KEY_UP ) || globalSystem.GetButtonState( INPUT_UP ) )
	{
		currentMenu->curSelection--;
		if ( currentMenu->curSelection > currentMenu->numMenuOptions )
			currentMenu->curSelection = currentMenu->numMenuOptions;

		return;
	}

	if ( globalSystem.GetKeyState( KEY_ENTER ) || globalSystem.GetButtonState( INPUT_A ) )
	{
		if ( currentMenu->options[ currentMenu->curSelection ].callback != NULL )
			currentMenu->options[ currentMenu->curSelection ].callback();
		if ( currentMenu->options[ currentMenu->curSelection ].nextMenu != NULL )
			currentMenu = currentMenu->options[ currentMenu->curSelection ].nextMenu;

		return;
	}
}

void Menu_Draw( void )
{
}
