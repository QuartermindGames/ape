// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "ape_client_gui.h"
#include "yin/core_interfaces.h"
#include "editor/editor.h"
#include "game/game_interface.h"
#include "client/renderer/renderer.h"
#include "client/renderer/renderer_render_target.h"
#include "client/renderer/post/post.h"

static GuiCanvas *canvas;

static const GuiStyleSheet *defaultStyle;
static GuiPanel *rootPanel;
static GuiPanel *cursor;

static int guiWidth = 800;
static int guiHeight = 600;

static bool drawGUI = false;

static ApeMaterial *baseGuiMat;

void apeInitializeGUI_( void )
{
	PlRegisterConsoleVariable( "gui_draw", "Enable/disable drawing of the GUI.", "0", PL_VAR_BOOL, &drawGUI, NULL, false );
	PlRegisterConsoleVariable( "gui_width", "Width of the GUI canvas.", "800", PL_VAR_I32, &guiWidth, NULL, false );
	PlRegisterConsoleVariable( "gui_height", "Height of the GUI canvas.", "600", PL_VAR_I32, &guiHeight, NULL, false );

	ss_gui_initialize();

	defaultStyle = ss_gui_cache_style_sheet( "guis/styles/default.n" );
	if ( defaultStyle == NULL )
		PRINT_ERROR( "Failed to cache base style for GUI!\n" );

	ss_gui_set_style_sheet( defaultStyle );

	canvas = ss_gui_canvas_create( guiWidth, guiHeight );
	if ( canvas == NULL )
		PRINT_ERROR( "Failed to create GUI canvas!\n" );

	rootPanel = ss_gui_panel_create( NULL, 0, 0, guiWidth, guiHeight, GUI_PANEL_BACKGROUND_NONE, GUI_PANEL_BORDER_NONE );
	if ( rootPanel == NULL )
		PRINT_ERROR( "Failed to create base panel!\n" );

	cursor = ss_gui_cursor_create( rootPanel, 0, 0 );
	if ( cursor == NULL )
		PRINT_ERROR( "Failed to create cursor!\n" );

	ss_gui_panel_set_visible( rootPanel, true );

	baseGuiMat = ss_arl_material_cache( "materials/ui/ui_rt_base.mat.n", APE_CACHE_WORLD, false, false );
	if ( baseGuiMat == NULL )
		PRINT_ERROR( "Failed to cache base material for ui!\n" );
}

void apeShutdownGUI_( void )
{
	ss_gui_panel_destroy( rootPanel );
	ss_gui_shutdown();

	ss_arl_material_release( baseGuiMat );
}

void apeDrawGUI_( SSArlViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );

	// Need to call this again to reset the viewport
	ss_arl_set_2d_viewport_size_( viewport->width, viewport->height );

	SSArlRenderTarget *renderTarget = ss_arl_postfx_get_render_target();
	if ( renderTarget != NULL )
	{
		PLGTexture *texture = ss_arl_render_target_get_texture( renderTarget );
		if ( texture != NULL )
		{
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
	}

	if ( drawGUI )
	{
		gui_canvas_set_size( canvas, guiWidth, guiHeight );
		gui_canvas_draw( canvas, rootPanel );

		// Need to call this again to reset the viewport
		ss_arl_set_2d_viewport_size_( viewport->width, viewport->height );

		// draw the output of the canvas
		ss_arl_draw_quad( baseGuiMat, 0, 0, viewport->width, viewport->height, &PL_COLOUR_WHITE );
	}

	game_modeInterface->requestCallbackMethod( GAME_MODE_REQUEST_DRAW_UI, viewport );

	ss_acl_draw_editor_gui_( viewport );

	// todo: this should use GUI
	PL_GET_CVAR( "debug/overlay", debugOverlay );
	if ( ape_config_.renderer.showFps && debugOverlay->i_value == 0 )
	{
		GuiFont *font = guiGetDefaultFont( GUI_FONT_DEFAULT_MEDIUM );
		assert( font != NULL );
		if ( font != NULL )
		{
			char tmp[ 32 ];
			snprintf( tmp, sizeof( tmp ), "FPS: %u", ss_arl_viewport_get_framerate( viewport ) );
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
	ss_acl_console_draw_( viewport );

	COM_PROFILE_FUNCTION_END();
}

void ss_acl_tick_gui_( void )
{
	gui_panel_tick( rootPanel );
}

void apeResizeGUI( int w, int h )
{
	gui_panel_set_size( rootPanel, w, h );
}

GuiPanel *ss_gui_get_root_panel( void )
{
	return rootPanel;
}
