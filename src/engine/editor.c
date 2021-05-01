/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

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
static OSWindow *editorViewports[ MAX_VIEW_PERSPECTIVES ];

char texturePackages[ PL_SYSTEM_MAX_PATH ][ 256 ];
static void Editor_MountTexturePackageCallback( const char *path, void *userData ) {
	u_unused( userData );
	PlMountLocation( path );
}

void Editor_Initialize( void ) {
	Print( "Initializing Editor...\n" );

	Print( "Mounting textures\n" );
	PlScanDirectory( "Textures/", "pkg", Editor_MountTexturePackageCallback, false, NULL );

	Map_ClearData();

	const char *mapPath = PlGetCommandLineArgumentValue( "-map" );
	if ( mapPath != NULL ) {
		Map_Load( mapPath );
	}

	/* setup each of the editor cameras */
	editorViewports[ VIEW_PERSPECTIVE_EYE ] = Engine_GetMainWindow();
	editorCameras[ VIEW_PERSPECTIVE_EYE ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_EYE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), editorViewports[ VIEW_PERSPECTIVE_EYE ] );

	editorViewports[ VIEW_PERSPECTIVE_TOP ] = globalSystem.CreateWindow( "Top", 640, 480 );
	editorCameras[ VIEW_PERSPECTIVE_TOP ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_TOP, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), editorViewports[ VIEW_PERSPECTIVE_TOP ] );

	editorViewports[ VIEW_PERSPECTIVE_SIDE ] = globalSystem.CreateWindow( "Side", 640, 480 );
	editorCameras[ VIEW_PERSPECTIVE_SIDE ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_SIDE, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), editorViewports[ VIEW_PERSPECTIVE_SIDE ] );

	editorViewports[ VIEW_PERSPECTIVE_FRONT ] = globalSystem.CreateWindow( "Front", 640, 480 );
	editorCameras[ VIEW_PERSPECTIVE_FRONT ] = Gfx_CreateCamera( VIEW_PERSPECTIVE_FRONT, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 0 ), editorViewports[ VIEW_PERSPECTIVE_FRONT ] );

	editorIsInitialized = true;
}

static void Editor_Input( void ) {
	static const unsigned int maxDelay = 10;
	static unsigned int inputDelay = 0;

	/* handle camera input */
	GfxCamera *curCamera = editorCameras[ curEditorCamera ];
	if ( curCamera != NULL ) {
		if ( globalSystem.GetButtonState( INPUT_UP ) ) {
			if ( curCamera->perspective == VIEW_PERSPECTIVE_EYE ) {
				curCamera->internalPtr->position.x += 1.0f;
			} else {
				curCamera->internalPtr->position.y += 1.0f;
			}
		} else if ( globalSystem.GetButtonState( INPUT_DOWN ) ) {

		} else if ( globalSystem.GetButtonState( INPUT_LEFT ) ) {

		} else if ( globalSystem.GetButtonState( INPUT_RIGHT ) ) {

		}
	}

	if ( globalSystem.GetKeyState( '1' ) ) {
		editorGridSize = 1;
	} else if ( globalSystem.GetKeyState( '2' ) ) {
		editorGridSize = 2;
	} else if ( globalSystem.GetKeyState( '3' ) ) {
		editorGridSize = 3;
	} else if ( globalSystem.GetKeyState( '4' ) ) {
		editorGridSize = 4;
	} else if ( globalSystem.GetKeyState( '5' ) ) {
		editorGridSize = 5;
	} else if ( globalSystem.GetKeyState( '6' ) ) {
		editorGridSize = 6;
	} else if ( globalSystem.GetKeyState( '7' ) ) {
		editorGridSize = 7;
	} else if ( globalSystem.GetKeyState( '8' ) ) {
		editorGridSize = 8;
	} else if ( globalSystem.GetKeyState( '9' ) ) {
		editorGridSize = 9;
	} else if ( globalSystem.GetKeyState( '0' ) ) {
		editorGridSize = 10;
	}

	if ( globalSystem.GetButtonState( INPUT_LEFT_STICK ) ) { /* swap between the cameras */
		if ( inputDelay >= Engine_GetNumTicks() ) {
			return;
		}

		curEditorCamera++;
		if ( curEditorCamera >= 4 ) {
			curEditorCamera = 0;
		}

		Print( "Selected camera %d (%s)\n", curEditorCamera, Gfx_GetPerspectiveDescription( editorCameras[ curEditorCamera ]->perspective ) );

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

		PlgDrawGrid( *PlGetMatrix( PL_MODELVIEW_MATRIX ), -2048, -2048, 4096, 4096, editorGridSize );
	}
}

void Editor_Shutdown( void ) {
	Act_Shutdown();
	R_Shutdown();
}
