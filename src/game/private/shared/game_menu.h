// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

void gameInitializeMenu( void );
void gameShutdownMenu( void );

typedef void ( *MenuCallback )( void );

typedef enum MenuOptionType {
	MENU_OPTION_TYPE_LABEL,      //static label, will be skipped during selection
	MENU_OPTION_TYPE_BUTTON,     //text-based button
	MENU_OPTION_TYPE_BUTTON_ICON,//button represented by icon
	MENU_OPTION_TYPE_CHECKBOX,   //typical checkbox
	MENU_OPTION_TYPE_SLIDER,     //and typical slider
} MenuOptionType;

typedef struct MenuOption {
	const char *string;
	struct Menu *nextMenu;
	MenuCallback callback;
	MenuOptionType type;
} MenuOption;

typedef struct Menu {
	const char *heading;
	const MenuOption *options;
	uint8_t numOptions;
} Menu;

extern uint8_t menuOptionSelection;

void Game_Menu_SetCurrent( Menu *menu );
Menu *Game_Menu_GetCurrent( void );
