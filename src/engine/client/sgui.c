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

/* ======================================================================
 * Temporary menu system...
 * ====================================================================*/

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
	const char *	 heading;
	const MenuOption options[ MAX_MENU_ITEMS ];
	uint8_t			 numMenuOptions;

	uint8_t curSelection;
} Menu;

static Menu	 mainMenu;
static Menu *currentMenu = &mainMenu;

static Menu newGameMenu;
static Menu settingsMenu;
static Menu quitMenu;
static Menu mainMenu = {
		"MAIN MENU",
		{
				{ "START GAME", &newGameMenu, NULL },
				{ "SETTINGS", &settingsMenu, NULL },
				{ "QUIT", &quitMenu, NULL },
		},
		3,
};

static void Menu_CB_StartGame( void )
{
}

static Menu newGameMenu = {
		"START GAME",
		{
				{ "EASY", NULL, Menu_CB_StartGame },
				{ "NORMAL", NULL, Menu_CB_StartGame },
				{ "HARD", NULL, Menu_CB_StartGame },
				{ "BACK...", &mainMenu },
		},
		4,
};

static void Menu_CB_SetResolution( void )
{
	int w;
	int h;

	switch ( currentMenu->curSelection )
	{
		case 0:
			w = 1920;
			h = 1080;
			break;
		case 1:
			w = 1280;
			h = 720;
			break;
		case 2:
			w = 1024;
			h = 768;
			break;
	}

	CallSystemFunction( SetDisplaySize, &w, &h );
}

static Menu resolutionMenu = {
		"RESOLUTION",
		{
				{ "1920X1080", NULL, Menu_CB_SetResolution },
				{ "1280X720", NULL, Menu_CB_SetResolution },
				{ "1024X768", NULL, Menu_CB_SetResolution },
				{ "BACK...", &settingsMenu },
		},
		4,
};

static Menu settingsMenu = {
		"SETTINGS",
		{
				{ "RESOLUTION", &resolutionMenu },
				{ "BACK...", &mainMenu },
		},
		2,
};

// Quit Menu

static void Menu_CB_Quit( void )
{
	Engine_Shutdown();
}

static Menu quitMenu = {
		"ARE YOU SURE?",
		{
				{ "YES", NULL, Menu_CB_Quit },
				{ "NO", &mainMenu, NULL },
		},
		2,
};

static BitmapFont *menuFont;
static BitmapFont *hudFont;

static Material *hudMaterial;

void Menu_Initialize( void )
{
	currentMenu = &mainMenu;

	menuFont = Font_CacheBitmap( "materials/ui/fonts/big_00.mat", 320, 192, 32, 32, 32, 91 );
	hudFont	 = Font_CacheBitmap( "materials/ui/fonts/x1.mat", 320, 80, 16, 16, 32, 131 );

	hudMaterial = RM_CacheMaterial( "materials/ui/hud.mat", CACHE_GROUP_WORLD, true );
}

void Menu_Shutdown( void )
{
	Font_ReleaseBitmap( menuFont );
	Font_ReleaseBitmap( hudFont );
}

bool Menu_HandleKeyboardEvent( int key, OSInputState keyState )
{
	if ( Game_GetMenuState() != MENU_STATE_START )
		return false;

	if ( keyState != INPUT_STATE_PRESSED )
		return false;

	switch ( key )
	{
		default: break;
		case KEY_DOWN:
			currentMenu->curSelection++;
			if ( currentMenu->curSelection >= currentMenu->numMenuOptions )
				currentMenu->curSelection = 0;
			return true;
		case KEY_UP:
			if ( currentMenu->curSelection == 0 )
				currentMenu->curSelection = currentMenu->numMenuOptions - 1;
			else
				currentMenu->curSelection--;
			return true;
		case KEY_ENTER:
			if ( currentMenu->options[ currentMenu->curSelection ].callback != NULL )
				currentMenu->options[ currentMenu->curSelection ].callback();
			if ( currentMenu->options[ currentMenu->curSelection ].nextMenu != NULL )
				currentMenu = currentMenu->options[ currentMenu->curSelection ].nextMenu;

			return true;
	}

	return false;
}

#if 0 /* todo: reintroduce once input shit is better */
void Menu_Tick( void )
{
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
#endif

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
			Font_DrawBitmapCharacter( menuFont, x - menuFont->cw, y, 1.0f, PL_COLOUR_WHITE, '(' );

		Font_DrawBitmapString( menuFont, x, y, 1.0f, 1.0f, PL_COLOUR_WHITE, currentMenu->options[ i ].string, false );
		y += menuFont->ch;
	}
}
