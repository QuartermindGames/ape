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
#include "entityw.h"
#include "MainWindow.h"
#include "Viewport.h"

using namespace huang::util;

BOOL SaveRegistryInfo( const char *pszName, void *pvBuf, long lSize );

extern void WXY_Print( void );

/*
==============================================================================

  MENU

==============================================================================
*/

void OpenDialog( void );
void SaveAsDialog( void );
qboolean ConfirmModified( void );
void  Select_Ungroup( void );

void QE_ExpandBspString( char *bspaction, char *out, char *mapname ) {
	char	src[ 1024 ];
	char	rsh[ 1024 ];
	char	base[ 256 ];

	ExtractFileName( mapname, base );
	sprintf( src, "%s/maps/%s", ValueForKey( g_qeglobals.d_project_entity, "remotebasepath" ), base );
	strcpy( rsh, ValueForKey( g_qeglobals.d_project_entity, "rshcmd" ) );

	const char *in = ValueForKey( g_qeglobals.d_project_entity, bspaction );
	while( *in ) {
		if( in[ 0 ] == '!' ) {
			strcpy( out, rsh );
			out += strlen( rsh );
			in++;
			continue;
		}
		if( in[ 0 ] == '$' ) {
			strcpy( out, src );
			out += strlen( src );
			in++;
			continue;
		}
		if( in[ 0 ] == '@' ) {
			*out++ = '"';
			in++;
			continue;
		}
		*out++ = *in++;
	}
	*out = 0;
}

void RunBsp( char *command ) {
#if 0
	char	sys[ 1024 ];
	char	batpath[ 1024 ];
	char	outputpath[ 1024 ];
	char	temppath[ 512 ];
	char	name[ 1024 ];
	FILE *hFile;
	BOOL	ret;
	PROCESS_INFORMATION ProcessInformation;
	STARTUPINFO	startupinfo;

	SetInspectorMode( W_CONSOLE );

	if( bsp_process ) {
		Sys_Printf( "BSP is still going...\n" );
		return;
	}

	GetTempPath( 512, temppath );
	sprintf( outputpath, "%sjunk.txt", temppath );

	strcpy( name, currentmap );
	if( region_active ) {
		Map_SaveFile( name, false );
		StripExtension( name );
		strcat( name, ".reg" );
	}

	Map_SaveFile( name, region_active );


	QE_ExpandBspString( command, sys, name );

	Sys_ClearPrintf();
	Sys_Printf( "======================================\nRunning bsp command...\n" );
	Sys_Printf( "\n%s\n", sys );

	//
	// write qe3bsp.bat
	//
	sprintf( batpath, "%sqe3bsp.bat", temppath );
	hFile = fopen( batpath, "w" );
	if( !hFile )
		Error( "Can't write to %s", batpath );
	fprintf( hFile, sys );
	fclose( hFile );

	//
	// write qe3bsp2.bat
	//
	sprintf( batpath, "%sqe3bsp2.bat", temppath );
	hFile = fopen( batpath, "w" );
	if( !hFile )
		Error( "Can't write to %s", batpath );
	fprintf( hFile, "%sqe3bsp.bat > %s", temppath, outputpath );
	fclose( hFile );

	Pointfile_Delete();

	GetStartupInfo( &startupinfo );

	ret = CreateProcess(
		batpath,		// pointer to name of executable module
		NULL,			// pointer to command line string
		NULL,			// pointer to process security attributes
		NULL,			// pointer to thread security attributes
		FALSE,			// handle inheritance flag
		0 /*DETACHED_PROCESS*/,		// creation flags
		NULL,			// pointer to new environment block
		NULL,			// pointer to current directory name
		&startupinfo,	// pointer to STARTUPINFO
		&ProcessInformation 	// pointer to PROCESS_INFORMATION
	);

	if( !ret )
		Error( "CreateProcess failed" );

	bsp_process = ProcessInformation.hProcess;

	Sleep( 100 );	// give the new process a chance to open it's window

	BringWindowToTop( g_qeglobals.d_hwndMain );	// pop us back on top
	SetFocus( g_qeglobals.d_hwndCamera );
#endif
}

/*
=============
DoColor

=============
*/
qboolean DoColor( int iIndex ) {
	return false;
}

/* handle all WM_COMMAND messages here */
#if 0
LONG WINAPI CommandHandler(
	HWND    hWnd,
	WPARAM  wParam,
	LPARAM  lParam ) {
	switch( LOWORD( wParam ) ) {
		//
		// file menu
		//
	case ID_FILE_EXIT:
		/* exit application */
		if( !ConfirmModified() )
			return TRUE;

		PostMessage( hWnd, WM_CLOSE, 0, 0L );
		break;

#if 0
	case ID_FILE_OPEN:
		if( !ConfirmModified() )
			return TRUE;
		OpenDialog();
		break;

	case ID_FILE_NEW:
		if( !ConfirmModified() )
			return TRUE;
		Map_New();
		break;
	case ID_FILE_SAVE:
		if( !strcmp( currentmap, "unnamed.map" ) )
			SaveAsDialog();
		else
			Map_SaveFile( currentmap, false );	// ignore region
		break;
#endif

	case ID_FILE_SAVEAS:
		SaveAsDialog();
		break;

	case ID_FILE_LOADPROJECT:
		if( !ConfirmModified() )
			return TRUE;
		ProjectDialog();
		break;

	case ID_FILE_POINTFILE:
		if( g_qeglobals.d_pointfile_display_list )
			Pointfile_Clear();
		else
			Pointfile_Check();
		break;

		//
		// view menu
		//
	case ID_VIEW_ENTITY:
		SetInspectorMode( W_ENTITY );
		break;
	case ID_VIEW_CONSOLE:
		SetInspectorMode( W_CONSOLE );
		break;
	case ID_VIEW_TEXTURE:
		SetInspectorMode( W_TEXTURE );
		break;
#if 0
	case ID_VIEW_100:
		g_qeglobals.d_xy.scale = 1;
		Sys_UpdateWindows( W_XY | W_XY_OVERLAY );
		break;
	case ID_VIEW_Z100:
		z.scale = 1;
		Sys_UpdateWindows( W_Z | W_Z_OVERLAY );
		break;

	case ID_VIEW_CENTER:
		camera.angles[ ROLL ] = camera.angles[ PITCH ] = 0;
		camera.angles[ YAW ] = 22.5f *
			floorf( ( camera.angles[ YAW ] + 11.0f ) / 22.5f );
		Sys_UpdateWindows( W_CAMERA | W_XY_OVERLAY );
		break;

	case ID_VIEW_UPFLOOR:
		Cam_ChangeFloor( true );
		break;
	case ID_VIEW_DOWNFLOOR:
		Cam_ChangeFloor( false );
		break;
#endif

	case ID_VIEW_SHOWNAMES:
		g_qeglobals.d_savedinfo.show_names = !g_qeglobals.d_savedinfo.show_names;
		CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWNAMES, MF_BYCOMMAND | ( g_qeglobals.d_savedinfo.show_names ? MF_CHECKED : MF_UNCHECKED ) );
		Map_BuildBrushData();
		Sys_UpdateWindows( W_XY );
		break;

	case ID_VIEW_SHOWCOORDINATES:
		g_qeglobals.d_savedinfo.show_coordinates ^= 1;
		CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWCOORDINATES, MF_BYCOMMAND | ( g_qeglobals.d_savedinfo.show_coordinates ? MF_CHECKED : MF_UNCHECKED ) );
		Sys_UpdateWindows( W_XY );
		break;

	case ID_VIEW_SHOWBLOCKS:
		g_qeglobals.show_blocks ^= 1;
		CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWBLOCKS, MF_BYCOMMAND | ( g_qeglobals.show_blocks ? MF_CHECKED : MF_UNCHECKED ) );
		Sys_UpdateWindows( W_XY );
		break;

	case ID_VIEW_SHOWLIGHTS:
		if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_LIGHTS ) & EXCLUDE_LIGHTS )
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_UNCHECKED );
		else
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_CHECKED );
		Sys_UpdateWindows( W_XY | W_CAMERA );
		break;

	case ID_VIEW_SHOWPATH:
		if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_PATHS ) & EXCLUDE_PATHS )
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_UNCHECKED );
		else
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_CHECKED );
		Sys_UpdateWindows( W_XY | W_CAMERA );
		break;

	case ID_VIEW_SHOWENT:
		if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_ENT ) & EXCLUDE_ENT )
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWENT, MF_BYCOMMAND | MF_UNCHECKED );
		else
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWENT, MF_BYCOMMAND | MF_CHECKED );
		Sys_UpdateWindows( W_XY | W_CAMERA );
		break;

	case ID_VIEW_SHOWWATER:
		if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_WATER ) & EXCLUDE_WATER )
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_UNCHECKED );
		else
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_CHECKED );
		Sys_UpdateWindows( W_XY | W_CAMERA );
		break;

	case ID_VIEW_SHOWCLIP:
		if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_CLIP ) & EXCLUDE_CLIP )
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_UNCHECKED );
		else
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_CHECKED );
		Sys_UpdateWindows( W_XY | W_CAMERA );
		break;

	case ID_VIEW_SHOWDETAIL:
		if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_DETAIL ) & EXCLUDE_DETAIL ) {
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWDETAIL, MF_BYCOMMAND | MF_UNCHECKED );
			SetWindowText( g_qeglobals.d_hwndCamera, "Camera View (DETAIL EXCLUDED)" );
		} else {
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWDETAIL, MF_BYCOMMAND | MF_CHECKED );
			SetWindowText( g_qeglobals.d_hwndCamera, "Camera View" );
		}
		Sys_UpdateWindows( W_XY | W_CAMERA );
		break;

	case ID_VIEW_SHOWWORLD:
		if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_WORLD ) & EXCLUDE_WORLD )
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_UNCHECKED );
		else
			CheckMenuItem( GetMenu( g_qeglobals.d_hwndMain ), ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_CHECKED );
		Sys_UpdateWindows( W_XY | W_CAMERA );
		break;

		//
		// texture menu
		//
	case ID_VIEW_NEAREST:
	case ID_VIEW_NEARESTMIPMAP:
	case ID_VIEW_LINEAR:
	case ID_VIEW_BILINEAR:
	case ID_VIEW_BILINEARMIPMAP:
	case ID_VIEW_TRILINEAR:
	case ID_TEXTURES_WIREFRAME:
	case ID_TEXTURES_FLATSHADE:
		Texture_SetMode( LOWORD( wParam ) );
		break;

	case ID_TEXTURES_SHOWINUSE:
		Sys_BeginWait();
		Texture_ShowInuse();
		SetInspectorMode( W_TEXTURE );
		break;

	case ID_TEXTURES_INSPECTOR:
		DoSurface();
		break;

	case CMD_TEXTUREWAD:
		Sys_BeginWait();
		Texture_ShowDirectory( LOWORD( wParam ) );
		SetInspectorMode( W_TEXTURE );
		break;

		//
		// bsp menu
		//
	case CMD_BSPCOMMAND:
	{
		extern	char *bsp_commands[ 256 ];

		RunBsp( bsp_commands[ LOWORD( wParam - CMD_BSPCOMMAND ) ] );
	}
	break;

	//
	// misc menu
	//
	case ID_MISC_BENCHMARK:
		SendMessage( g_qeglobals.d_hwndCamera,
			WM_USER + 267, 0, 0 );
		break;

	case ID_TEXTUREBK:
		DoColor( COLOR_TEXTUREBACK );
		Sys_UpdateWindows( W_ALL );
		break;

	case ID_MISC_SELECTENTITYCOLOR:
	{
		extern int inspector_mode;

		if( ( inspector_mode == W_ENTITY ) && DoColor( COLOR_ENTITY ) == true ) {
			extern void AddProp( void );

			char buffer[ 100 ];

			sprintf( buffer, "%f %f %f", g_qeglobals.d_savedinfo.colors[ COLOR_ENTITY ][ 0 ],
				g_qeglobals.d_savedinfo.colors[ COLOR_ENTITY ][ 1 ],
				g_qeglobals.d_savedinfo.colors[ COLOR_ENTITY ][ 2 ] );

			SetWindowText( hwndEnt[ EntValueField ], buffer );
			SetWindowText( hwndEnt[ EntKeyField ], "_color" );
			AddProp();
		}
		Sys_UpdateWindows( W_ALL );
	}
	break;

	case ID_MISC_PRINTXY:
		WXY_Print();
		break;

	case ID_COLORS_XYBK:
		DoColor( COLOR_GRIDBACK );
		Sys_UpdateWindows( W_ALL );
		break;

	case ID_COLORS_MAJOR:
		DoColor( COLOR_GRIDMAJOR );
		Sys_UpdateWindows( W_ALL );
		break;

	case ID_COLORS_MINOR:
		DoColor( COLOR_GRIDMINOR );
		Sys_UpdateWindows( W_ALL );
		break;

	case ID_MISC_FINDBRUSH:
		DoFind();
		break;

	case ID_MISC_NEXTLEAKSPOT:
		Pointfile_Next();
		break;
	case ID_MISC_PREVIOUSLEAKSPOT:
		Pointfile_Prev();
		break;

		//
		// brush menu
		//
	case ID_BRUSH_3SIDED:
		Brush_MakeSided( 3 );
		break;
	case ID_BRUSH_4SIDED:
		Brush_MakeSided( 4 );
		break;
	case ID_BRUSH_5SIDED:
		Brush_MakeSided( 5 );
		break;
	case ID_BRUSH_6SIDED:
		Brush_MakeSided( 6 );
		break;
	case ID_BRUSH_7SIDED:
		Brush_MakeSided( 7 );
		break;
	case ID_BRUSH_8SIDED:
		Brush_MakeSided( 8 );
		break;
	case ID_BRUSH_9SIDED:
		Brush_MakeSided( 9 );
		break;
	case ID_BRUSH_ARBITRARYSIDED:
		DoSides();
		break;

		//
		// select menu
		//
	case ID_BRUSH_FLIPX:
		Select_FlipAxis( 0 );
		break;
	case ID_BRUSH_FLIPY:
		Select_FlipAxis( 1 );
		break;
	case ID_BRUSH_FLIPZ:
		Select_FlipAxis( 2 );
		break;
	case ID_BRUSH_ROTATEX:
		Select_RotateAxis( 0, 90 );
		break;
	case ID_BRUSH_ROTATEY:
		Select_RotateAxis( 1, 90 );
		break;
	case ID_BRUSH_ROTATEZ:
		Select_RotateAxis( 2, 90 );
		break;

	case ID_SELECTION_ARBITRARYROTATION:
		DoRotate();
		break;

	case ID_SELECTION_UNGROUPENTITY:
		Select_Ungroup();
		break;

	case ID_SELECTION_CONNECT:
		ConnectEntities();
		break;

	case ID_SELECTION_DRAGVERTECIES:
		if( g_qeglobals.d_select_mode == sel_vertex ) {
			g_qeglobals.d_select_mode = sel_brush;
		} else {
			SetupVertexSelection();
			//if( g_qeglobals.d_numpoints )
				g_qeglobals.d_select_mode = sel_vertex;
		}
		break;
	case ID_SELECTION_DRAGEDGES:
		if( g_qeglobals.d_select_mode == sel_edge ) {
			g_qeglobals.d_select_mode = sel_brush;
		} else {
			SetupVertexSelection();
			//if( g_qeglobals.d_numpoints )
				g_qeglobals.d_select_mode = sel_edge;
		}
		break;

	case ID_SELECTION_SELECTPARTIALTALL:
		Select_PartialTall();
		break;
	case ID_SELECTION_SELECTCOMPLETETALL:
		Select_CompleteTall();
		break;
	case ID_SELECTION_SELECTTOUCHING:
		Select_Touching();
		break;
	case ID_SELECTION_SELECTINSIDE:
		Select_Inside();
		break;
	case ID_SELECTION_CSGSUBTRACT:
		CSG_Subtract();
		break;
	case ID_SELECTION_MAKEHOLLOW:
		CSG_MakeHollow();
		break;

	case ID_SELECTION_CLONE:
		Select_Clone();
		break;
	case ID_SELECTION_DELETE:
		Select_Delete();
		break;
	case ID_SELECTION_DESELECT:
		Select_Deselect();
		break;

	case ID_SELECTION_MAKE_DETAIL:
		Select_MakeDetail();
		break;
	case ID_SELECTION_MAKE_STRUCTURAL:
		Select_MakeStructural();
		break;


		//
		// region menu
		//
	case ID_REGION_OFF:
		Map_RegionOff();
		break;
	case ID_REGION_SETXY:
		Map_RegionXY();
		break;
	case ID_REGION_SETTALLBRUSH:
		Map_RegionTallBrush();
		break;
	case ID_REGION_SETBRUSH:
		Map_RegionBrush();
		break;
	case ID_REGION_SETSELECTION:
		Map_RegionSelectedBrushes();
		break;

#if 0
	case IDMRU + 1:
	case IDMRU + 2:
	case IDMRU + 3:
	case IDMRU + 4:
	case IDMRU + 5:
	case IDMRU + 6:
	case IDMRU + 7:
	case IDMRU + 8:
	case IDMRU + 9:
		DoMru( hWnd, LOWORD( wParam ) );
		break;
#endif

		//
		// help menu
		//

	case ID_HELP_ABOUT:
		DoAbout();
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
#endif

#if 0
/*
============
WMAIN_WndProc
============
*/
LONG WINAPI WMAIN_WndProc(
	HWND    hWnd,
	UINT    uMsg,
	WPARAM  wParam,
	LPARAM  lParam ) {
	LONG    lRet = 1;
	RECT	rect;
	HDC		maindc;

	GetClientRect( hWnd, &rect );

	switch( uMsg ) {
	case WM_TIMER:
		QE_CountBrushesAndUpdateStatusBar();
		QE_CheckAutoSave();
		return 0;

	case WM_CREATE:
		maindc = GetDC( hWnd );
		//	    QEW_SetupPixelFormat(maindc, false);
		return 0;

	case WM_KEYDOWN:
		return QE_KeyDown( wParam );

	case WM_CLOSE:
		DestroyWindow( hWnd );
		return 0;

	case WM_COMMAND:
		return CommandHandler( hWnd, wParam, lParam );
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
#endif

static void M_SaveGlobalRegistryData() {
	reg::WriteInt( "Global", "exclude", g_qeglobals.d_savedinfo.exclude );
	reg::WriteBool( "Global", "showCoordinates", g_qeglobals.d_savedinfo.show_coordinates );
	reg::WriteBool( "Global", "showNames", g_qeglobals.d_savedinfo.show_names );
	reg::WriteColourF( "Global", STRINGIFY( COLOR_TEXTUREBACK ), g_qeglobals.d_savedinfo.colors[ COLOR_TEXTUREBACK ] );
	reg::WriteColourF( "Global", STRINGIFY( COLOR_GRIDBACK ), g_qeglobals.d_savedinfo.colors[ COLOR_GRIDBACK ] );
	reg::WriteColourF( "Global", STRINGIFY( COLOR_CAMERABACK ), g_qeglobals.d_savedinfo.colors[ COLOR_CAMERABACK ] );
	reg::WriteColourF( "Global", STRINGIFY( COLOR_GRIDMINOR ), g_qeglobals.d_savedinfo.colors[ COLOR_GRIDMINOR ] );
	reg::WriteColourF( "Global", STRINGIFY( COLOR_GRIDMAJOR ), g_qeglobals.d_savedinfo.colors[ COLOR_GRIDMAJOR ] );
	reg::WriteColourF( "Global", STRINGIFY( COLOR_CAMERA_WIREFRAME ), g_qeglobals.d_savedinfo.colors[ COLOR_CAMERA_WIREFRAME ] );
}

static void M_LoadGlobalRegistryData() {
	g_qeglobals.d_savedinfo.exclude = reg::ReadBool( "Global", "exclude" );
	g_qeglobals.d_savedinfo.show_coordinates = reg::ReadBool( "Global", "showCoordinates", true );
	g_qeglobals.d_savedinfo.show_names = reg::ReadBool( "Global", "showNames", true );

	// Colours
	vec3_t def;
	VectorSet( def, 0.25f );
	reg::ReadColourF( "Global", STRINGIFY( COLOR_TEXTUREBACK ), g_qeglobals.d_savedinfo.colors[ COLOR_TEXTUREBACK ], def );
	reg::ReadColourF( "Global", STRINGIFY( COLOR_GRIDBACK ), g_qeglobals.d_savedinfo.colors[ COLOR_GRIDBACK ], def );
	reg::ReadColourF( "Global", STRINGIFY( COLOR_CAMERABACK ), g_qeglobals.d_savedinfo.colors[ COLOR_CAMERABACK ], def );
	VectorSet( def, 0.35f );
	reg::ReadColourF( "Global", STRINGIFY( COLOR_GRIDMINOR ), g_qeglobals.d_savedinfo.colors[ COLOR_GRIDMINOR ], def );
	VectorSet( def, 0.45f );
	reg::ReadColourF( "Global", STRINGIFY( COLOR_GRIDMAJOR ), g_qeglobals.d_savedinfo.colors[ COLOR_GRIDMAJOR ], def );
	VectorSet( def, 1.0f );
	reg::ReadColourF( "Global", STRINGIFY( COLOR_CAMERA_WIREFRAME ), g_qeglobals.d_savedinfo.colors[ COLOR_CAMERA_WIREFRAME ], def );
}

/*
==============
Main_Create
==============
*/
#if 0
void Main_Create( HINSTANCE hInstance ) {
	WNDCLASS   wc;
	HMENU      hMenu;

	/* Register the camera class */
	memset( &wc, 0, sizeof( wc ) );

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WMAIN_WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = 0;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	wc.lpszMenuName = MAKEINTRESOURCE( IDR_MENU1 );
	wc.lpszClassName = "QUAKE_MAIN";

	if( !RegisterClass( &wc ) )
		Error( "WCam_Register: failed" );

	g_qeglobals.d_hwndMain = CreateWindow( "QUAKE_MAIN",
		EDITOR_TITLE,
		WS_OVERLAPPEDWINDOW |
		WS_CLIPSIBLINGS |
		WS_CLIPCHILDREN,
		0, 0, screen_width, screen_height + GetSystemMetrics( SM_CYSIZE ),	// size
		0,
		0,		// no menu
		hInstance,
		NULL );
	if( !g_qeglobals.d_hwndMain )
		Error( "Couldn't create main window" );

	/* create a timer so that we can count brushes */
	//SetTimer( g_qeglobals.d_hwndMain,
	//	QE_TIMER0,
	//	1000,
	//	NULL );

	//LoadWindowState( g_qeglobals.d_hwndMain, "mainwindow" );

	//g_qeglobals.d_hwndStatus = CreateMyStatusWindow(hInstance);

	//
	// load misc info from registry
	//

	M_LoadGlobalRegistryData();
	M_SaveGlobalRegistryData();

	if( ( hMenu = GetMenu( g_qeglobals.d_hwndMain ) ) != 0 ) {
		/*
		** by default all of these are checked because that's how they're defined in the menu editor
		*/
		if( !g_qeglobals.d_savedinfo.show_names )
			CheckMenuItem( hMenu, ID_VIEW_SHOWNAMES, MF_BYCOMMAND | MF_UNCHECKED );
		if( !g_qeglobals.d_savedinfo.show_coordinates )
			CheckMenuItem( hMenu, ID_VIEW_SHOWCOORDINATES, MF_BYCOMMAND | MF_UNCHECKED );

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_LIGHTS )
			CheckMenuItem( hMenu, ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_UNCHECKED );
		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_ENT )
			CheckMenuItem( hMenu, ID_VIEW_ENTITY, MF_BYCOMMAND | MF_UNCHECKED );
		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_PATHS )
			CheckMenuItem( hMenu, ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_UNCHECKED );
		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_WATER )
			CheckMenuItem( hMenu, ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_UNCHECKED );
		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_WORLD )
			CheckMenuItem( hMenu, ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_UNCHECKED );
		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_CLIP )
			CheckMenuItem( hMenu, ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_UNCHECKED );
	}

	ShowWindow( g_qeglobals.d_hwndMain, SW_SHOWDEFAULT );
}
#endif

/*
===============================================================

  STATUS WINDOW

===============================================================
*/

void Sys_UpdateStatusBar( void ) {
	extern int   g_numbrushes, g_numentities;

	char numbrushbuffer[ 100 ] = "";

	sprintf( numbrushbuffer, "Brushes: %d Entities: %d", g_numbrushes, g_numentities );

	Sys_Status( numbrushbuffer, 2 );
}

void Sys_Status( const char *psz, int part ) {
	//SendMessage(g_qeglobals.d_hwndStatus, SB_SETTEXT, part, (LPARAM)psz);
}

//==============================================================

const FXuint TIMER_INTERVAL = 1;

FXDEFMAP( huang::MainWindow ) MainWindowMap[] = {
	FXMAPFUNC( SEL_CONFIGURE, huang::MainWindow::ID_CANVAS, huang::MainWindow::OnConfigure ),
	FXMAPFUNC( SEL_PAINT, huang::MainWindow::ID_CANVAS, huang::MainWindow::OnExpose ),
	FXMAPFUNC( SEL_CHORE, huang::MainWindow::ID_TIMEOUT,  huang::MainWindow::OnTimeout ),

	FXMAPFUNC( SEL_COMMAND, huang::MainWindow::ID_NEW, huang::MainWindow::OnCmdNew ),
	FXMAPFUNC( SEL_COMMAND, huang::MainWindow::ID_OPEN, huang::MainWindow::OnCmdOpen ),
	FXMAPFUNC( SEL_COMMAND, huang::MainWindow::ID_ABOUT, huang::MainWindow::OnCmdAbout ),

	FXMAPFUNC( SEL_COMMAND, huang::MainWindow::ID_TOGGLE_EDIT, huang::MainWindow::OnToggleEdit ),

	FXMAPFUNC( SEL_KEYRELEASE, huang::MainWindow::ID_CANVAS, huang::MainWindow::OnInput ),
};

// Object implementation
FXIMPLEMENT( huang::MainWindow, FXMainWindow, MainWindowMap, ARRAYNUMBER( MainWindowMap ) )

// Make some windows
huang::MainWindow::MainWindow( FXApp *a ) :
	FXMainWindow( a, EDITOR_TITLE, nullptr, nullptr, DECOR_ALL, 0, 0, 1024, 768, 0, 0 ),

	myGridSizeTarget( g_qeglobals.d_gridsize ),
	myGridStateTarget( g_qeglobals.d_showgrid ),

	showNamesTarget( g_qeglobals.d_savedinfo.show_names ),
	showCoordinatesTarget( g_qeglobals.d_savedinfo.show_coordinates )
{
	// Menu bar
	menubar = new FXMenuBar( this, LAYOUT_SIDE_TOP | LAYOUT_FILL_X );

	{
		toolBar = new FXToolBar( this, FRAME_RAISED | FRAME_THICK | LAYOUT_FILL_X );
		new FXToolBarGrip( toolBar, toolBar, FXToolBar::ID_TOOLBARGRIP, TOOLBARGRIP_DOUBLE );

		// File options
		new FXButton( toolBar, "", huang::util::LoadImageIcon( getApp(), "icons/new.gif" ), this, MainWindow::ID_NEW );
		new FXButton( toolBar, "", huang::util::LoadImageIcon( getApp(), "icons/open.gif" ), this, MainWindow::ID_OPEN );
		new FXButton( toolBar, "", huang::util::LoadImageIcon( getApp(), "icons/save.gif" ) );
		new FXVerticalSeparator( toolBar );
		// Edit options
		new FXButton( toolBar, "", huang::util::LoadImageIcon( getApp(), "icons/cut.gif" ) );
		new FXButton( toolBar, "", huang::util::LoadImageIcon( getApp(), "icons/copy.gif" ) );
		new FXButton( toolBar, "", huang::util::LoadImageIcon( getApp(), "icons/paste.gif" ) );
		new FXVerticalSeparator( toolBar );
		// Edit modes
		{
			editModeButtons[ EDIT_MODE_BRUSH ] = new FXToggleButton( toolBar, "", "", huang::util::LoadImageIcon( getApp(), "icons/brush_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
			editModeButtons[ EDIT_MODE_VERTEX ] = new FXToggleButton( toolBar, "", "", huang::util::LoadImageIcon( getApp(), "icons/vertex_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
			editModeButtons[ EDIT_MODE_EDGE ] = new FXToggleButton( toolBar, "", "", huang::util::LoadImageIcon( getApp(), "icons/edge_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
			editModeButtons[ EDIT_MODE_FACE ] = new FXToggleButton( toolBar, "", "", huang::util::LoadImageIcon( getApp(), "icons/face_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
			editModeButtons[ currentEditMode ]->setState( true );
		}
		new FXVerticalSeparator( toolBar );
		// Grouping
		new FXButton( toolBar, "", huang::util::LoadImageIcon( getApp(), "icons/group.gif" ) );
		new FXButton( toolBar, "", huang::util::LoadImageIcon( getApp(), "icons/ungroup.gif" ) );
		// Grid controls
		new FXVerticalSeparator( toolBar );
		{
			new FXToggleButton( toolBar, "", "", huang::util::LoadImageIcon( getApp(), "icons/grid.gif" ), 0, &myGridStateTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
			new FXTextField( toolBar, 4, &myGridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );
		}
		// Play
		new FXVerticalSeparator( toolBar );
		new FXButton( toolBar, "", huang::util::LoadImageIcon( getApp(), "icons/play_controller.gif" ) );
	}

	// Status bar
	new FXStatusBar( this, LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X );

	// File menu
	util::MenuItem fileMenuCmds[] = {
		{ "&New\t\tCreate a new map.", util::MenuType::COMMAND, MainWindow::ID_NEW, this },
		{ "&Open\t\tOpen an existing map.", util::MenuType::COMMAND, MainWindow::ID_OPEN, this },
		{ "&Quit\tCtl-Q\tQuit the application.", util::MenuType::COMMAND, FXApp::ID_QUIT },
		{ nullptr }
	};
	filemenu = util::CreateMenus( getApp(), menubar, "&File", fileMenuCmds );

	util::MenuItem editMenuCmds[] = {
		{ "&Copy\tCtl-C\tCopy the current brush.", util::MenuType::COMMAND, MainWindow::ID_COPY, this },
		{ "&Paste\tCtl-V\tPaste the current item.", util::MenuType::COMMAND, MainWindow::ID_PASTE, this },
		{ nullptr }
	};
	editMenu = util::CreateMenus( getApp(), menubar, "&Edit", editMenuCmds );

	FX4Splitter *viewportSplitter = new FX4Splitter( this, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | FOURSPLITTER_TRACKING );
	util::MenuItem viewMenuCmds[] = {
		{ "All four\t\tAbout the about dialog.", util::MenuType::CHECKBOX, FX4Splitter::ID_EXPAND_ALL, viewportSplitter },
		{ "Top/left", util::MenuType::CHECKBOX, FX4Splitter::ID_EXPAND_TOPLEFT, viewportSplitter },
		{ "Top/right", util::MenuType::CHECKBOX, FX4Splitter::ID_EXPAND_TOPRIGHT, viewportSplitter },
		{ "Bottom/left", util::MenuType::CHECKBOX, FX4Splitter::ID_EXPAND_BOTTOMLEFT, viewportSplitter },
		{ "Bottom/right", util::MenuType::CHECKBOX, FX4Splitter::ID_EXPAND_BOTTOMRIGHT, viewportSplitter },
		{ "", util::MenuType::SEPERATOR },
		{ "Show Names", util::MenuType::CHECKBOX, FXDataTarget::ID_VALUE, &showNamesTarget },
		{ "Show Coordinates", util::MenuType::CHECKBOX, FXDataTarget::ID_VALUE, &showCoordinatesTarget },
		{ nullptr }
	};
	util::CreateMenus( getApp(), menubar, "&View", viewMenuCmds );

	util::MenuItem aboutMenuCmds[] = {
		{ "&About\t\tAbout the about dialog.", util::MenuType::COMMAND, MainWindow::ID_ABOUT, this },
		{ nullptr }
	};
	util::CreateMenus( getApp(), menubar, "&Help", aboutMenuCmds );

	ViewMode modes[] = {
		VIEW_MODE_PERSPECTIVE,
		VIEW_MODE_TOP,
		VIEW_MODE_LEFT,
		VIEW_MODE_FRONT,
	};

	glVisual = new FXGLVisual( getApp(), VISUAL_DOUBLEBUFFER /*| VISUAL_STEREO */ );
	for( unsigned int i = 0; i < MAX_VIEWPORTS; ++i ) {
		viewports[ i ] = new Viewport( viewportSplitter, glVisual, modes[ i ] );
	}

	new FXToolTip( getApp() );
}

// Clean up
huang::MainWindow::~MainWindow() {
	delete filemenu;
	delete editMenu;
	delete viewMenu;
}

long huang::MainWindow::OnExpose( FXObject *, FXSelector, void * ) {
	return 1;
}

long huang::MainWindow::OnTimeout( FXObject *, FXSelector, void * ) {
	return 1;
}

long huang::MainWindow::OnConfigure( FXObject *, FXSelector, void * ) {
	return 1;
}

long huang::MainWindow::OnCmdNew( FXObject *, FXSelector, void * ) {
	// TODO: check if we need to save existing doc...
	
	CreateWorld();
	
	return TRUE;
}

long huang::MainWindow::OnCmdOpen( FXObject *, FXSelector, void * ) {
	FXFileDialog openDialog( this, "Open File" );
	openDialog.setSelectMode( SELECTFILE_EXISTING );
	openDialog.setPatternList( 
		"All Files (*)\n"
		"Map Files (*.map)\n"
		"World Files (*.wld)" );
	openDialog.setCurrentPattern( 1 );
	openDialog.setDirectory( "./worlds/" );
	if( openDialog.execute() ) {
		FXString filePath = openDialog.getFilename();
		LoadWorld( filePath.text() );
	}

	return TRUE;
}

long huang::MainWindow::OnCmdAbout( FXObject *, FXSelector, void * ) {
	static FXIcon *icon = nullptr;
	if( icon == nullptr ) {
		icon = huang::util::LoadImageIcon( getApp(), "icons/icon64.gif" );
	}
	FXMessageBox aboutBox(
		this,
		"About " EDITOR_TITLE,
		EDITOR_TITLE " is a level editor created for the Yin Game Engine.\n"
		"This software uses the FOX C++ GUI Library (http://www.fox-toolkit.org)\n\n"
		"Copyright (C) 1997-2001 Id Software, Inc.\n"
		"Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>\n",
		icon,
		MBOX_OK | DECOR_TITLE | DECOR_BORDER
	);

	aboutBox.execute();

	return 1;
}

long huang::MainWindow::OnToggleEdit( FXObject *object, FXSelector, void * ) {
	FXToggleButton *button = dynamic_cast<FXToggleButton *>( object );
	if( button == nullptr ) {
		return FALSE;
	}

	// Don't allow us to uncheck the same button without selecting a different one
	if( editModeButtons[ currentEditMode ] == button ) {
		button->setState( true );
		return TRUE;
	}

	// Now figure out what mode we selected
	for( uint8_t i = 0; i < MAX_EDIT_MODES; ++i ) {
		if( editModeButtons[ i ] == button ) {
			currentEditMode = i;
			continue;
		}

		editModeButtons[ i ]->setState( false );
	}

	switch( currentEditMode ) {
	default:
	case EDIT_MODE_BRUSH:
		g_qeglobals.d_select_mode = sel_brush;
		break;
	case EDIT_MODE_EDGE:
		g_qeglobals.d_select_mode = sel_edge;
		break;
	case EDIT_MODE_VERTEX:
		g_qeglobals.d_select_mode = sel_vertex;
		break;
	}

	return TRUE;
}

long huang::MainWindow::OnInput( FXObject *, FXSelector, void *ptr ) {
	FXEvent *ev = (FXEvent *)ptr;
	switch( ev->type ) {
	default: break;
	case SEL_KEYPRESS:
	{
		switch( ev->code ) {
		default: break;
		case '0':
			g_qeglobals.d_showgrid = !g_qeglobals.d_showgrid;
			return TRUE;
		case KEY_1:
			g_qeglobals.d_gridsize = 1;
			return TRUE;
		case KEY_2:
			g_qeglobals.d_gridsize = 2;
			return TRUE;
		case KEY_3:
			g_qeglobals.d_gridsize = 4;
			return TRUE;
		case KEY_4:
			g_qeglobals.d_gridsize = 8;
			return TRUE;
		case KEY_5:
			g_qeglobals.d_gridsize = 16;
			return TRUE;
		case KEY_6:
			g_qeglobals.d_gridsize = 32;
			return TRUE;
		case KEY_7:
			g_qeglobals.d_gridsize = 64;
			return TRUE;
		case KEY_8:
			g_qeglobals.d_gridsize = 128;
			return TRUE;
		case KEY_9:
			g_qeglobals.d_gridsize = 256;
			return TRUE;
		}
	}
	}

	return FALSE;
}

void huang::MainWindow::ResetViews() {
	for( unsigned int i = 0; i < MAX_VIEWPORTS; ++i ) {
		if( viewports[ i ] == nullptr ) {
			continue;
		}

		viewports[ i ]->ResetViews();
	}
}

void huang::MainWindow::CreateWorld() {
	Map_New();

	ResetViews();

	setTitle( "Untitled - " EDITOR_TITLE );
}

void huang::MainWindow::LoadWorld( const char *path ) {
	Map_LoadFile( path );

	ResetViews();

	char buf[ 512 ];
	snprintf( buf, sizeof( buf ), "%s - " EDITOR_TITLE, path );
	setTitle( buf );
}

void huang::MainWindow::CentreViewsOnBrush( const Brush *b ) {
	for( unsigned int i = 0; i < MAX_VIEWPORTS; ++i ) {
		viewports[ i ]->CentreViewOnBrush( b );
	}
}

// Start
void huang::MainWindow::create() {
	FXMainWindow::create();
	show( PLACEMENT_SCREEN );
}

huang::MainWindow *g_mainWindow = nullptr;
