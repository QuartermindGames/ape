// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "ape_client_gui.h"

#include "yin/core_interfaces.h"

#include "editor/editor.h"

#include "game/game_interface.h"

#include "renderer/renderer.h"
#include "renderer/renderer_render_target.h"
#include "renderer/renderer_font.h"
#include "renderer/post/post.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static GuiCanvas *canvas;

static const GuiStyleSheet *defaultStyle;
static GuiPanel *rootPanel;
static GuiPanel *cursor;

static int guiWidth = 800;
static int guiHeight = 600;

static bool drawGUI = false;

static ApeMaterial *baseGuiMat;

static SSArlCamera *auxCamera;

static void draw_debug_overlay( const SSArlViewport *viewport )
{
	PL_GET_CVAR( "debug/overlay", debugOverlay );
	if ( debugOverlay->i_value <= 0 )
		return;

	SS_Arl_BitmapFont *defaultFont = ss_arl_get_default_small_bitmap_font();
	assert( defaultFont != NULL );
	if ( defaultFont == NULL )
		return;

	ss_arl_bitmap_font_begin_draw( defaultFont );

	static const float sy = 8;
	static const float sx = 8;
	static const float tx = 8 + 4;
	float y = sy;

	float ch = ( float ) defaultFont->ch;

	const SSArlCamera *camera = viewport->camera;
	if ( camera != NULL )
	{
		// Draw camera position
		char buf[ 128 ];
		PL_ZERO( buf, sizeof( buf ) );
		const char *vpos = PlPrintVector3( &camera->internal->position, PL_VAR_I32 );
		strcat( buf, vpos );
		strcat( buf, " (" );
		const char *vang = PlPrintVector3( &camera->internal->angles, PL_VAR_I32 );
		strcat( buf, vang );
		strcat( buf, ")" );
		ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_WHITE, buf, strlen( buf ), false );
	}

	// Draw stats
	char buf[ 64 ];
	snprintf( buf, sizeof( buf ), "FPS:              " PL_FMT_uint32 "\n", ss_arl_viewport_get_framerate( viewport ) );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num rooms:        " PL_FMT_uint32 "\n", ape_rendererPerformance_.numRooms );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num detail rooms: " PL_FMT_uint32 "\n", ape_rendererPerformance_.numDetailRooms );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num portals:      " PL_FMT_uint32 "\n", ape_rendererPerformance_.numVisiblePortals );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num faces:        " PL_FMT_uint32 "\n", ape_rendererPerformance_.numFacesDrawn );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num lights:       " PL_FMT_uint32 "\n", ape_rendererPerformance_.numLights );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num triangles:    " PL_FMT_uint32 "\n", ape_rendererPerformance_.numTriangles );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num batches:      " PL_FMT_uint32 "\n", ape_rendererPerformance_.numBatches );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "---------------------\n" );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_WHITE, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Alloc memory:     %.2lfMB\n", PlBytesToMegabytes( PlGetTotalAllocatedMemory() ) );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_ORCHID, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Total memory:     %.2lfMB\n", PlBytesToMegabytes( PlGetCurrentMemoryUsage() ) );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_ORCHID, buf, strlen( buf ), false );

	unsigned int numTasks = apeGetNumScheduledTasks();
	snprintf( buf, sizeof( buf ), "Num tasks:     " PL_FMT_uint32 "\n", numTasks );
	ss_arl_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	for ( unsigned int i = 0; i < numTasks; ++i )
	{
		double taskDelay;
		const char *taskDescription = apeGetScheduledTaskDescription( i, &taskDelay );
		snprintf( buf, sizeof( buf ), "%u %s\n", i, taskDescription );
		ss_arl_bitmap_font_batch_string( defaultFont, tx + 8, y += ch, 1.0f, PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	}
	y += ch * 2;

	static const float bw = 128;

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
	PlgSetBlendMode( PLG_BLEND_DEFAULT );
	PlgDrawRectangle( sx, sy, bw, y - sy, PLColour( 0, 0, 0, 200 ) );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	ss_arl_bitmap_font_draw( defaultFont );

	if ( debugOverlay->i_value > 1 )
	{
		static const float Y_SPACING = 4.0f;
		static const float X_SPACING = 4.0f;
		static const float GRAPH_HEIGHT = 32.0f;

		y += Y_SPACING;

		float x = sx;

		ComProfilingGroup *group = comGetFirstProfilingGroup();
		while ( group != NULL )
		{
			if ( y + GRAPH_HEIGHT >= ( float ) viewport->height )
			{
				y = sy;
				x += ( bw + X_SPACING );
			}

			unsigned int numPoints;
			const double *graph = comGetProfilerGroupSamples( group, &numPoints );
			const char *name = comGetProfilingGroupName( group );
			arl_draw_graph( name, x, y, bw, GRAPH_HEIGHT, graph, numPoints, .0f, 1.0f );
			y += GRAPH_HEIGHT + Y_SPACING;

			group = comGetNextProfilingGroup( group );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

PLGCamera *ss_arl_get_aux_camera_( void )
{
	return ss_arl_camera_get_internal( auxCamera );
}

void ss_acl_initialize_gui_( void )
{
	PlRegisterConsoleVariable( "gui_draw", "Enable/disable drawing of the GUI.", "0", PL_VAR_BOOL, &drawGUI, NULL, false );
	PlRegisterConsoleVariable( "gui_width", "Width of the GUI canvas.", "800", PL_VAR_I32, &guiWidth, NULL, false );
	PlRegisterConsoleVariable( "gui_height", "Height of the GUI canvas.", "600", PL_VAR_I32, &guiHeight, NULL, false );

	auxCamera = ss_arl_camera_create( "aux", &pl_vecOrigin3, &pl_vecOrigin3, SS_ARL_CAMERA_MODE_FRONT );
	if ( auxCamera == NULL )
		PRINT_ERROR( "Failed to create auxiliary camera!\n" );

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

void ss_acl_shutdown_gui_( void )
{
	ss_gui_panel_destroy( rootPanel );
	ss_gui_shutdown();

	ss_arl_material_release( baseGuiMat );
}

void ss_arl_set_2d_viewport_size_( int w, int h )
{
	PlgSetViewport( 0, 0, w, h );
	PlgSetupCamera( ss_arl_camera_get_internal( auxCamera ) );
}

void ss_arl_get_2d_viewport_size_( int *width, int *height )
{
	PlgGetViewport( NULL, NULL, width, height );
}

void ss_arl_draw_gui_( SSArlViewport *viewport )
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

void ss_arl_draw_menu_( SSArlViewport *viewport )
{
	if ( viewport == NULL )
		return;

	COM_PROFILE_FUNCTION_START();

	ss_arl_viewport_make_active( viewport );
	ss_arl_set_2d_viewport_size_( viewport->width, viewport->height );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	ss_arl_postfx_draw_( viewport );

	ss_arl_draw_gui_( viewport );
	ss_acl_draw_editor_gui_( viewport );

	draw_debug_overlay( viewport );

	PlgSetTexture( NULL, 0 );

	PlPopMatrix();

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );

	COM_PROFILE_FUNCTION_END();
}

void ss_acl_tick_gui_( void )
{
	gui_panel_tick( rootPanel );
}

void ss_acl_resize_gui_( int w, int h )
{
	gui_panel_set_size( rootPanel, w, h );
}

GuiPanel *ss_gui_get_root_panel( void )
{
	return rootPanel;
}
