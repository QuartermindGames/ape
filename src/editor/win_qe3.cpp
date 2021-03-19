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

int screen_width;
int screen_height;
qboolean have_quit;

int update_bits;

//===========================================

void Sys_SetTitle( char *text ) {
	g_mainWindow->setTitle( text );
}

void Sys_BeginWait( void ) {
#if 0
	waitcursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
#endif
}

void Sys_EndWait( void ) {
#if 0
	if ( waitcursor ) {
		SetCursor( waitcursor );
		waitcursor = NULL;
	}
#endif
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

void Sys_Beep() {
#if defined( _WIN32 )
	MessageBeep( MB_ICONASTERISK );
#endif
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

void Sys_ClearPrintf( void ) {
	char text[ 4 ];

	text[ 0 ] = 0;

	//SendMessage (g_qeglobals.d_hwndEdit,
	//	WM_SETTEXT,
	//	0,
	//	(LPARAM)text);
}

void Sys_Printf( const char *text, ... ) {
	va_list argptr;
	char buf[ 32768 ];
	char *out;

	va_start( argptr, text );
	vsprintf( buf, text, argptr );
	va_end( argptr );

	out = TranslateString( buf );

#ifdef LATER
	Sys_Status( out );
#else
	//SendMessage (g_qeglobals.d_hwndEdit,
	//	EM_REPLACESEL,
	//	0,
	//	(LPARAM)out);
#endif
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

#if 0 // todo
	int err = GetLastError();
	char text2[ 1024 ];
	sprintf( text2, "%s\nGetLastError() = %i", text, err );
	MessageBox( g_qeglobals.d_hwndMain, text2, "Error", 0 /* MB_OK */ );
#else
	printf( "Error: %s", text );
#endif

	exit( EXIT_FAILURE );
}

/*
======================================================================

FILE DIALOGS

======================================================================
*/

qboolean ConfirmModified( void ) {
#if 0
	if ( !modified )
		return true;

	if ( MessageBox( g_qeglobals.d_hwndMain, "This will lose changes to the map", "warning", MB_OKCANCEL ) == IDCANCEL )
		return false;
#endif
	return true;
}

void OpenDialog( void ) {
#if 0
	/*
	 * Obtain the system directory name and
	 * store it in szDirName.
	 */

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
		OFN_FILEMUSTEXIST;

	/* Display the Open dialog box. */

	if (!GetOpenFileName(&ofn))
		return;	// canceled

	// Add the file in MRU.
	AddNewItem( g_qeglobals.d_lpMruMenu, ofn.lpstrFile);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu,GetSubMenu(GetMenu(g_qeglobals.d_hwndMain),0),
			ID_FILE_EXIT);

	/* Open the file. */

	Map_LoadFile (ofn.lpstrFile);
#endif
}

void ProjectDialog( void ) {
#if 0
	/*
	 * Obtain the system directory name and
	 * store it in szDirName.
	 */

	strcpy (szDirName, ValueForKey(g_qeglobals.d_project_entity, "basepath") );
	strcat (szDirName, "\\scripts");

	/* Place the terminating null character in the szFile. */

	szFile[0] = '\0';

	/* Set the members of the OPENFILENAME structure. */

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_qeglobals.d_hwndCamera;
	ofn.lpstrFilter = szProjectFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;
	ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST |
		OFN_FILEMUSTEXIST;

	/* Display the Open dialog box. */

	if (!GetOpenFileName(&ofn))
		return;	// canceled

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu,GetSubMenu(GetMenu(g_qeglobals.d_hwndMain),0),
			ID_FILE_EXIT);

	/* Open the file. */
	if (!QE_LoadProject(ofn.lpstrFile))
		Error ("Couldn't load project file");
#endif
}


void SaveAsDialog( void ) {
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

/*
=======================================================

Menu modifications

=======================================================
*/

/*
==================
FillBSPMenu

==================
*/
char *bsp_commands[ 256 ];

void FillBSPMenu( void ) {
#if 0
	HMENU hmenu;
	epair_t *ep;
	int i;
	static int count;

	hmenu = GetSubMenu( GetMenu( g_qeglobals.d_hwndMain ), MENU_BSP );

	for ( i = 0; i < count; i++ )
		DeleteMenu( hmenu, CMD_BSPCOMMAND + i, MF_BYCOMMAND );
	count = 0;

	i = 0;
	for ( ep = g_qeglobals.d_project_entity->epairs; ep; ep = ep->next ) {
		if ( ep->key[ 0 ] == 'b' && ep->key[ 1 ] == 's' && ep->key[ 2 ] == 'p' ) {
			bsp_commands[ i ] = ep->key;
			AppendMenu( hmenu, MF_ENABLED | MF_STRING,
			            CMD_BSPCOMMAND + i, ( LPCTSTR ) ep->key );
			i++;
		}
	}
	count = i;
#endif
}

//==============================================

/*
===============
CheckBspProcess

See if the BSP is done yet
===============
*/
void CheckBspProcess( void ) {
#if 0
	char outputpath[ 1024 ];
	char temppath[ 512 ];
	DWORD exitcode;
	char *out;
	BOOL ret;

	if ( !bsp_process )
		return;

	ret = GetExitCodeProcess( bsp_process, &exitcode );
	if ( !ret )
		Error( "GetExitCodeProcess failed" );
	if ( exitcode == STILL_ACTIVE )
		return;

	bsp_process = 0;

	GetTempPath( 512, temppath );
	sprintf( outputpath, "%sjunk.txt", temppath );

	LoadFile( outputpath, ( void ** ) &out );
	Sys_Printf( "%s", out );
	Sys_Printf( "\ncompleted.\n" );
	free( out );
	Sys_Beep();

	Pointfile_Check();
#endif
}

extern int cambuttonstate;

/*
==================
WinMain

==================
*/
#if defined( _WIN32 )
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {
#if 0
	HACCEL accelerators;

	InitCommonControls();

	accelerators = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDR_ACCELERATOR1 ) );
	if ( !accelerators )
		Error( "LoadAccelerators failed" );
#endif

	screen_width = GetSystemMetrics( SM_CXFULLSCREEN );
	screen_height = GetSystemMetrics( SM_CYFULLSCREEN );

	// hack for broken NT 4.0 dual screen
	if ( screen_width > 2 * screen_height )
		screen_width /= 2;

	// START NEW NEW NEW
	FXApp app( EDITOR_TITLE );
	app.init( __argc, __argv );

	FXIcon *icon = huang::util::LoadImageIcon( &app, "icons/app_icon.gif" );
	g_mainWindow = new huang::MainWindow( &app );
	g_mainWindow->setIcon( icon );
	// END NEW NEW NEW

	// the project file can be specified on the command line,
	// or implicitly found in the scripts directory
	if ( lpCmdLine && strlen( lpCmdLine ) ) {
		ParseCommandLine( lpCmdLine );
		if ( !QE_LoadProject( argv[ 1 ] ) )
			Error( "Couldn't load %s project file", argv[ 1 ] );
	} else if ( !QE_LoadProject( EDITOR_CONFIG ) )
		Error( "Couldn't load \"" EDITOR_CONFIG "\" project file" );

	QE_Init();

	Sys_Printf( "Entering message loop\n" );

	app.create();

	extern void M_LoadGlobalRegistryData();
	M_LoadGlobalRegistryData();
	extern void M_SaveGlobalRegistryData();
	M_SaveGlobalRegistryData();

	return app.run();

#if 0
	while (!have_quit)
	{
		Sys_EndWait ();		// remove wait cursor if active

		while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(g_qeglobals.d_hwndMain, accelerators, &msg) )
			{
      			TranslateMessage (&msg);
      			DispatchMessage (&msg);
			}
			if (msg.message == WM_QUIT)
				have_quit = true;
		}

		CheckBspProcess ();

		time = Sys_DoubleTime ();
		delta = time - oldtime;
		oldtime = time;
		if (delta > 0.2)
			delta = 0.2;

		// run time dependant behavior
		Cam_MouseControl (delta);

		// update any windows now
		if (update_bits & W_CAMERA)
		{
			InvalidateRect(g_qeglobals.d_hwndCamera, NULL, false);
			UpdateWindow (g_qeglobals.d_hwndCamera);
		}
		if (update_bits & (W_Z | W_Z_OVERLAY) )
		{
			InvalidateRect(g_qeglobals.d_hwndZ, NULL, false);
			UpdateWindow (g_qeglobals.d_hwndZ);
		}

		if ( update_bits & W_TEXTURE )
		{
			InvalidateRect(g_qeglobals.d_hwndTexture, NULL, false);
			UpdateWindow (g_qeglobals.d_hwndEntity);
		}

		if (update_bits & (W_XY | W_XY_OVERLAY))
		{
			InvalidateRect(g_qeglobals.d_hwndXY, NULL, false);
			UpdateWindow (g_qeglobals.d_hwndXY);
		}

		update_bits = 0;

		if (!cambuttonstate && !have_quit)
		{	// if not driving in the camera view, block
			WaitMessage ();
		}

	}

    /* return success of application */
    return TRUE;
#endif
}
#else
int main( int argc, char **argv ) {
    FXApp app( EDITOR_TITLE );
    app.init( argc, argv );

    FXIcon *icon = huang::util::LoadImageIcon( &app, "icons/app_icon.gif" );
    g_mainWindow = new huang::MainWindow( &app );
    g_mainWindow->setIcon( icon );

    // the project file can be specified on the command line,
    // or implicitly found in the scripts directory
	const char *config = EDITOR_CONFIG;
	if ( argc > 1 ) {
		config = argv[ 1 ];
	}

    QE_Init();

    Sys_Printf( "Entering message loop\n" );

    app.create();

    // Ensure the main window is maximised after creation
    auto *mainWindow = dynamic_cast< huang::MainWindow * >( app.getActiveWindow() );
    if ( mainWindow == nullptr ) {
    //    Error( "Failed to fetch main window!\n" );
    }

   // mainWindow->maximize();

    return app.run();
}
#endif
