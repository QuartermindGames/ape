/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 *
 * Purpose: SGUI; Simple Graphical User Interface
 */

#include "sgui.h"
#include "game_interface.h"

#include <plgraphics/plg_camera.h>

#include "client/renderer/renderer.h"
#include "client/renderer/font.h"

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
        "MAIN MENU",
        {
                { "NEW GAME", &newGameMenu, NULL },
                { "SETTINGS", &settingsMenu, NULL },
                { "QUIT", &quitMenu, NULL },
        },
        3,
};

static Menu newGameMenu = {
        "NEW GAME",
        {
                { "EASY" },
                { "NORMAL" },
                { "HARD" },
                { "BACK...", &mainMenu },
        },
        4,
};

static Menu settingsMenu = {
        "SETTINGS",
        {
                { "RESOLUTION" },
        },
        1,
};

static Menu resolutionMenu = {
        "RESOLUTION",
        {
                { "1920X1080" },
                { "1280X720" },
                { "1024X768" },
                { "BACK...", &settingsMenu },
        },
        4,
};

// Quit Menu

static void Menu_Callback_Quit( void )
{
	Engine_Shutdown();
}

static Menu quitMenu = {
        "ARE YOU SURE?",
        {
                { "YES", NULL, Menu_Callback_Quit },
                { "NO", &mainMenu, NULL },
        },
        plArrayElements( quitMenu.options ),
};

static Menu *      currentMenu = &mainMenu;
static BitmapFont *menuFont    = NULL;

void Menu_Initialize( void )
{
	currentMenu = &mainMenu;

	menuFont = Font_CacheBitmap( "materials/ui/fonts/fontx1.mat", 320, 80, 16, 16, 32, 131 );//Font_CacheBitmap( "materials/fonts/font_big_00.mat", 320, 192, 32, 32, 32, 91 );
	if ( menuFont == NULL )
	{
		PrintWarn( "Failed to load default font for menu, using fallback instead!\n" );
		menuFont = Font_GetDefault();
	}
}

void Menu_Shutdown( void )
{
	Font_ReleaseBitmap( menuFont );
}

void Menu_Tick( void )
{
	if ( Game_GetMenuState() != MENU_STATE_START )
		return;

	if ( ( globalSystem.GetKeyState( KEY_DOWN ) && INPUT_STATE_PRESSED ) || 
		 ( globalSystem.GetButtonState( INPUT_DOWN ) && INPUT_STATE_PRESSED ) )
	{
		currentMenu->curSelection++;
		if ( currentMenu->curSelection > currentMenu->numMenuOptions )
			currentMenu->curSelection = 0;

		return;
	}

	if ( ( globalSystem.GetKeyState( KEY_UP ) && INPUT_STATE_PRESSED ) || 
		 ( globalSystem.GetButtonState( INPUT_UP ) && INPUT_STATE_PRESSED ) )
	{
		currentMenu->curSelection--;
		if ( currentMenu->curSelection > currentMenu->numMenuOptions )
			currentMenu->curSelection = currentMenu->numMenuOptions;

		return;
	}

	if ( ( globalSystem.GetKeyState( KEY_ENTER ) && INPUT_STATE_PRESSED ) ||
		 ( globalSystem.GetButtonState( INPUT_A ) && INPUT_STATE_PRESSED ) )
	{
		if ( currentMenu->options[ currentMenu->curSelection ].callback != NULL )
			currentMenu->options[ currentMenu->curSelection ].callback();
		if ( currentMenu->options[ currentMenu->curSelection ].nextMenu != NULL )
			currentMenu = currentMenu->options[ currentMenu->curSelection ].nextMenu;

		return;
	}
}

void Menu_Draw( const PLGViewport *viewport )
{
	if ( currentMenu == NULL )
		return;

#define STR_CENTER( FONT, STRLEN ) ( viewport->w / 2.0f ) - ( ( menuFont->cw * ( STRLEN ) ) / 2.0f );
	float x = STR_CENTER( menuFont, strlen( currentMenu->heading ) );
	Font_DrawBitmapString( menuFont, x, 50.0f, 1.0f, 1.0f, PL_COLOUR_WHITE, currentMenu->heading, false );

	/* make sure the options are aligned to the middle of the screen */
	float y = ( viewport->h / 2.0f ) - menuFont->ch * currentMenu->numMenuOptions;
	for ( uint8_t i = 0; i < currentMenu->numMenuOptions; ++i )
	{
		x = STR_CENTER( menuFont, strlen( currentMenu->options[ i ].string ) );
		if ( i == currentMenu->curSelection )
		{
			Font_DrawBitmapCharacter( menuFont, x - menuFont->cw, y, 1.0f, PL_COLOUR_WHITE, '(' );
		}

		Font_DrawBitmapString( menuFont, x, y, 1.0f, 1.0f, PL_COLOUR_WHITE, currentMenu->options[ i ].string, false );
		y += menuFont->ch;
	}
}
