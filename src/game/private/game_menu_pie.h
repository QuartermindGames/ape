// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Delicious pie menu!

typedef struct GamePieMenu GamePieMenu;
typedef struct GamePieMenuOption GamePieMenuOption;

typedef void ( *GamePieMenuOptionCallback )( GamePieMenu *menu, GamePieMenuOption *option, void *userData );

/**
 * Creates a new pie menu object.
 */
GamePieMenu *menu_pie_create( void );

/**
 * Destroys the pie menu object and destroys all options.
 */
void menu_pie_destroy( GamePieMenu *menu );

/**
 * Simulate/animate the pie menu.
 */
void menu_pie_tick( GamePieMenu *menu );

/**
 * Allows for the pie menu to catch any input events,
 * returns true if any are handled.
 */
bool menu_pie_handle_input( GamePieMenu *menu );

/**
 * Draws the pie menu to the screen - will always be centered.
 */
void menu_pie_draw( GamePieMenu *menu, float x, float y );

/**
 * Activate the pie menu - by default a menu isn't active, and therefore
 * won't be rendered or interactive.
 */
void menu_pie_make_active( GamePieMenu *menu, bool active );

/**
 * Adds an option to the pie menu.
 */
GamePieMenuOption *menu_pie_add_option( GamePieMenu *menu, const char *label, struct ApeMaterial *icon, GamePieMenuOptionCallback callback );

/**
 * Destroys the specific pie menu option.
 */
void menu_pie_destroy_option( GamePieMenuOption *option );
