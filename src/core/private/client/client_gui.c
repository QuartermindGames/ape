// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "client_gui.h"

#include "editor/editor.h"

#include "game_interface.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_font.h"
#include "legacy/actor.h"

static GUICanvas *canvas;

static const GUIStyleSheet *defaultStyle;
static GUIPanel *rootPanel;
static GUIPanel *cursor;

static int guiWidth  = 800;
static int guiHeight = 600;

static bool drawGUI = false;

static OgeMaterial *baseGuiMat;

void YnCore_InitializeGUI( void )
{
	PlRegisterConsoleVariable( "gui.draw", "Enable/disable drawing of the GUI.", "0", PL_VAR_BOOL, &drawGUI, NULL, false );
	PlRegisterConsoleVariable( "gui.width", "Width of the GUI canvas.", "800", PL_VAR_I32, &guiWidth, NULL, false );
	PlRegisterConsoleVariable( "gui.height", "Height of the GUI canvas.", "600", PL_VAR_I32, &guiHeight, NULL, false );

	OgeRenderTarget *guiTarget = ogeRenderTarget_Create( "gui", 640, 480, PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
	if ( guiTarget == NULL )
	{
		PRINT_ERROR( "Failed to create default render target for GUI!\n" );
	}

	baseGuiMat = ogeCacheMaterial( "materials/ui/ui_rt_base.mat.n", YN_CORE_CACHE_GROUP_WORLD, false, false );
	if ( baseGuiMat == NULL )
	{
		PRINT_ERROR( "Failed to cache base material for ui!\n" );
	}

	GUI_Initialize();

	defaultStyle = GUI_CacheStyleSheet( "guis/styles/default.n" );
	if ( defaultStyle == NULL )
	{
		PRINT_ERROR( "Failed to cache base style for GUI!\n" );
	}

	GUI_SetStyleSheet( defaultStyle );

	canvas = GUI_CreateCanvas( guiWidth, guiHeight );
	if ( canvas == NULL )
	{
		PRINT_ERROR( "Failed to create GUI canvas!\n" );
	}

	rootPanel = GUI_Panel_Create( NULL, 0, 0, guiWidth, guiHeight, GUI_PANEL_BACKGROUND_NONE, GUI_PANEL_BORDER_NONE );
	if ( rootPanel == NULL )
	{
		PRINT_ERROR( "Failed to create base panel!\n" );
	}

	cursor = GUI_Cursor_Create( rootPanel, 0, 0 );
	if ( cursor == NULL )
	{
		PRINT_ERROR( "Failed to create cursor!\n" );
	}

	GUI_Panel_SetVisible( rootPanel, true );
}

void YnCore_ShutdownGUI( void )
{
	GUI_Panel_Destroy( rootPanel );
	GUI_Shutdown();
}

void YnCore_DrawGUI( const OgeViewport *viewport )
{
	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );

	// Need to call this again to reset the viewport
	YnCore_Set2DViewportSize( viewport->width, viewport->height );

	PLGTexture *texture;
	if ( ( texture = ogeGetPrimaryColourAttachment() ) != NULL )
	{
		float x = ( float ) viewport->x;
		float y = ( float ) viewport->y;
		float w = ( float ) viewport->width;
		float h = ( float ) viewport->height;

		PlgSetCullMode( PLG_CULL_NEGATIVE );

		PlgSetShaderProgram( oge_defaultShaderPrograms_[ OGE_SHADER_DEFAULT ] );
		PlgSetTexture( texture, 0 );

		PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );

		PlgImmPushVertex( x, y + h, 0.0f );
		PlgImmColour( 255, 255, 255, 255 );
		PlgImmTextureCoord( 0.0f, 0.0f );

		PlgImmPushVertex( x, y, 0.0f );
		PlgImmColour( 255, 255, 255, 255 );
		PlgImmTextureCoord( 0.0f, 1.0f );

		PlgImmPushVertex( x + w, y + h, 0.0f );
		PlgImmColour( 255, 255, 255, 255 );
		PlgImmTextureCoord( 1.0f, 0.0f );

		PlgImmPushVertex( x + w, y, 0.0f );
		PlgImmColour( 255, 255, 255, 255 );
		PlgImmTextureCoord( 1.0f, 1.0f );

		PlgImmDraw();

		PlgSetCullMode( PLG_CULL_POSITIVE );
	}

	OGE_PROFILE_START( PROFILE_DRAW_GUI );
	if ( drawGUI )
	{
		// todo: no built-in shaders for GUI yet, just assumes we have one bound... urgh
		PlgSetShaderProgram( oge_defaultShaderPrograms_[ OGE_SHADER_DEFAULT_VERTEX ] );
		GUI_SetCanvasSize( canvas, guiWidth, guiHeight );
		GUI_Draw( canvas, rootPanel );

		YnCore_Draw2DQuad( baseGuiMat, 0, 0, viewport->width, viewport->height );
	}
	OGE_PROFILE_END( PROFILE_DRAW_GUI );

	if ( gameModeInterface->DrawMenu != NULL )
	{
		gameModeInterface->DrawMenu( viewport );
	}

	YnCore_DrawEditorGUI( viewport );

	// todo: this should use GUI
	PL_GET_CVAR( "debug.overlay", debugOverlay );
	PL_GET_CVAR( "r.showFPS", showFPS );
	if ( showFPS->b_value && debugOverlay->i_value == 0 )
	{
		char tmp[ 32 ];
		snprintf( tmp, sizeof( tmp ), "FPS: %u", YnCore_Viewport_GetAverageFPS( viewport ) );
		GUI_Font_DrawString( GUI_Font_GetDefault( GUI_FONT_DEFAULT_MEDIUM ),
		                     10.0f, 10.0f, NULL, NULL,
		                     1.0f,
		                     &PL_COLOUR_GOLD,
		                     tmp, strlen( tmp ),
		                     false );
	}

	// todo: this should use GUI
	ogeDrawConsole_( viewport );
}

void YnCore_TickGUI( void )
{
	GUI_Tick( rootPanel );
}

void YnCore_ResizeGUI( int w, int h )
{
	GUI_Panel_SetSize( rootPanel, w, h );
}

GUIPanel *YnCore_GetGUIRootPanel( void )
{
	return rootPanel;
}
