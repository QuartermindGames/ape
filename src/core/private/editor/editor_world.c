// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: World editor specific functionality.

#include "core_private.h"

#include "editor.h"
#include "world/world.h"

#include "client/client_input.h"
#include "client/renderer/renderer.h"
#include "client/renderer/renderer_font.h"
#include "game_interface.h"

#define WORLD_CONTEXT_IDENTIFIER "world"

static YNCoreEditorContext context;
static YNCoreEditorGeometryMode geometryMode = EDITOR_GEOMETRYMODE_VERTEX;

#define MAX_CAMERA_SLOTS 16
static OgeCamera *cameras[ MAX_CAMERA_SLOTS ];

static OgeWorld *world = NULL;

static int mouseCursorX = 0,
           mouseCursorY = 0;

static void RegisterWorldEditorVariables( void )
{
}

static void CreateWorldCommand( unsigned int argc, char **argv )
{
	if ( !YnCore_IsEditorContextActive( WORLD_CONTEXT_IDENTIFIER ) )
	{
		return;
	}

	world = ogeCreateWorld();
}

static void DestroyWorldCommand( unsigned int argc, char **argv )
{
	if ( !YnCore_IsEditorContextActive( WORLD_CONTEXT_IDENTIFIER ) )
	{
		return;
	}

	ogeDestroyWorld( world );
	world = NULL;
}

static void CreateMeshCommand( unsigned int argc, char **argv )
{
	YNCoreEditorContext *editorInstance = YnCore_GetCurrentEditorContext();
	if ( editorInstance == NULL )
	{
		PRINT_WARNING( "Command failed - no active instance!\n" );
		return;
	}

	if ( editorInstance->mode != YN_CORE_EDITOR_CONTEXT_WORLD )
	{
		PRINT_WARNING( "Command failed - invalid active instance mode!\n" );
		return;
	}

	if ( world == NULL )
	{
		return;
	}

	PLVector3 pos = ( PLVector3 ){
	        strtof( argv[ 1 ], NULL ),
	        strtof( argv[ 2 ], NULL ),
	        strtof( argv[ 3 ], NULL ) };

	OgeWorldMesh *mesh = YnCore_WorldMesh_Create( world );
}

static void IncreaseGridSize( OgeInputState state )
{
	if ( !YnCore_IsEditorContextActive( WORLD_CONTEXT_IDENTIFIER ) )
	{
		return;
	}

	if ( state != OGE_INPUT_STATE_PRESSED )
	{
		return;
	}

	context.gridScale += 2;
}

static void DecreaseGridSize( OgeInputState state )
{
	if ( !YnCore_IsEditorContextActive( WORLD_CONTEXT_IDENTIFIER ) )
	{
		return;
	}

	if ( state != OGE_INPUT_STATE_PRESSED )
	{
		return;
	}

	context.gridScale -= 2;
	if ( context.gridScale <= 0 )
	{
		context.gridScale = 1;
	}
}

static void ToggleView( OgeInputState state )
{
	if ( !YnCore_IsEditorContextActive( WORLD_CONTEXT_IDENTIFIER ) )
	{
		return;
	}

	if ( state != OGE_INPUT_STATE_PRESSED )
	{
		return;
	}

	context.camera->mode++;
	if ( context.camera->mode >= OGE_CAMERA_MAX_MODES )
	{
		context.camera->mode = OGE_CAMERA_MODE_PERSPECTIVE;
	}
}

static void InitializeWorldEditor( void )
{
	for ( uint32_t i = 0; i < MAX_CAMERA_SLOTS; ++i )
	{
		char buf[ 64 ];
		snprintf( buf, sizeof( buf ), "worldCamera%u", i );
		cameras[ i ] = ogeCreateCamera( buf, &pl_vecOrigin3, &pl_vecOrigin3 );
	}

	context.camera           = cameras[ 0 ];
	context.camera->mode     = OGE_CAMERA_MODE_TOP;
	context.camera->drawMode = OGE_CAMERA_DRAW_MODE_WIREFRAME;

	ogeRegisterInputAction( "editor.world.gridUp", NULL, 0, ( OgeInputKey[] ){ '[' }, 1, IncreaseGridSize );
	ogeRegisterInputAction( "editor.world.gridDown", NULL, 0, ( OgeInputKey[] ){ ']' }, 1, DecreaseGridSize );
	ogeRegisterInputAction( "editor.world.toggleView", NULL, 0, ( OgeInputKey[] ){ KEY_TAB }, 1, ToggleView );
}

static void ShutdownWorldEditor( void )
{
}

static void DrawWorldEditor( void )
{
}

static void DrawWorldEditorGUI( void )
{
	int w, h;
	PlgGetViewport( NULL, NULL, &w, &h );

	if ( context.camera != NULL && ( context.camera->mode != OGE_CAMERA_MODE_PERSPECTIVE ) && ( context.gridScale > 0 ) )
	{
		PlgSetShaderProgram( oge_defaultShaderPrograms_[ OGE_SHADER_DEFAULT_VERTEX ] );

		static float z = 16.0f;
		float zoom     = roundf( z ) / 2.0f;

		float x = 500.0f + sinf( zoom * 2.0f ) * 100.0f;
		float y = 200.0f + cosf( zoom * 2.0f ) * 100.0f;

		PLMatrix4 transform = PlMatrix4Identity();
		transform           = PlScaleMatrix4( transform, ( PLVector3 ){ zoom, zoom, zoom } );

		// stupid matrix bollocks, blargh
		transform = PlTransposeMatrix4( &transform );
		PlgSetViewMatrix( &transform );

		int m = ( w > h ) ? w : h;
		PlgDrawDottedGrid( -m / 2, -m / 2, m, m, context.gridScale / 2, &( PLColour ){ 70, 70, 70, 255 } );
		PlgDrawDottedGrid( -m / 2, -m / 2, m, m, ( context.gridScale / 2 ) * 4, &( PLColour ){ 100, 100, 100, 255 } );

		switch ( context.camera->mode )
		{
			default:
				break;
			case OGE_CAMERA_MODE_TOP:
				transform = PlMultiplyMatrix4( transform, PlTranslateMatrix4( ( PLVector3 ){ x, -0.0f, -y } ) );
				transform = PlMultiplyMatrix4( transform, PlRotateMatrix4( PL_DEG2RAD( 90.0f ), &( PLVector3 ){ 1.0f, 0.0f, 0.0f } ) );
				break;
			case OGE_CAMERA_MODE_LEFT:
				transform = PlMultiplyMatrix4( transform, PlTranslateMatrix4( ( PLVector3 ){ 0.0f, -y, -x } ) );
				transform = PlMultiplyMatrix4( transform, PlRotateMatrix4( PL_DEG2RAD( 90.0f ), &( PLVector3 ){ 0.0f, 1.0f, 0.0f } ) );
				transform = PlMultiplyMatrix4( transform, PlRotateMatrix4( PL_DEG2RAD( 180.0f ), &( PLVector3 ){ 0.0f, 0.0f, 1.0f } ) );
				break;
			case OGE_CAMERA_MODE_FRONT:
				transform = PlMultiplyMatrix4( transform, PlTranslateMatrix4( ( PLVector3 ){ -x, -y, 0.0f } ) );
				transform = PlMultiplyMatrix4( transform, PlRotateMatrix4( PL_DEG2RAD( 180.0f ), &( PLVector3 ){ 0.0f, 0.0f, 1.0f } ) );
				break;
		}

		// stupid matrix bollocks, blargh
		transform = PlTransposeMatrix4( &transform );
		PlgSetViewMatrix( &transform );

		OgeCamera tmp;
		PL_ZERO_( tmp );
		tmp.internal = ogeGetAuxCamera();
		switch ( context.camera->drawMode )
		{
			case OGE_CAMERA_DRAW_MODE_WIREFRAME:
				ogeDrawWorldWireframe( world, &tmp );
				break;
			case OGE_CAMERA_DRAW_MODE_SOLID:
			case OGE_CAMERA_DRAW_MODE_TEXTURED:
				ogeDrawWorld( world, NULL, &tmp );
				break;
			default:
				break;
		}

		// reset the view matrix back to it's original state
		PlgSetViewMatrix( &tmp.internal->internal.view );
	}

	BitmapFont *defaultFont = Font_GetDefault();
	if ( defaultFont == NULL )
	{
		return;
	}

	Font_BeginDraw( defaultFont );

	const char *label;
	if ( context.camera != NULL )
	{
		switch ( context.camera->mode )
		{
			default:
			case OGE_CAMERA_MODE_FRONT:
				label = "Front";
				break;
			case OGE_CAMERA_MODE_LEFT:
				label = "Left";
				break;
			case OGE_CAMERA_MODE_PERSPECTIVE:
				label = "Perspective";
				break;
			case OGE_CAMERA_MODE_TOP:
				label = "Top";
				break;
		}
	}
	else
	{
		label = "No camera!";
	}

	Font_AddBitmapStringToPass( defaultFont,
	                            ( float ) ( ( w - ( defaultFont->cw * 2 ) ) - ( defaultFont->cw * strlen( label ) ) ),
	                            ( float ) ( h - ( defaultFont->ch * 2 ) ),
	                            1.0f, PL_COLOUR_GOLD, label, strlen( label ), true );

	Font_Draw( defaultFont );

	PlgSetShaderProgram( oge_defaultShaderPrograms_[ OGE_SHADER_DEFAULT_VERTEX ] );
	static const float CURSOR_SIZE = 8.0f;
	PlgDrawRectangle( ( float ) ( mouseCursorX ) - ( CURSOR_SIZE / 2 ), ( float ) ( mouseCursorY ) - ( CURSOR_SIZE / 2 ),
	                  CURSOR_SIZE, CURSOR_SIZE,
	                  PL_COLOUR_RED );
}

static void TickWorldEditor( void )
{
	ogeGetMousePosition( &mouseCursorX, &mouseCursorY );
	mouseCursorX = PlRoundUp( mouseCursorX, context.gridScale * 4 );
	mouseCursorY = PlRoundUp( mouseCursorY, context.gridScale * 4 );
}

static void OnWorldEditorActive( void )
{
	OgeViewport *viewport = YnCore_Viewport_GetBySlot( 0 );
	assert( viewport != NULL );
	if ( viewport == NULL )
	{
		return;
	}

	YnCore_Viewport_SetCamera( viewport, context.camera );

	world = Game_GetCurrentWorld();
}

YNCoreEditorContext *YnCore_RegisterWorldEditorContext( void )
{
	PL_ZERO_( context );

	context.name       = "World Editor";
	context.identifier = WORLD_CONTEXT_IDENTIFIER;
	context.mode       = YN_CORE_EDITOR_CONTEXT_WORLD;

	context.RegisterConsoleVariables = RegisterWorldEditorVariables;
	context.Initialize               = InitializeWorldEditor;
	context.Shutdown                 = ShutdownWorldEditor;
	context.Draw                     = DrawWorldEditor;
	context.DrawGUI                  = DrawWorldEditorGUI;
	context.Tick                     = TickWorldEditor;

	context.OnActive = OnWorldEditorActive;

	return &context;
}
