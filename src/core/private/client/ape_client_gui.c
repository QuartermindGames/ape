// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "ape_client_gui.h"

#include "yin/core_interfaces.h"

#include "editor/editor.h"

#include "game/game_public.h"

#include "renderer/renderer.h"
#include "renderer/renderer_font.h"
#include "renderer/post/post.h"

static bool drawGUI = true;

static ApeMaterial *baseGuiMat;

static ApeCamera *auxCamera;

static void draw_debug_overlay( ApeViewport *viewport )
{
	PL_GET_CVAR( "debug/overlay", debugOverlay );
	if ( debugOverlay->i_value <= 0 )
		return;

	COM_PROFILE_FUNCTION_START();

	ApeBitmapFont *defaultFont = ape_get_default_small_bitmap_font();
	ape_bitmap_font_begin_draw( defaultFont );

	static const float sy = 8;
	static const float sx = 8;
	static const float tx = 8 + 4;
	float              y  = sy;

	float ch = ( float ) defaultFont->ch;

	const ApeCamera *camera = viewport->camera;
	if ( camera != nullptr )
	{
		// Draw camera position
		char buf[ 128 ];
		PL_ZERO( buf, sizeof( buf ) );
		const char *vpos = PlPrintVector3( &camera->internal->position, PL_VAR_I32 );
		S_STRCAT( buf, vpos );
		S_STRCAT( buf, " (" );
		const char *vang = PlPrintVector3( &camera->internal->angles, PL_VAR_I32 );
		S_STRCAT( buf, vang );
		S_STRCAT( buf, ")" );
		ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_WHITE, buf, strlen( buf ), false );
	}

	// Draw stats
	char buf[ 64 ];
	snprintf( buf, sizeof( buf ), "FPS:              " PL_FMT_uint32 "\n", ape_viewport_get_framerate( viewport ) );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num rooms:        " PL_FMT_uint32 "\n", ape_rendererPerformance_.numRooms );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num detail rooms: " PL_FMT_uint32 "\n", ape_rendererPerformance_.numDetailRooms );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num portals:      " PL_FMT_uint32 "\n", ape_rendererPerformance_.numVisiblePortals );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num faces:        " PL_FMT_uint32 "\n", ape_rendererPerformance_.numFacesDrawn );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num lights:       " PL_FMT_uint32 "\n", ape_rendererPerformance_.numLights );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num triangles:    " PL_FMT_uint32 "\n", ape_rendererPerformance_.numTriangles );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num batches:      " PL_FMT_uint32 "\n", ape_rendererPerformance_.numBatches );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "---------------------\n" );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_WHITE, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Alloc memory:     %.2lfMB\n", PlBytesToMegabytes( PlGetTotalAllocatedMemory() ) );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_ORCHID, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Total memory:     %.2lfMB\n", PlBytesToMegabytes( PlGetCurrentMemoryUsage() ) );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_ORCHID, buf, strlen( buf ), false );

	unsigned int numTasks = apeGetNumScheduledTasks();
	snprintf( buf, sizeof( buf ), "Num tasks:     " PL_FMT_uint32 "\n", numTasks );
	ape_bitmap_font_batch_string( defaultFont, tx, y += ch, 1.0f, PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	for ( unsigned int i = 0; i < numTasks; ++i )
	{
		double      taskDelay;
		const char *taskDescription = apeGetScheduledTaskDescription( i, &taskDelay );
		snprintf( buf, sizeof( buf ), "%u %s\n", i, taskDescription );
		ape_bitmap_font_batch_string( defaultFont, tx + 8, y += ch, 1.0f, PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	}
	y += ch * 2;

	static const float bw = 128;

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgSetBlendMode( PLG_BLEND_DEFAULT );
	PlgDrawRectangle( sx, sy, bw, y - sy, PLColour( 0, 0, 0, 200 ) );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	ape_bitmap_font_draw( defaultFont );

	if ( debugOverlay->i_value > 1 )
	{
		static const float Y_SPACING    = 4.0f;
		static const float X_SPACING    = 4.0f;
		static const float GRAPH_HEIGHT = 32.0f;

		y += Y_SPACING;

		float x = sx;

		ComProfilingGroup *group = com_profiler_get_first_group();
		while ( group != nullptr )
		{
			if ( y + GRAPH_HEIGHT >= ( float ) viewport->height )
			{
				y = sy;
				x += ( bw + X_SPACING );
			}

			unsigned int  numPoints;
			const double *graph = com_profiler_get_samples( group, &numPoints );
			const char   *name  = com_profiler_get_group_name( group );
			ape_draw_graph( name, x, y, bw, GRAPH_HEIGHT, graph, numPoints, .0f, 1.0f );
			y += GRAPH_HEIGHT + Y_SPACING;

			group = com_profiler_get_next_group( group );
		}
	}

	COM_PROFILE_FUNCTION_END();
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_gui_( void )
{
	PlRegisterConsoleVariable( "gui_draw", "Enable/disable drawing of the GUI.", "true", PL_VAR_BOOL, &drawGUI, nullptr, false );

	auxCamera = ape_create_camera( nullptr, nullptr, &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_FRONT, APE_CAMERA_DRAW_MODE_SHADED );
	if ( auxCamera == nullptr )
	{
		ape_error_( true, "Failed to create auxiliary camera!\n" );
	}

	ape_gui_initialize_();
}

void ape_shutdown_gui_( void )
{
	ape_gui_shutdown_();

	ape_material_release( baseGuiMat );
}

void ape_set_2d_viewport_size_( int w, int h )
{
	PlgSetViewport( 0, 0, w, h );
	PlgSetupCamera( ape_camera_get_internal( auxCamera ) );
}

void ape_get_2d_viewport_size_( int *width, int *height )
{
	PlgGetViewport( nullptr, nullptr, width, height );
}

void ape_flare_draw_( const ApeViewport *viewport );

void ape_draw_gui_( ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	if ( !drawGUI )
	{
		COM_PROFILE_FUNCTION_END();
		return;
	}

	PlgBindFrameBuffer( nullptr, PLG_FRAMEBUFFER_DRAW );

	// Need to call this again to reset the viewport
	ape_set_2d_viewport_size_( viewport->width, viewport->height );

	float x = ( float ) viewport->x;
	float y = ( float ) viewport->y;
	float w = ( float ) viewport->width;
	float h = ( float ) viewport->height;

	ApeRenderTarget *renderTarget = ape_postfx_get_render_target();
	if ( renderTarget != nullptr )
	{
		PLGTexture *texture = ape_render_target_get_texture( renderTarget );
		if ( texture != nullptr )
		{
			ApeShaderProgram *program = ape_get_default_shader( APE_SHADER_DEFAULT );
			PlgSetShaderProgram( program->internal );
			PlgSetTexture( texture, 0 );

			ape_draw_textured_quad( nullptr, x, y, w, h, &PL_COLOUR_WHITE );
		}
	}

	ape_flare_draw_( viewport );

	if ( !ape_is_editor_active() )
	{
		ape_gameInterface->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_DRAW_UI, viewport );
	}

	ape_editor_draw_gui_( viewport );

	// todo: this should use GUI
	PL_GET_CVAR( "debug/overlay", debugOverlay );
	if ( ape_config_.renderer.showFps && debugOverlay->i_value == 0 )
	{
		char tmp[ 32 ];
		snprintf( tmp, sizeof( tmp ), "FPS: %u", ape_viewport_get_framerate( viewport ) );

		ApeGuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_MEDIUM );
		gui_font_draw_string( font, 10.0f, 10.0f, nullptr, nullptr, 1.0f, &PL_COLOUR_GOLD, tmp, strlen( tmp ), false );
		gui_font_display( font );
	}

#if !defined( NDEBUG )
	if ( !ape_is_editor_active() )
	{
		static const char *buildIdentifier = "DEBUG - VERSION[" ENGINE_VERSION_STR "] BUILD[" GIT_COMMIT_COUNT "] BRANCH[" GIT_BRANCH "]";
		float              sw, sh;
		ApeGuiFont        *font = gui_get_default_font( GUI_FONT_DEFAULT_TINY );
		gui_font_get_string_pixel_size( font, 1.0f, buildIdentifier, strlen( buildIdentifier ), &sw, &sh );
		gui_font_draw_string( font, w / 2 - ( sw / 2 ), h - sh, nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, buildIdentifier, strlen( buildIdentifier ), false );
		gui_font_display( font );
	}
#endif

	// todo: this should use GUI
	ape_console_draw_( viewport );

	COM_PROFILE_FUNCTION_END();
}

void ape_draw_menu_( ApeViewport *viewport )
{
	if ( viewport == nullptr )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	ape_viewport_make_active( viewport );
	ape_set_2d_viewport_size_( viewport->width, viewport->height );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	ape_postfx_draw_( viewport );

	ape_draw_gui_( viewport );

	draw_debug_overlay( viewport );

	PlgSetTexture( nullptr, 0 );

	PlPopMatrix();

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );

	COM_PROFILE_FUNCTION_END();
}

void ape_console_update_notifications_( double delta );// client_console.c

void ape_tick_gui_( double delta )
{
	COM_PROFILE_FUNCTION_START();

	ape_console_update_notifications_( delta );

	COM_PROFILE_FUNCTION_END();
}
