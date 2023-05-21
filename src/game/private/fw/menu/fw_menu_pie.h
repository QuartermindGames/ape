// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "fw_menu.h"

typedef struct FWPieMenu FWPieMenu;
typedef struct FWPieMenuOption FWPieMenuOption;

typedef void ( *FWPieMenuOptionCallback )( FWPieMenu *menu, FWPieMenuOption *option, void *userData );

/**
 * Creates a new pie menu object.
 */
FWPieMenu *FW_Menu_CreatePie( void );

/**
 * Destroys the pie menu object and destroys all options.
 */
void FW_Menu_DestroyPie( FWPieMenu *menu );

/**
 * Simulate/animate the pie menu.
 */
void FW_Menu_TickPie( FWPieMenu *menu );

/**
 * Allows for the pie menu to catch any input events,
 * returns true if any are handled.
 */
bool FW_Menu_HandlePieInput( FWPieMenu *menu );

/**
 * Draws the pie menu to the screen - will always be centered.
 */
void FW_Menu_DrawPie( FWPieMenu *menu, float x, float y );

/**
 * Activate the pie menu - by default a menu isn't active, and therefore
 * won't be rendered or interactive.
 */
void FW_Menu_SetPieActive( FWPieMenu *menu, bool active );

/**
 * Adds an option to the pie menu.
 */
FWPieMenuOption *FW_Menu_AddPieOption( FWPieMenu *menu, const char *label, struct OgeMaterial *icon, FWPieMenuOptionCallback callback );

/**
 * Destroys the specific pie menu option.
 */
void FW_Menu_DestroyPieOption( FWPieMenuOption *option );
