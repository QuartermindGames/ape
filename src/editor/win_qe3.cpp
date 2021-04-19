/*
===========================================================================
Copyright (C) 1997-2006 Id Software, Inc.

This file is part of Quake 2 Tools source code.

Quake 2 Tools source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake 2 Tools source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake 2 Tools source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "qe3.h"
#include "mru.h"

qboolean have_quit;

int update_bits;

//===========================================

void Sys_SetTitle( char *text ) {
	g_mainWindow->setTitle( text );
}

void Sys_BeginWait( void ) {
	g_mainWindow->getApp()->beginWaitCursor();
}

void Sys_EndWait( void ) {
	g_mainWindow->getApp()->endWaitCursor();
}

void Sys_GetCursorPos( int *x, int *y ) {
	unsigned int temp;
	g_mainWindow->getCursorPosition( *x, *y, temp );
}

void Sys_SetCursorPos( int x, int y ) {
	g_mainWindow->setCursorPosition( x, y );
}

void Sys_UpdateWindows( int bits ) {
	//	Sys_Printf("updating 0x%X\n", bits);
	update_bits |= bits;
	//update_bits = -1;
}

char *TranslateString( char *buf ) {
	static char buf2[ 32768 ];
	char *out;

	size_t l = strlen( buf );
	out = buf2;
	for ( size_t i = 0; i < l; i++ ) {
		if ( buf[ i ] == '\n' ) {
			*out++ = '\r';
			*out++ = '\n';
		} else
			*out++ = buf[ i ];
	}
	*out++ = 0;

	return buf2;
}

void Sys_Printf( const char *text, ... ) {
	va_list argptr;
	char buf[ 32768 ];
	char *out;

	va_start( argptr, text );
	vsprintf( buf, text, argptr );
	va_end( argptr );

	out = TranslateString( buf );

	LMsg( out );
}

double Sys_DoubleTime( void ) {
	return clock() / 1000.0;
}

/*
=================
Error

For abnormal program terminations
=================
*/
void Error( const char *error, ... ) {
	va_list argptr;
	char text[ 1024 ];

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr );

	LError( text );

	exit( EXIT_FAILURE );
}

/*
======================================================================

FILE DIALOGS

======================================================================
*/

qboolean ConfirmModified() {
	if ( !modified ) {
		return true;
	}

	if ( FXMessageBox::warning(
	             g_mainWindow,
	             MBOX_OK_CANCEL,
	             "Warning",
	             "This will lose changes to the world!" ) == MBOX_CLICKED_YES ) {
		return false;
	}

	return true;
}

void SaveAsDialog() {
#if 0
	strcpy (szDirName, ValueForKey (g_qeglobals.d_project_entity, "basepath") );
	strcat (szDirName, "\\maps");

	/* Place the terminating null character in the szFile. */

	szFile[0] = '\0';

	/* Set the members of the OPENFILENAME structure. */

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_qeglobals.d_hwndCamera;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;
	ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST |
		OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;

	/* Display the Open dialog box. */

	if (!GetSaveFileName(&ofn))
		return;	// canceled

	DefaultExtension (ofn.lpstrFile, ".map");
	strcpy (currentmap, ofn.lpstrFile);

	// Add the file in MRU.
	AddNewItem(g_qeglobals.d_lpMruMenu, ofn.lpstrFile);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu,GetSubMenu(GetMenu(g_qeglobals.d_hwndMain),0),
			ID_FILE_EXIT);

	Map_SaveFile (ofn.lpstrFile, false);	// ignore region
#endif
}

/****************************************
 * MEMORY MANAGEMENT
 ****************************************/

/* wrapper for calloc */
void *Sys_calloc( size_t num, size_t size, bool abortOnFail ) {
	void *mem = calloc( num, size );
	if ( mem == nullptr ) {
		if ( abortOnFail ) {
			LError( "Failed to allocate %d bytes!\n", num * size );
		}

		LWarn( "Failed to allocate %d bytes!\n", num * size );
	}

	return mem;
}

/* wrapper for malloc */
void *Sys_malloc( size_t size, bool abortOnFail ) {
	return Sys_calloc( 1, size, abortOnFail );
}

/* wrapper for realloc */
void *Sys_realloc( void *ptr, size_t newSize, bool abortOnFail ) {
	void *buf = realloc( ptr, newSize );
	if ( buf == nullptr ) {
		if ( abortOnFail ) {
			LError( "Failed to allocate %d bytes!\n", newSize );
		}

		LWarn( "Failed to allocate %d bytes!\n", newSize );
	}

	return buf;
}

/* wrappers for platform lib */
void *Sys_WMAlloc( size_t size ) { return Sys_malloc( size, true ); }
void *Sys_WCAlloc( size_t num, size_t size ) { return Sys_calloc( num, size, true ); }
void *Sys_WReAlloc( void *ptr, size_t newSize ) { return Sys_realloc( ptr, newSize, true ); }

/****************************************
 ****************************************/

EngineInterface globalEngine;
static PLLibrary *dllEnginePtr;
static void SetupEngineInterface() {
	LMsg( "Setting up engine interface\n" );

	dllEnginePtr = plLoadLibrary( "./engine", true );
	if ( dllEnginePtr == nullptr ) {
		LError( "Failed to load engine module, aborting!\nPL: %s\n", plGetError() );
	}

	auto GetDllInterface = ( DllEngineInterface ) plGetLibraryProcedure( dllEnginePtr, "GetDllInterface" );
	if ( GetDllInterface == nullptr ) {
		LError( "Failed to fetch \"" INTERFACE_PROCEDURE "\" from engine module, aborting!\nPL: %s\n", plGetError() );
	}

	static SystemInterface systemInterface = {
		.version = { ENGINE_INTERFACE_VERSION_MAJOR, ENGINE_INTERFACE_VERSION_MINOR },

#if 0
			.Shutdown = nullptr,
			.CreateWindow = nullptr,
			.GetWindowSize = nullptr,
			.GetButtonState = nullptr,
			.GetKeyState = nullptr,
			.HasKeyboard = nullptr,

			.GetPerformanceCounter = nullptr,
			.GetPerformanceFrequency = nullptr,
#endif
			.CAlloc = Sys_calloc,
			.MAlloc = Sys_malloc,
			.ReAlloc = Sys_realloc,
	};

	/* initialize the interface */
	globalEngine = *GetDllInterface( ENGINE_INTERFACE_VERSION, &systemInterface );
	if ( globalEngine.version[ VERSION_MAJOR ] != ENGINE_INTERFACE_VERSION_MAJOR ) {
		LWarn( "Unexpected major interface version (%d vs %d)!\n", globalEngine.version[ VERSION_MAJOR ], ENGINE_INTERFACE_VERSION_MAJOR );
	}
}

unsigned int LOG_LEVEL_INFO, LOG_LEVEL_WARNING, LOG_LEVEL_ERROR;

#if defined( _WIN32 )
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {
	int vargc = __argc;
	char **vargv = __argv;
#else
int main( int argc, char **argv ) {
	int vargc = argc;
	char **vargv = argv;
#endif

	pl_calloc = Sys_WCAlloc;
	pl_malloc = Sys_WMAlloc;
	pl_realloc = Sys_WReAlloc;

	plInitialize( vargc, vargv );

    plRegisterPlugins( "./" );

	// Setup logging
	if ( plHasCommandLineArgument( "-log" ) ) {
		const char *path = plGetCommandLineArgumentValue( "-log" );
		if ( path == nullptr ) {
			path = EDITOR_LOG_PATH;
		}

		plSetupLogOutput( path );
	}
	LOG_LEVEL_INFO = plAddLogLevel( "editor", PL_COLOUR_WHITE, true );
	LOG_LEVEL_WARNING = plAddLogLevel( "editor/warning", PL_COLOUR_ORANGE, true );
	LOG_LEVEL_ERROR = plAddLogLevel( "editor/error", PL_COLOUR_RED, true );

	LMsg( "Log initialized\n" );

	CommonLibrary_Initialize();

	SetupEngineInterface();

	FXApp app( EDITOR_TITLE );
	app.init( vargc, vargv );

	FXIcon *icon = huang::util::LoadImageIcon( &app, "icons/app_icon.gif" );
	g_mainWindow = new huang::MainWindow( &app );
	g_mainWindow->setIcon( icon );

    // the project file can be specified on the command line,
    // or implicitly found in the scripts directory
    char projectPath[ PL_SYSTEM_MAX_PATH ];
    const char *arg = plGetCommandLineArgumentValue( "-config" );
	if ( arg != nullptr ) {
		snprintf( projectPath, sizeof( projectPath ), "%s", arg );
	} else {
		snprintf( projectPath, sizeof( projectPath ), "%s" EDITOR_CONFIG, ComFS_GetDataDirectory() );
	}

    if ( !QE_LoadProject( projectPath ) ) {
        Error( "Couldn't load %s project file", projectPath );
    }

	QE_Init();

	Sys_Printf( "Entering message loop\n" );

	app.create();

    plInitializePlugins();

	extern void M_LoadGlobalRegistryData();
	M_LoadGlobalRegistryData();
	extern void M_SaveGlobalRegistryData();
	M_SaveGlobalRegistryData();

	return app.run();

#if 0
	while (!have_quit)
	{
		time = Sys_DoubleTime ();
		delta = time - oldtime;
		oldtime = time;
		if (delta > 0.2)
			delta = 0.2;

		// run time dependant behavior
		Cam_MouseControl (delta);
	}
#endif
}
