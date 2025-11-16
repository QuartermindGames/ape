// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

typedef struct GameMenu       GameMenu;
typedef struct GameMenuOption GameMenuOption;

typedef void ( *GameMenuCallback )( const GameMenuOption *option );

typedef enum GameMenuOptionType
{
	GAME_MENU_OPTION_TYPE_BUTTON,     //text-based button
	GAME_MENU_OPTION_TYPE_BUTTON_ICON,//button represented by icon
	GAME_MENU_OPTION_TYPE_CHECKBOX,   //typical checkbox
	GAME_MENU_OPTION_TYPE_SLIDER,     //and typical slider
	GAME_MENU_OPTION_TYPE_SEPERATOR,
} GameMenuOptionType;

typedef struct GameMenuOption
{
	const char        *string;
	GameMenu          *nextMenu;
	GameMenuCallback   callback;
	GameMenuOptionType type;
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
} GameMenuOption;

typedef enum GameMenuFlag
{
	PL_BITFLAG( GAME_MENU_FLAG_PROMPT, 0U ),    // appears centered in the screen
	PL_BITFLAG( GAME_MENU_FLAG_BACKGROUND, 1U ),// background is visible
} GameMenuFlag;

typedef struct GameMenu
{
	const char     *heading;
	GameMenuOption *options;
	uint8_t         numOptions;
	GameMenu       *parent;
	uint16_t        flags;
	uint8_t         lastOption;
} GameMenu;

bool game_menu_is_open();

void      game_menu_set_active( GameMenu *menu );
GameMenu *game_menu_get_active( void );

PL_EXTERN_C_END
