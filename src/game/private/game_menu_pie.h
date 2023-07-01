// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Delicious pie menu!

typedef struct GamePieMenu GamePieMenu;
typedef struct GamePieMenuOption GamePieMenuOption;

typedef void ( *GamePieMenuOptionCallback )( GamePieMenu *menu, GamePieMenuOption *option, void *userData );

/**
 * Creates a new pie menu object.
 */
GamePieMenu *gameCreatePieMenu( void );

/**
 * Destroys the pie menu object and destroys all options.
 */
void gameDestroyPieMenu( GamePieMenu *menu );

/**
 * Simulate/animate the pie menu.
 */
void gameTickPieMenu( GamePieMenu *menu );

/**
 * Allows for the pie menu to catch any input events,
 * returns true if any are handled.
 */
bool gameHandlePieMenuInput( GamePieMenu *menu );

/**
 * Draws the pie menu to the screen - will always be centered.
 */
void gameDrawPieMenu( GamePieMenu *menu, float x, float y );

/**
 * Activate the pie menu - by default a menu isn't active, and therefore
 * won't be rendered or interactive.
 */
void gameSetActivePieMenu( GamePieMenu *menu, bool active );

/**
 * Adds an option to the pie menu.
 */
GamePieMenuOption *gameAddPieMenuOption( GamePieMenu *menu, const char *label, struct ApeMaterial *icon, GamePieMenuOptionCallback callback );

/**
 * Destroys the specific pie menu option.
 */
void gameDestroyPieMenuOption( GamePieMenuOption *option );
