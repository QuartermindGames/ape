// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "ape_client_gui.h"
#include "yin/core_interfaces.h"
#include "editor/editor.h"
#include "game/game_interface.h"
#include "client/renderer/renderer.h"

static GuiCanvas *canvas;

static const GuiStyleSheet *defaultStyle;
static GuiPanel *rootPanel;
static GuiPanel *cursor;

static int guiWidth = 800;
static int guiHeight = 600;

static bool drawGUI = false;

static ApeMaterial *baseGuiMat;

void apeInitializeGUI_( void ) {
	PlRegisterConsoleVariable( "gui/draw", "Enable/disable drawing of the GUI.", "0", PL_VAR_BOOL, &drawGUI, NULL, false );
	PlRegisterConsoleVariable( "gui/width", "Width of the GUI canvas.", "800", PL_VAR_I32, &guiWidth, NULL, false );
	PlRegisterConsoleVariable( "gui/height", "Height of the GUI canvas.", "600", PL_VAR_I32, &guiHeight, NULL, false );

	guiInitialize();

	defaultStyle = guiCacheStyleSheet( "guis/styles/default.n" );
	if ( defaultStyle == NULL ) {
		PRINT_ERROR( "Failed to cache base style for GUI!\n" );
	}

	guiSetStyleSheet( defaultStyle );

	canvas = guiCreateCanvas( guiWidth, guiHeight );
	if ( canvas == NULL ) {
		PRINT_ERROR( "Failed to create GUI canvas!\n" );
	}

	rootPanel = guiCreatePanel( NULL, 0, 0, guiWidth, guiHeight, GUI_PANEL_BACKGROUND_NONE, GUI_PANEL_BORDER_NONE );
	if ( rootPanel == NULL ) {
		PRINT_ERROR( "Failed to create base panel!\n" );
	}

	cursor = guiCreateCursor( rootPanel, 0, 0 );
	if ( cursor == NULL ) {
		PRINT_ERROR( "Failed to create cursor!\n" );
	}

	guiSetPanelVisible( rootPanel, true );

	baseGuiMat = apeCacheMaterial( "materials/ui/ui_rt_base.mat.n", APE_CACHE_WORLD, false, false );
	if ( baseGuiMat == NULL ) {
		PRINT_ERROR( "Failed to cache base material for ui!\n" );
	}
}

void apeShutdownGUI_( void ) {
	guiDestroyPanel( rootPanel );
	guiShutdown();

	apeReleaseMaterial( baseGuiMat );
}

void apeDrawGUI_( const ApeViewport *viewport ) {
	COM_PROFILE_FUNCTION_START();

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );

	// Need to call this again to reset the viewport
	apeSet2DViewportSize( viewport->width, viewport->height );

	PLGTexture *texture;
	if ( ( texture = apeGetPrimaryColourAttachment() ) != NULL ) {
		float x = ( float ) viewport->x;
		float y = ( float ) viewport->y;
		float w = ( float ) viewport->width;
		float h = ( float ) viewport->height;

		PlgSetCullMode( PLG_CULL_NEGATIVE );

		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );
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

	if ( drawGUI ) {
		guiSetCanvasSize( canvas, guiWidth, guiHeight );
		guiDraw( canvas, rootPanel );

		// Need to call this again to reset the viewport
		apeSet2DViewportSize( viewport->width, viewport->height );

		// draw the output of the canvas
		apeDraw2DQuad( baseGuiMat, 0, 0, viewport->width, viewport->height );
	}

	if ( game_modeInterface->DrawMenu != NULL ) {
		game_modeInterface->DrawMenu( viewport );
	}

	apeDrawEditorGUI_( viewport );

	// todo: this should use GUI
	PL_GET_CVAR( "debug/overlay", debugOverlay );
	PL_GET_CVAR( "r/showFPS", showFPS );
	if ( showFPS->b_value && debugOverlay->i_value == 0 ) {
		GuiFont *font = guiGetDefaultFont( GUI_FONT_DEFAULT_MEDIUM );
		assert( font != NULL );
		if ( font != NULL ) {
			char tmp[ 32 ];
			snprintf( tmp, sizeof( tmp ), "FPS: %u", apeGetViewportFramerate( viewport ) );
			guiDrawFontString( font, 10.0f, 10.0f, NULL, NULL, 1.0f, &PL_COLOUR_GOLD, tmp, strlen( tmp ), false );
			guiDisplayFont( font );
		}
	}

#if 0
	extern PLVector2 screenPosTest;
	GuiFont *font = guiGetDefaultFont( GUI_FONT_DEFAULT_MEDIUM );
	assert( font != NULL );
	if ( font != NULL ) {
		char tmp[ 32 ];
		snprintf( tmp, sizeof( tmp ), "coord %s", PlPrintVector2( &screenPosTest, PL_VAR_I32 ) );
		guiDrawFontString( font, screenPosTest.x, screenPosTest.y, NULL, NULL, 1.0f, &PL_COLOUR_ANTIQUE_WHITE, tmp, strlen( tmp ), false );
		guiDisplayFont( font );
	}
#endif

	// todo: this should use GUI
	apeDrawConsole_( viewport );

	COM_PROFILE_FUNCTION_END();
}

void apeTickGUI_( void ) {
	guiTick( rootPanel );
}

void apeResizeGUI( int w, int h ) {
	guiSetPanelSize( rootPanel, w, h );
}

GuiPanel *apeGetDefaultRootPanel( void ) {
	return rootPanel;
}
