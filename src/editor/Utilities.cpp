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
	const char *dataDir = ComFS_GetDataDirectory();
    char fullPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( fullPath, sizeof( fullPath ), "%s%s", dataDir, path );

	FXIconSource iconSource( app );
	return iconSource.loadIconFile( fullPath );
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

const char *huang::util::reg::ReadString( const char *section, const char *key, const char *def ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().readStringEntry( section, key, def );
}

int huang::util::reg::ReadInt( const char *section, const char *key, int def ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().readIntEntry( section, key, def );
}

bool huang::util::reg::ReadBool( const char *section, const char *key, bool def ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().readBoolEntry( section, key, def );
}

float huang::util::reg::ReadFloat( const char *section, const char *key, float def ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().readRealEntry( section, key, def );
}

FXColor huang::util::reg::ReadColour( const char *section, const char *key, FXColor def ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().readColorEntry( section, key, def );
}

int huang::util::reg::ReadColourF( const char *section, const char *key, vec3_t out, const vec3_t def ) {
	const char *str = ReadString( section, key );
	if( *str == '\0' ) {
		VectorCopy( def, out );
		return 0;
	}

	// Return number of elements read, so caller can handle error in this case
	return sscanf( str, "%f %f %f", &out[ 0 ], &out[ 1 ], &out[ 2 ] );
}

bool huang::util::reg::WriteString( const char *section, const char *key, const char *value ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().writeStringEntry( section, key, value );
}

bool huang::util::reg::WriteInt( const char *section, const char *key, int value ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().writeIntEntry( section, key, value );
}

bool huang::util::reg::WriteBool( const char *section, const char *key, bool value ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().writeBoolEntry( section, key, value );
}

bool huang::util::reg::WriteFloat( const char *section, const char *key, float value ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().writeRealEntry( section, key, value );
}

bool huang::util::reg::WriteColour( const char *section, const char *key, FXColor value ) {
	FXApp *app = g_mainWindow->getApp();
	return app->reg().writeColorEntry( section, key, value );
}

bool huang::util::reg::WriteColourF( const char *section, const char *key, const vec3_t value ) {
	char buffer[ 256 ];
	snprintf( buffer, sizeof( buffer ), "%f %f %f", value[ 0 ], value[ 1 ], value[ 2 ] );
	return WriteString( section, key, buffer );
}
