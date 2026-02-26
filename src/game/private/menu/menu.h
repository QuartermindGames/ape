// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "../game_private.h"

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

#define GAME_MENU_OPTION_BUTTON_SIMPLE( LABEL, NEXT ) { LABEL, NEXT, nullptr, GAME_MENU_OPTION_TYPE_BUTTON }
#define GAME_MENU_OPTION_BUTTON( LABEL, NEXT, CALLBACK, CMD )                  \
	{                                                                          \
		LABEL, NEXT, CALLBACK, GAME_MENU_OPTION_TYPE_BUTTON, .button = { CMD } \
	}
#define GAME_MENU_OPTION_CHECKBOX( LABEL, CALLBACK, VAR )                             \
	{                                                                                 \
		LABEL, nullptr, CALLBACK, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { VAR } \
	}
#define GAME_MENU_OPTION_SEPERATOR() { nullptr, nullptr, nullptr, GAME_MENU_OPTION_TYPE_SEPERATOR }

typedef enum GameMenuFlag
{
	QM_OS_BIT_FLAG( GAME_MENU_FLAG_PROMPT, 0U ),    // appears centered in the screen
	QM_OS_BIT_FLAG( GAME_MENU_FLAG_BACKGROUND, 1U ),// background is visible
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

#define GAME_MENU_IMPLEMENT_ROOT( VAR, HEADING, OPTIONS, FLAGS ) \
	static GameMenu VAR = { HEADING, OPTIONS, QM_OS_ARRAY_ELEMENTS( OPTIONS ), nullptr, FLAGS }
#define GAME_MENU_IMPLEMENT( VAR, HEADING, OPTIONS, PARENT, FLAGS ) \
	static GameMenu VAR = { HEADING, OPTIONS, QM_OS_ARRAY_ELEMENTS( OPTIONS ), PARENT, FLAGS }

void game_menu_initialize();
void game_menu_shutdown();

void game_menu_setup( GameMenu *self );

bool game_menu_is_open();

void game_menu_draw_( const ApeViewport *viewport );

void game_menu_set_title( const char *title );
void game_menu_set_font( ApeGuiFont *font );
void game_menu_set_title_font( ApeGuiFont *font );

void      game_menu_set_active( GameMenu *menu );
GameMenu *game_menu_get_active( void );

/////////////////////////////////////////////////////////////////////////////////////
// Splash Screen
/////////////////////////////////////////////////////////////////////////////////////

typedef struct GameMenuSplash
{
	const char *materialPath;// required
	const char *samplePath;  // optional

	float fadeInTime;
	float fadeOutTime;

	struct
	{
		ApeMaterial    *material;
		ApeAudioSample *sample;
		float           lifetime;
		float           maxLifetime;
	} p;// private data
} GameMenuSplash;

/**
 * Cleanup everything that's queued, essentially skipping.
 */
void game_menu_splash_cleanup_();

/**
 * Queue up all the splash screens you want to show on start-up.
 * @param splashes Array of splash screens you want to display.
 * @param numSplashes The number of splash screens in your array.
 */
void game_menu_splash_setup_queue_( const GameMenuSplash *splashes, unsigned int numSplashes );

/**
 * Check if the splash screens are still displaying or not.
 * @return True if the splash screens are done displaying.
 */
bool game_menu_splash_is_complete_();

/**
 * Simulate the splash screens as they're displayed.
 * (automatically skips if splash is complete).
 * @param delta Delta time.
 */
void game_menu_splash_tick_( double delta );

/**
 * Draw the splash screen.
 * @param viewport Active viewport we're drawing into.
 */
void game_menu_splash_draw_( const ApeViewport *viewport );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
