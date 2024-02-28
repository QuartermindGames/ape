// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Desktop environment API for APE

#include "gui_private.h"

/***********************************************************/
// GUI MENU
// Overhead menu that provides contextual actions.
/***********************************************************/

typedef enum GuiMenuItemType
{
	GUI_MENU_ITEM_ACTION,
	GUI_MENU_ITEM_SUBMENU,

	GUI_MAX_MENU_ITEM_TYPES
} GuiMenuItemType;

typedef struct GuiMenuItem
{
	GuiPanel *panel;
	char *name;
	GuiMenuItemType type;
	void *user;
} GuiMenuItem;

typedef struct GuiMenu
{
	uint8_t numItems;
} GuiMenu;

typedef struct GuiDesktop
{
	GuiPanel *panel;
	GuiMenu *menu;
} GuiDesktop;

/***********************************************************/
// GUI DESKTOP
/***********************************************************/

GuiDesktop *guiCreateDesktop( GuiPanel *parent )
{
	if ( parent == NULL )
	{
		parent = ss_gui_panel_create( NULL, 0, 0, 640, 480, GUI_PANEL_BACKGROUND_DEFAULT, GUI_PANEL_BORDER_NONE );
		if ( parent == NULL )
		{
			GUI_WARNING( "Failed to create root panel for desktop!\n" );
			return NULL;
		}
	}

	GuiDesktop *desktop = PL_NEW( GuiDesktop );
	desktop->panel = parent;

	return desktop;
}

/**
 * Destroy the given desktop handle and it's
 * children.
 */
void guiDestroyDesktop( GuiDesktop *desktop )
{
	if ( desktop == NULL )
	{
		return;
	}

	ss_gui_panel_destroy( desktop->panel );

	PL_DELETE( desktop );
}
