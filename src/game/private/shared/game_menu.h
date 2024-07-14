// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef struct MenuOption MenuOption;

typedef void ( *MenuCallback )( const MenuOption *option );

typedef enum MenuOptionType
{
	MENU_OPTION_TYPE_BUTTON,     //text-based button
	MENU_OPTION_TYPE_BUTTON_ICON,//button represented by icon
	MENU_OPTION_TYPE_CHECKBOX,   //typical checkbox
	MENU_OPTION_TYPE_SLIDER,     //and typical slider
} MenuOptionType;

typedef struct MenuOption
{
	const char    *string;
	struct Menu   *nextMenu;
	MenuCallback   callback;
	MenuOptionType type;
	const char    *command;
	union
	{
		bool    checkbox;
		uint8_t slider;
	};
} MenuOption;

typedef struct Menu
{
	const char       *heading;
	const MenuOption *options;
	uint8_t           numOptions;
	struct Menu      *parent;
	uint8_t           lastOption;
} Menu;

void  Game_Menu_SetCurrent( Menu *menu );
Menu *Game_Menu_GetCurrent( void );
