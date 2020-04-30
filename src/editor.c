/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/pl_llist.h>
#include <PL/pl_window.h>

#include "yin.h"
#include "editor.h"
#include "map.h"
#include "gfx.h"
#include "act.h"

/* Simple editor interface, for editing */

static unsigned int numEditorCameras = 4;
static unsigned int curEditorCamera = 0;
static GfxCamera *editorCameras[ 4 ];

char texturePackages[ PL_SYSTEM_MAX_PATH ][ 256 ];
static void Editor_MountTexturePackageCallback( const char *path, void *userData ) {
	u_unused( userData );
	plMountLocation( path );
}

void Editor_Initialize( void ) {
	/* initialize core services */
	Gfx_Initialize();
	Act_Initialize();

	PrintMsg( "Initializing Editor...\n" );

	PrintMsg( "Mounting textures\n" );
	plScanDirectory( "Textures/", "pkg", Editor_MountTexturePackageCallback, false, NULL );

	const char *mapPath = plGetCommandLineArgumentValue( "-map" );
	if ( mapPath == NULL ) {
		PrintError( "No map specified on command line!\n" );
	}

	Map_Load( mapPath );

	/* setup each of the editor cameras */
	SysWindow *viewport = Sys_GetMainWindow(); /* for now all cameras just attach to the main viewport */
	editorCameras[ 0 ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_EYE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), viewport );
	editorCameras[ 1 ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_TOP, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), viewport );
	editorCameras[ 2 ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_SIDE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), viewport );
	editorCameras[ 3 ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_FRONT, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), viewport );
}

void Editor_Tick( void ) {
	if ( Sys_GetInputState( YIN_INPUT_LEFT_STICK ) ) { /* swap between the cameras */
		curEditorCamera++;
		if ( curEditorCamera >= 4 ) {
			curEditorCamera = 0;
		}
	}
}

void Editor_Display( void ) {
	GfxCamera *curCamera = editorCameras[ curEditorCamera ];
	if ( curCamera == NULL ) {
		PrintError( "Invalid camera!\n" );
	}

	Gfx_DrawPerspective( curCamera );

	Sys_SwapWindow( curCamera->viewportPtr );
}

void Editor_Shutdown( void ) {
	Act_Shutdown();
	Gfx_Shutdown();
}

void Editor_SetupInterface( EngineInterface *interface ) {
	interface->Tick 		= Editor_Tick;
	interface->Display 		= Editor_Display;
	interface->Initialize 	= Editor_Initialize;
	interface->Shutdown 	= Editor_Shutdown;
}
