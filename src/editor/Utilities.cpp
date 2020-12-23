/**
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
**/

#include "qe3.h"

FXIcon *huang::util::LoadImageIcon( FXApp *app, const char *path ) {
	FXIconSource iconSource( app );
	return iconSource.loadIconFile( path );
}

/**
 * Pull in a list of menu items and add them onto the given menu bar.
 */
FXMenuPane *huang::util::CreateMenus( FXApp *app, FXMenuBar *menuBar, const char *menuName, MenuItem *items ) {
	FXMenuPane *menu = new FXMenuPane( menuBar->getParent() );
	
	MenuItem *curMenuItem = items;
	while( curMenuItem->label != nullptr ) {
		FXObject *target = app;
		if( curMenuItem->target != nullptr ) {
			target = curMenuItem->target;
		}

		switch( curMenuItem->type ) {
		case MenuType::CHECKBOX:
			new FXMenuCheck( menu, curMenuItem->label, target, curMenuItem->selector );
			break;
		case MenuType::COMMAND:
			new FXMenuCommand( menu, curMenuItem->label, curMenuItem->icon, target, curMenuItem->selector );
			break;
		case MenuType::RADIO:
			new FXMenuRadio( menu, curMenuItem->label, target, curMenuItem->selector );
			break;
		case MenuType::SEPERATOR:
			new FXSeparator( menu );
			break;
		}

		curMenuItem++;
	}

	new FXMenuTitle( menuBar, menuName, nullptr, menu );

	return menu;
}
