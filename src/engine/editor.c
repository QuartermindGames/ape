/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/pl_llist.h>

#include "yin.h"
#include "editor.h"
#include "map.h"
#include "renderer/renderer.h"
#include "actor.h"

/* Simple editor interface, for editing */

static bool editorIsInitialized = false;

static unsigned int editorGridSize = 10;

static unsigned int numEditorCameras = 4;
static unsigned int curEditorCamera = 0;

static GfxCamera *editorCameras[ MAX_VIEW_PERSPECTIVES ];
static SysWindow *editorViewports[ MAX_VIEW_PERSPECTIVES ];

char texturePackages[ PL_SYSTEM_MAX_PATH ][ 256 ];
static void Editor_MountTexturePackageCallback( const char *path, void *userData ) {
	u_unused( userData );
	plMountLocation( path );
}

void Editor_Initialize( void ) {
	PrintMsg( "Initializing Editor...\n" );

	PrintMsg( "Mounting textures\n" );
	plScanDirectory( "Textures/", "pkg", Editor_MountTexturePackageCallback, false, NULL );

	Map_ClearData();

	const char *mapPath = plGetCommandLineArgumentValue( "-map" );
	if ( mapPath != NULL ) {
		Map_Load( mapPath );
	}

	/* setup each of the editor cameras */
	editorViewports[ VIEW_PERSPECTIVE_EYE ] = Engine_GetMainWindow();
	editorCameras[ VIEW_PERSPECTIVE_EYE ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_EYE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), editorViewports[ VIEW_PERSPECTIVE_EYE ] );

	editorViewports[ VIEW_PERSPECTIVE_TOP ] = g_system.CreateWindow( "Top", 640, 480 );
	editorCameras[ VIEW_PERSPECTIVE_TOP ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_TOP, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), editorViewports[ VIEW_PERSPECTIVE_TOP ] );

	editorViewports[ VIEW_PERSPECTIVE_SIDE ] = g_system.CreateWindow( "Side", 640, 480 );
	editorCameras[ VIEW_PERSPECTIVE_SIDE ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_SIDE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), editorViewports[ VIEW_PERSPECTIVE_SIDE ] );

	editorViewports[ VIEW_PERSPECTIVE_FRONT ] = g_system.CreateWindow( "Front", 640, 480 );
	editorCameras[ VIEW_PERSPECTIVE_FRONT ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_FRONT, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), editorViewports[ VIEW_PERSPECTIVE_FRONT ] );

	editorIsInitialized = true;
}

static void Editor_Input( void ) {
	static const unsigned int maxDelay = 10;
	static unsigned int inputDelay = 0;

	/* handle camera input */
	GfxCamera *curCamera = editorCameras[ curEditorCamera ];
	if ( curCamera != NULL ) {
		if ( g_system.GetButtonState( INPUT_UP ) ) {
			if ( curCamera->perspective == VIEW_PERSPECTIVE_EYE ) {
				curCamera->cameraPtr->position.x += 1.0f;
			} else {
				curCamera->cameraPtr->position.y += 1.0f;
			}
		} else if ( g_system.GetButtonState( INPUT_DOWN ) ) {

		} else if ( g_system.GetButtonState( INPUT_LEFT ) ) {

		} else if ( g_system.GetButtonState( INPUT_RIGHT ) ) {

		}
	}

	if ( g_system.GetKeyState( '1' ) ) {
		editorGridSize = 1;
	} else if ( g_system.GetKeyState( '2' ) ) {
		editorGridSize = 2;
	} else if ( g_system.GetKeyState( '3' ) ) {
		editorGridSize = 3;
	} else if ( g_system.GetKeyState( '4' ) ) {
		editorGridSize = 4;
	} else if ( g_system.GetKeyState( '5' ) ) {
		editorGridSize = 5;
	} else if ( g_system.GetKeyState( '6' ) ) {
		editorGridSize = 6;
	} else if ( g_system.GetKeyState( '7' ) ) {
		editorGridSize = 7;
	} else if ( g_system.GetKeyState( '8' ) ) {
		editorGridSize = 8;
	} else if ( g_system.GetKeyState( '9' ) ) {
		editorGridSize = 9;
	} else if ( g_system.GetKeyState( '0' ) ) {
		editorGridSize = 10;
	}

	if ( g_system.GetButtonState( INPUT_LEFT_STICK ) ) { /* swap between the cameras */
		if ( inputDelay >= Engine_GetNumTicks() ) {
			return;
		}

		curEditorCamera++;
		if ( curEditorCamera >= 4 ) {
			curEditorCamera = 0;
		}

		PrintMsg( "Selected camera %d (%s)\n", curEditorCamera, Gfx_GetPerspectiveDescription( editorCameras[ curEditorCamera ]->perspective ) );

		inputDelay = Engine_GetNumTicks() + maxDelay;
	}
}

void Editor_Tick( void ) {
	if ( !editorIsInitialized ) {
		return;
	}

	Editor_Input();
}

void Editor_Display( void ) {
	if ( !editorIsInitialized ) {
		return;
	}

	for ( unsigned int i = 0; i < MAX_VIEW_PERSPECTIVES; ++i ) {
		GfxCamera *curCamera = editorCameras[ i ];
		if ( curCamera == NULL ) {
			PrintError( "Invalid camera!\n" );
		}

		Gfx_DrawPerspective( curCamera );

		plDrawGrid( plGetMatrix( PL_MODELVIEW_MATRIX ), -2048, -2048, 4096, 4096, editorGridSize );
	}
}

void Editor_Shutdown( void ) {
	Act_Shutdown();
	Gfx_Shutdown();
}
