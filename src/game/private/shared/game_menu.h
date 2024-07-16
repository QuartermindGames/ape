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
	MENU_OPTION_TYPE_SEPERATOR,
} MenuOptionType;

typedef struct MenuOption
{
	const char    *string;
	struct Menu   *nextMenu;
	MenuCallback   callback;
	MenuOptionType type;
	union
	{
		struct
		{
			const char        *varName;
			PLConsoleVariable *var;
		} checkbox;
		struct
		{
			const char *command;
		} button;
	};
} MenuOption;

typedef struct Menu
{
	const char  *heading;
	MenuOption  *options;
	uint8_t      numOptions;
	struct Menu *parent;
	uint8_t      lastOption;
} Menu;

void  Game_Menu_SetCurrent( Menu *menu );
Menu *Game_Menu_GetCurrent( void );
