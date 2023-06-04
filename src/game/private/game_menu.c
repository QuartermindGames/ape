// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "game_private.h"
#include "game_menu.h"

uint8_t menuOptionSelection = 0;

static Menu *currentMenu = NULL;

void Game_Menu_SetCurrent( Menu *menu )
{
	currentMenu = menu;
	menuOptionSelection = 0;
}

Menu *Game_Menu_GetCurrent( void )
{
	return currentMenu;
}

static void Menu_CB_StartGame( void )
{
#if 0
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
#endif

	PlParseConsoleString( "world arena" );
}

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

static const PLQuad hudElementLayouts[ MAX_HUD_ELEMENTS ] = {
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

void Menu_DrawHUDBar( const PLQuad *layouts, HUDElement element, int x, int y, int w, int h )
{
#if 0
	if ( w <= 0 )
		return;

	Menu_DrawElement( layouts, element, x, y, hudElementLayouts[ element ].w, h );
	x += hudElementLayouts[ element ].w;

	Menu_DrawElement( layouts, element + 1, x, y, w, h );
	x += w;

	Menu_DrawElement( layouts, element + 2, x, y, hudElementLayouts[ element + 2 ].w, h );
#endif
}

#define BORDER_MARGIN 20

#define STR_CENTER( FONT, STRLEN ) ( viewport->w / 2 ) - ( ( menuFont->cw * ( STRLEN ) ) / 2 )

static void Menu_DrawHUD( const ApeViewport *viewport )
{
#if 0// old crap
	Menu_DrawElement( NULL, HUD_ELEMENT_ICON_CHAR, BORDER_MARGIN, viewport->h - hudElementLayouts[ HUD_ELEMENT_ICON_CHAR ].h - BORDER_MARGIN, 104, 112 );

	Menu_DrawHUDBar( NULL, HUD_ELEMENT_BAR_BG_L, BORDER_MARGIN + 72, viewport->h - 90 - BORDER_MARGIN, 242, hudElementLayouts[ HUD_ELEMENT_BAR_BG_L ].h );
	Menu_DrawElement( NULL, HUD_ELEMENT_ICON_HP, BORDER_MARGIN + 72, viewport->h - 90 - BORDER_MARGIN, hudElementLayouts[ HUD_ELEMENT_ICON_HP ].w, hudElementLayouts[ HUD_ELEMENT_ICON_HP ].h );

	Menu_DrawHUDBar( NULL, HUD_ELEMENT_BAR_DMG_L, BORDER_MARGIN + 72 + hudElementLayouts[ HUD_ELEMENT_ICON_HP ].w, viewport->h - 87 - BORDER_MARGIN, 200, hudElementLayouts[ HUD_ELEMENT_BAR_DMG_L ].h - 5 );

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

	Menu_DrawHUDBar( NULL, HUD_ELEMENT_BAR_HP_L, BORDER_MARGIN + 72 + hudElementLayouts[ HUD_ELEMENT_ICON_HP ].w, viewport->h - 87 - BORDER_MARGIN, 200 / 100 * health, hudElementLayouts[ HUD_ELEMENT_BAR_HP_L ].h - 5 );

	Menu_EndDrawHUD();

	if ( health <= 0 )
	{
		static const char *deathMsg = "SHIP DESTROYED";
		Font_DrawBitmapString( menuFont, ( float ) STR_CENTER( menuFont, strlen( deathMsg ) ), 50.0f, 1.0f, 1.0f, PL_COLOUR_WHITE, deathMsg, false );
	}

	Font_DrawBitmapString( menuFont, 5.0f, 5.0f, 1.0f, 0.5f, PL_COLOUR_CYAN, scoreBuf, true );
#endif
}

void Menu_Draw( const ApeViewport *viewport )
{
#if 0
	if ( Game_GetMenuState() == MENU_STATE_HUD )
	{
		Menu_DrawHUD( viewport );
		return;
	}

	if ( menuFont == NULL )
		return;

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
#endif
}
