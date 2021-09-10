/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 *
 * Purpose: SGUI; Simple Graphical User Interface
 */

#include "sgui.h"
#include "game_interface.h"
#include "actor.h"

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

#define MENU_WIDTH	640
#define MENU_HEIGHT 480

#define MAX_MENU_ITEMS 32

typedef void ( *MenuCallback )( void );

typedef struct MenuOption
{
	const char *string;
	struct Menu *nextMenu;
	MenuCallback callback;
} MenuOption;

typedef struct Menu
{
	const char *heading;
	const MenuOption options[ MAX_MENU_ITEMS ];
	uint8_t numMenuOptions;

	uint8_t curSelection;
} Menu;

static Menu mainMenu;
static Menu *currentMenu = &mainMenu;

static Menu newGameMenu;
static Menu settingsMenu;
static Menu creditsMenu;
static Menu quitMenu;
static Menu mainMenu = {
		"MAIN MENU",
		{
				{ "START GAME", &newGameMenu, NULL },
				{ "SETTINGS", &settingsMenu, NULL },
				{ "CREDITS", &creditsMenu, NULL },
				{ "QUIT", &quitMenu, NULL },
		},
		4,
};

static void Menu_CB_StartGame( void )
{
	GameDifficulty difficulty;
	switch ( currentMenu->curSelection )
	{
		default:
		case 0:
			difficulty = GAME_DIFFICULTY_EASY;
			break;
		case 1:
			difficulty = GAME_DIFFICULTY_NORMAL;
			break;
		case 2:
			difficulty = GAME_DIFFICULTY_HARD;
			break;
	}

	Game_SetDifficultyMode( difficulty );

	PlParseConsoleString( "world worlds/arena.node" );
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

// Credits Menu

static Menu creditsMenu = {
		"CREDITS",
		{
			{ "CREATED BY", NULL, NULL },
			{ "  MARK SOWDEN", NULL, NULL },
			{ "SPECIAL THANKS", NULL, NULL },
			{ "  DANIEL COLLINS", NULL, NULL },
			{ "BACK...", &mainMenu },
		},
		5,
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

static Material *hudMaterial;
static PLGMesh *hudMesh;

static PLGFrameBuffer *mRTBuffer = NULL;
static PLGTexture *mRTAttachment = NULL;
PLGTexture *Menu_GetRenderTargetAttachment( void )
{
	return mRTAttachment;
}

void Menu_Initialize( void )
{
	currentMenu = &mainMenu;

	menuFont = Font_CacheBitmap( "materials/ui/fonts/big_00.mat", 320, 192, 32, 32, 32, 91 );

	hudMaterial = RM_CacheMaterial( "materials/ui/hud.mat", CACHE_GROUP_WORLD, true );
	hudMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, 100, 100 );

	R_SetupRenderTarget( &mRTBuffer, &mRTAttachment, MENU_WIDTH, MENU_HEIGHT );
}

void Menu_Shutdown( void )
{
	Font_ReleaseBitmap( menuFont );
}

bool Menu_HandleKeyboardEvent( int key, OSInputState keyState )
{
	if ( Game_GetMenuState() != MENU_STATE_START )
		return false;

	if ( keyState != INPUT_STATE_NONE )
		return false;

	switch ( key )
	{
		default: break;
		case 's':
		case KEY_DOWN:
			currentMenu->curSelection++;
			if ( currentMenu->curSelection >= currentMenu->numMenuOptions )
				currentMenu->curSelection = 0;
			return true;
		case 'w':
		case KEY_UP:
			if ( currentMenu->curSelection == 0 )
				currentMenu->curSelection = currentMenu->numMenuOptions - 1;
			else
				currentMenu->curSelection--;
			return true;
		case KEY_LEFT_CTRL:
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

/* texture width and height, crudely hard-coded here... */
static const int hudSheetW = 256;
static const int hudSheetH = 256;

typedef struct HUDElementLayout
{
	int x, y, w, h;
} HUDElementLayout;

typedef enum HUDElement
{
	HUD_ELEMENT_BAR_BG_L,
	HUD_ELEMENT_BAR_BG_M,
	HUD_ELEMENT_BAR_BG_R,

	HUD_ELEMENT_BAR_HP_L,
	HUD_ELEMENT_BAR_HP_M,
	HUD_ELEMENT_BAR_HP_R,

	HUD_ELEMENT_BAR_DMG_L,
	HUD_ELEMENT_BAR_DMG_M,
	HUD_ELEMENT_BAR_DMG_R,

	HUD_ELEMENT_ICON_HP,
	HUD_ELEMENT_ICON_CHAR,

	MAX_HUD_ELEMENTS
} HUDElement;

static const HUDElementLayout hudElementLayouts[ MAX_HUD_ELEMENTS ] = {
		[HUD_ELEMENT_BAR_BG_L] = { 8, 8, 8, 32 },
		[HUD_ELEMENT_BAR_BG_M] = { 16, 8, 16, 32 },
		[HUD_ELEMENT_BAR_BG_R] = { 32, 8, 8, 32 },

		[HUD_ELEMENT_BAR_HP_L] = { 48, 8, 8, 32 },
		[HUD_ELEMENT_BAR_HP_M] = { 56, 8, 16, 32 },
		[HUD_ELEMENT_BAR_HP_R] = { 72, 8, 8, 32 },

		[HUD_ELEMENT_BAR_DMG_L] = { 88, 8, 8, 32 },
		[HUD_ELEMENT_BAR_DMG_M] = { 96, 8, 16, 32 },
		[HUD_ELEMENT_BAR_DMG_R] = { 112, 8, 8, 32 },

		[HUD_ELEMENT_ICON_HP] = { 120, 8, 40, 32 },
		[HUD_ELEMENT_ICON_CHAR] = { 8, 40, 104, 112 },
};

static void GetUVCoordsForElement( HUDElement element, float *tw, float *th, float *tx, float *ty )
{
	*tw = ( float ) hudElementLayouts[ element ].w / ( float ) hudSheetW;
	*th = ( float ) hudElementLayouts[ element ].h / ( float ) hudSheetH;
	*tx = ( float ) hudElementLayouts[ element ].x / ( float ) hudSheetW;
	*ty = ( float ) hudElementLayouts[ element ].y / ( float ) hudSheetH;
}

static void Menu_DrawElement( HUDElement element, int x, int y, int w, int h )
{
	float tw, th, tx, ty;
	GetUVCoordsForElement( element, &tw, &th, &tx, &ty );

	unsigned int vX = PlgAddMeshVertex( hudMesh, PLVector3( x, y, 0 ), pl_vecOrigin3, PLColourRGB( 255, 255, 255 ), PLVector2( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( hudMesh, PLVector3( x, y + h, 0 ), pl_vecOrigin3, PLColourRGB( 255, 255, 255 ), PLVector2( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( hudMesh, PLVector3( x + w, y, 0 ), pl_vecOrigin3, PLColourRGB( 255, 255, 255 ), PLVector2( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( hudMesh, PLVector3( x + w, y + h, 0 ), pl_vecOrigin3, PLColourRGB( 255, 255, 255 ), PLVector2( tx + tw, ty + th ) );

	PlgAddMeshTriangle( hudMesh, vX, vY, vZ );
	PlgAddMeshTriangle( hudMesh, vZ, vY, vW );
}

static void Menu_DrawHUDBar( HUDElement element, int x, int y, int w, int h )
{
	if ( w <= 0 )
		return;

	Menu_DrawElement( element, x, y, hudElementLayouts[ element ].w, h );
	x += hudElementLayouts[ element ].w;

	Menu_DrawElement( element + 1, x, y, w, h );
	x += w;

	Menu_DrawElement( element + 2, x, y, hudElementLayouts[ element + 2 ].w, h );
}

#define BORDER_MARGIN 20

static void Menu_BeginDrawHUD( void )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgClearMesh( hudMesh );
}

static void Menu_EndDrawHUD( void )
{
	RM_DrawMesh( hudMaterial, hudMesh );

	PlPopMatrix();
}

#define STR_CENTER( FONT, STRLEN ) ( viewport->w / 2 ) - ( ( menuFont->cw * ( STRLEN ) ) / 2 )

static void Menu_DrawHUD( const PLGViewport *viewport )
{
	Menu_BeginDrawHUD();

	Menu_DrawElement( HUD_ELEMENT_ICON_CHAR, BORDER_MARGIN, viewport->h - hudElementLayouts[ HUD_ELEMENT_ICON_CHAR ].h - BORDER_MARGIN, 104, 112 );

	Menu_DrawHUDBar( HUD_ELEMENT_BAR_BG_L, BORDER_MARGIN + 72, viewport->h - 90 - BORDER_MARGIN, 242, hudElementLayouts[ HUD_ELEMENT_BAR_BG_L ].h );
	Menu_DrawElement( HUD_ELEMENT_ICON_HP, BORDER_MARGIN + 72, viewport->h - 90 - BORDER_MARGIN, hudElementLayouts[ HUD_ELEMENT_ICON_HP ].w, hudElementLayouts[ HUD_ELEMENT_ICON_HP ].h );

	Menu_DrawHUDBar( HUD_ELEMENT_BAR_DMG_L, BORDER_MARGIN + 72 + hudElementLayouts[ HUD_ELEMENT_ICON_HP ].w, viewport->h - 87 - BORDER_MARGIN, 200, hudElementLayouts[ HUD_ELEMENT_BAR_DMG_L ].h - 5 );

	char scoreBuf[ 32 ] = "SCORE: ";

	int16_t health = 0;
	Actor *player = Act_GetByTag( "player", NULL );
	if ( player != NULL )
	{
		health = player->health;
		if ( health < 0 )
			health = 0;
		else if ( health > 100 )
			health = 100;

		char num[ 8 ];
		pl_itoa( player->score, num, sizeof( num ), 10 );
		strcat( scoreBuf, num );
	}

	Menu_DrawHUDBar( HUD_ELEMENT_BAR_HP_L, BORDER_MARGIN + 72 + hudElementLayouts[ HUD_ELEMENT_ICON_HP ].w, viewport->h - 87 - BORDER_MARGIN, 200 / 100 * health, hudElementLayouts[ HUD_ELEMENT_BAR_HP_L ].h - 5 );

	Menu_EndDrawHUD();

	if ( health <= 0 )
	{
		static const char *deathMsg = "SHIP DESTROYED";
		Font_DrawBitmapString( menuFont, ( float ) STR_CENTER( menuFont, strlen( deathMsg ) ), 50.0f, 1.0f, 1.0f, PL_COLOUR_WHITE, deathMsg, false );
	}

	Font_DrawBitmapString( menuFont, 5.0f, 5.0f, 1.0f, 0.5f, PL_COLOUR_CYAN, scoreBuf, true );
}

void Menu_Draw( PLGViewport *viewport )
{
	if ( Game_GetMenuState() == MENU_STATE_HUD )
	{
		Menu_DrawHUD( viewport );
		return;
	}

	R_Set2DViewportSize( MENU_WIDTH, MENU_HEIGHT );

	PlgBindFrameBuffer( mRTBuffer, PLG_FRAMEBUFFER_DRAW );
	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );

	if ( currentMenu == NULL )
		return;

	int x = STR_CENTER( menuFont, ( int ) strlen( currentMenu->heading ) );
	Font_DrawBitmapString( menuFont, ( float ) x, 50.0f, 1.0f, 1.0f, PL_COLOUR_WHITE, currentMenu->heading, false );

	/* make sure the options are aligned to the middle of the screen */
	int y = ( viewport->h / 2 ) - menuFont->ch * currentMenu->numMenuOptions;
	for ( uint8_t i = 0; i < currentMenu->numMenuOptions; ++i )
	{
		x = STR_CENTER( menuFont, ( int ) strlen( currentMenu->options[ i ].string ) );
		if ( i == currentMenu->curSelection )
			Font_DrawBitmapCharacter( menuFont, ( float ) ( x - menuFont->cw ), ( float ) y, 1.0f, PL_COLOUR_WHITE, '(' );

		Font_DrawBitmapString( menuFont, ( float ) x, ( float ) y, 1.0f, 1.0f, PL_COLOUR_WHITE, currentMenu->options[ i ].string, false );
		y += menuFont->ch;
	}

	/* scanline noise and other tv-like effects */

	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_VERTEX ] );

	if ( rand() % 65 == 0 )
	{
		int l = rand() % 4;
		for ( unsigned int i = 0; i < l; ++i )
		{
			PLRectangle2D rectangle2D;
			rectangle2D.ll = PL_COLOUR_WHITE;
			rectangle2D.lr = PL_COLOUR_WHITE;
			rectangle2D.ul = PL_COLOUR_WHITE;
			rectangle2D.ur = PL_COLOUR_WHITE;
			y = rand() % viewport->h;
			rectangle2D.xy = PLVector2( 0.0f, y );
			rectangle2D.wh = PLVector2( viewport->w, rand() % 5 + 2 );
			PlgDrawFilledRectangle( &rectangle2D );
		}
	}

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );

	R_Restore2DViewportSize();
}
