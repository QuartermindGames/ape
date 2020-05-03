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

static unsigned int editorGridSize = 10;

static unsigned int numEditorCameras = 4;
static unsigned int curEditorCamera = 0;
static GfxCamera *editorCameras[ 4 ];

char texturePackages[ PL_SYSTEM_MAX_PATH ][ 256 ];
static void Editor_MountTexturePackageCallback( const char *path, void *userData ) {
	u_unused( userData );
	plMountLocation( path );
}

void Editor_Initialize( void ) {
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

static void Editor_Input( void ) {
	static const unsigned int maxDelay = 10;
	static unsigned int inputDelay = 0;

	/* handle camera input */
	GfxCamera *curCamera = editorCameras[ curEditorCamera ];
	if ( curCamera != NULL ) {
		if ( Sys_GetButtonState( YIN_INPUT_UP ) ) {
			if ( curCamera->perspective == VIEW_PERSPECTIVE_EYE ) {
				curCamera->cameraPtr->position.x += 1.0f;
			} else {
				curCamera->cameraPtr->position.y += 1.0f;
			}
		} else if ( Sys_GetButtonState( YIN_INPUT_DOWN ) ) {

		} else if ( Sys_GetButtonState( YIN_INPUT_LEFT ) ) {

		} else if ( Sys_GetButtonState( YIN_INPUT_RIGHT ) ) {

		}
	}

	if ( Sys_GetKeyState( '1' ) ) {
		editorGridSize = 1;
	} else if ( Sys_GetKeyState( '2' ) ) {
		editorGridSize = 2;
	} else if ( Sys_GetKeyState( '3' ) ) {
		editorGridSize = 3;
	} else if ( Sys_GetKeyState( '4' ) ) {
		editorGridSize = 4;
	} else if ( Sys_GetKeyState( '5' ) ) {
		editorGridSize = 5;
	} else if ( Sys_GetKeyState( '6' ) ) {
		editorGridSize = 6;
	} else if ( Sys_GetKeyState( '7' ) ) {
		editorGridSize = 7;
	} else if ( Sys_GetKeyState( '8' ) ) {
		editorGridSize = 8;
	} else if ( Sys_GetKeyState( '9' ) ) {
		editorGridSize = 9;
	} else if ( Sys_GetKeyState( '0' ) ) {
		editorGridSize = 10;
	}

	if ( Sys_GetButtonState( YIN_INPUT_LEFT_STICK ) ) { /* swap between the cameras */
		if ( inputDelay >= Sys_GetNumTicks() ) {
			return;
		}

		curEditorCamera++;
		if ( curEditorCamera >= 4 ) {
			curEditorCamera = 0;
		}

		PrintMsg( "Selected camera %d (%s)\n", curEditorCamera, Gfx_GetPerspectiveDescription( editorCameras[ curEditorCamera ]->perspective ) );

		inputDelay = Sys_GetNumTicks() + maxDelay;
	}
}

void Editor_Tick( void ) {
	Editor_Input();
}

void Editor_Display( void ) {
	GfxCamera *curCamera = editorCameras[ curEditorCamera ];
	if ( curCamera == NULL ) {
		PrintError( "Invalid camera!\n" );
	}

	plClearBuffers( PL_BUFFER_DEPTH | PL_BUFFER_COLOUR );

	Gfx_DrawPerspective( curCamera );

	plDrawGrid( plGetMatrix( PL_MODELVIEW_MATRIX ), -2048, -2048, 4096, 4096, editorGridSize );

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
