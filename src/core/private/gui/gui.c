// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "qmos/public/qm_os_random.h"

#include "gui_private.h"
#include "ape/ape_public_game.h"
#include "editor/editor.h"
#include "renderer/renderer.h"
#include "renderer/material/material.h"
#include "renderer/post/post.h"
#include "yin/core_game.h"
#include "core_console.h"

//TODO: eventually this should be used, rather than what we're doing
#define USE_GUI_CANVAS 0

#if USE_GUI_CANVAS == 1

static ApeGuiCanvas *guiBaseCanvas;

#endif

static bool guiDraw            = true;
static bool guiProfilerOverlay = false;

static constexpr char POST_MATERIAL_PATH[] = "materials/engine/engine_viewport.mat.n";
static ApeMaterial   *postMaterial;

static float profilerWidth  = 512;
static float profilerHeight = 256;

ApeGUIState ape_guiState_;

bool ape_gui_initialize_fonts_();

/**
 * Initialize the GUI sub-system.
 */
bool ape_gui_initialize_( void )
{
	ape_guiState_ = ( ApeGUIState ) {};

	ape_console_var_register( "gui.draw", "Enable/disable drawing of the GUI.", "true", PL_VAR_BOOL, &guiDraw, nullptr, 0 );
	ape_console_var_register( "gui.profiler", "Enable profiler.", "false", PL_VAR_BOOL, &guiProfilerOverlay, nullptr, 0 );
	ape_console_var_register( "gui.profilerWidth", "Set the width of the on-screen profiler.", "512", PL_VAR_F32, &profilerWidth, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
	ape_console_var_register( "gui.profilerHeight", "Set the width of the on-screen profiler.", "256", PL_VAR_F32, &profilerHeight, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );

#if USE_GUI_CANVAS == 1

	guiBaseCanvas = ape_gui_canvas_create( 640, 480 );
	if ( guiBaseCanvas == nullptr )
	{
		ape_console_error_( true, "Failed to create base canvas!\n" );
	}

#endif

	postMaterial = ape_material_cache( POST_MATERIAL_PATH, APE_CACHE_GROUP_GLOBAL, false );
	if ( postMaterial == nullptr )
	{
		ape_console_warning_( "Failed to find viewport material (%s); post-processing effects will not work!\n", POST_MATERIAL_PATH );
	}

	if ( !ape_gui_initialize_fonts_() )
	{
		ape_console_error_( true, "Font initialization failed!\n" );
	}

	ape_console_print_( "GUI initialized!\n" );
	return true;
}

void ape_gui_shutdown_( void )
{
#if USE_GUI_CANVAS == 1

	ape_gui_canvas_destroy( guiBaseCanvas );
	guiBaseCanvas = nullptr;

#endif
}

void ape_gui_update_mouse_position_( int x, int y )
{
	ape_guiState_.mouseOldPos = ape_guiState_.mousePos;
	ape_guiState_.mousePos.x  = x;
	ape_guiState_.mousePos.y  = y;
}

void gui_update_mouse_wheel( float x, float y )
{
	ape_guiState_.mouseOldWheel = ape_guiState_.mouseWheel;
	ape_guiState_.mouseWheel.x  = x;
	ape_guiState_.mouseWheel.y  = y;
}

void guiUpdateMouseButton( GuiMouseButton button, bool isDown )
{
}

static int compare_profiling_group( const void *a, const void *b )
{
	double ta = com_profiler_get_time_average( *( ComProfilingGroup ** ) a );
	double tb = com_profiler_get_time_average( *( ComProfilingGroup ** ) b );
	return ta < tb;
}

static QmMathColour4ub get_profiling_group_colour( const void *p )
{
	unsigned int seed = ( intptr_t ) p;
	return QM_MATH_COLOUR4UB(
	        100 + qm_os_random_int( &seed ) % 155,
	        100 + qm_os_random_int( &seed ) % 155,
	        100 + qm_os_random_int( &seed ) % 155,
	        255 );
}

static void draw_debug_window( const char *title, const float x, const float y, const float w, const float h )
{
	ApeGuiFont *font      = gui_get_default_font( GUI_FONT_DEFAULT_MEDIUM );
	float       barHeight = gui_font_get_line_spacing( font );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLES );

	// title bar
	if ( title != nullptr )
	{
		ape_draw_rectangle_( mesh, x, y - barHeight, w, barHeight, &QM_MATH_COLOUR4UB( 255, 255, 255, 200 ) );
	}

	ape_draw_rectangle_( mesh, x, y, w, h, &QM_MATH_COLOUR4UB( 0, 0, 0, 200 ) );

	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX_ALPHA );
	ape_material_draw( material, mesh, nullptr );

	if ( title == nullptr )
	{
		return;
	}

	gui_font_draw_string( font, x + 8.0f, y - barHeight, nullptr, nullptr, 1.0f, &QM_MATH_COLOUR4UB( 0, 0, 0, 255 ), title, strlen( title ), false );
	gui_font_display( font );
}

static void draw_profiler( const ApeViewport *viewport )
{
	// buildup a list of all the profiling groups
	static constexpr unsigned int MAX_PROFILING_GROUPS = 256;
	unsigned int                  numProfilingGroups;
	const ComProfilingGroup      *groups[ MAX_PROFILING_GROUPS ];
	const ComProfilingGroup      *group = com_profiler_get_first_group();
	for ( numProfilingGroups = 0; group != nullptr; ++numProfilingGroups )
	{
		if ( numProfilingGroups >= MAX_PROFILING_GROUPS )
		{
			ape_console_warning_( "Hit profiling group limit!\n" );
			break;
		}

		groups[ numProfilingGroups ] = group;

		group = com_profiler_get_next_group( group );
	}

	// now sort them based on whichever is taking the longest
	qsort( groups, numProfilingGroups, sizeof( ComProfilingGroup * ), compare_profiling_group );

	// setup the areas we'll be drawing to
	// mmm yes, numbers...

	static constexpr float PADDING = 4.0f;

	float graphW = profilerWidth;
	float graphH = profilerHeight;
	float graphX = viewport->width - graphW - PADDING;
	float graphY = viewport->height - graphH - PADDING;

	float sidebarW = graphW / 3.0f;
	float sidebarH = graphH - 16.0f - PADDING * 2.0f;
	float sidebarX = graphX + graphW - sidebarW - PADDING * 2.0f;
	float sidebarY = graphY + PADDING * 2.0f;

	float profW = graphW - sidebarW - PADDING * 4.0f;
	float profH = graphH - 16.0f - PADDING * 2.0f;
	float profX = graphX + PADDING * 2.0f;
	float profY = graphY + PADDING * 2.0f;

	// draw the background
	draw_debug_window( "CPU Profiler", graphX, graphY, graphW, graphH );

	//TODO: this clipping API sucks balls... I need to rework it!
	qm_gfx_clip_viewport( graphX, viewport->height - graphY - 1.0f - graphH, graphW, graphH );

	float sx = sidebarX + PADDING;
	float sy = sidebarY + PADDING;

	static float min = 0.0f;
	static float max = 0.0f;

	// now draw in all the colour coded group names
	ApeGuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_SMALL );
	for ( unsigned int i = 0; i < numProfilingGroups; ++i )
	{
		QmMathColour4ub colour = get_profiling_group_colour( groups[ i ] );

		char tmp[ 64 ];
		snprintf( tmp, sizeof( tmp ), "%.2f %s\n", com_profiler_get_time_average( groups[ i ] ), com_profiler_get_group_name( groups[ i ] ) );
		gui_font_draw_string( font, sx, sy, nullptr, &sy, 1.0f, &colour, tmp, strlen( tmp ), false );

		// determine the min and max while we're here
		unsigned int  numPoints;
		const double *points = com_profiler_get_samples( groups[ i ], &numPoints );
		for ( unsigned int j = 0; j < numPoints; ++j )
		{
			if ( points[ i ] > max )
			{
				max = points[ i ];
			}
			if ( points[ i ] < min )
			{
				min = points[ i ];
			}
		}
	}

	gui_font_display( font );

	// now let's draw all the graphs
	for ( unsigned int i = 0; i < numProfilingGroups; ++i )
	{
		QmMathColour4ub colour = get_profiling_group_colour( groups[ i ] );

		unsigned int  numPoints;
		const double *points = com_profiler_get_samples( groups[ i ], &numPoints );

		PLGMesh *lineMesh = PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );

		for ( unsigned int j = 1; j < numPoints; ++j )
		{
			QmMathVector2f point;
			point.x = profX + profW / ( numPoints - 1 ) * ( j - 1 );
			point.y = profY + profH - 1 - ( points[ j - 1 ] - min ) * ( profH / ( max - min ) );

			PlgPushVertex3f( lineMesh, point.x, point.y, 0.0f );
			PlgColour4bv( lineMesh, &colour );

			PlgPushVertex3f( lineMesh, point.x, profY + profH, 0.0f );
			PlgColour4bv( lineMesh, &colour );
		}

		ape_material_draw( ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX_ALPHA ), lineMesh, nullptr );
	}

	ape_viewport_set_clip( viewport );
}

static void draw_debug_overlay( ApeViewport *viewport )
{
	if ( !guiProfilerOverlay )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	ApeGuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_TINY );

	static constexpr float sy = 4.0f;
	static constexpr float sx = 4.0f;
	static constexpr float tx = sx + 4.0f;
	float                  y  = sy;

	// Draw stats
	char buf[ 64 ];

	snprintf( buf, sizeof( buf ), "FPS:              %u\n", ape_viewport_get_framerate( viewport ) );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "Num rooms:        %u\n", ape_rendererPerformance_.numRooms );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "Num detail rooms: %u\n", ape_rendererPerformance_.numDetailRooms );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "Num portals:      %u\n", ape_rendererPerformance_.numVisiblePortals );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "Num faces:        %u\n", ape_rendererPerformance_.numFacesDrawn );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "Num lights:       %u\n", ape_rendererPerformance_.numLights );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "Num triangles:    %u\n", ape_rendererPerformance_.numTriangles );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "Num batches:      %u\n", ape_rendererPerformance_.numBatches );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "---------------------\n" );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Alloc memory:     %.2lfMB\n", PlBytesToMegabytes( qm_os_memory_get_total() ) );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Total memory:     %.2lfMB\n", PlBytesToMegabytes( qm_os_memory_get_usage() ) );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_GOLD, buf, strlen( buf ), false );

	unsigned int numTasks = ape_scheduler_get_num_tasks_();
	snprintf( buf, sizeof( buf ), "Num tasks:     %u\n", numTasks );
	gui_font_draw_string( font, tx, y, nullptr, &y, 1.0f, &PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	for ( unsigned int i = 0; i < numTasks; ++i )
	{
		double      taskDelay;
		const char *taskDescription = ape_scheduler_get_task_desc_( i, &taskDelay );
		snprintf( buf, sizeof( buf ), "%u %s\n", i, taskDescription );
		gui_font_draw_string( font, tx + 8.0f, y, nullptr, &y, 1.0f, &PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	}

	// draw the background
	draw_debug_window( nullptr, sx, sy, 300.0f, y - sy );

	gui_font_display( font );

	draw_profiler( viewport );

	COM_PROFILE_FUNCTION_END();
}

void ape_flare_draw_( const ApeViewport *viewport );
void ape_gui_draw_( ApeViewport *viewport )
{
	if ( !guiDraw )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

#if USE_GUI_CANVAS == 0

	// Need to call this again to reset the viewport
	ape_viewport_make_active( viewport );
	ape_setup_2d_viewport_( viewport->width, viewport->height );

#else

	ape_gui_canvas_make_active( guiBaseCanvas );

#endif

	if ( ape_postfx_is_enabled_() && postMaterial != nullptr )
	{
		ape_draw_textured_quad( postMaterial, viewport->x, viewport->y, viewport->width, viewport->height, &PL_COLOUR_WHITE, 0 );
	}

	//TODO: whaa... these have nothing to do with the gui!?
	ape_flare_draw_( viewport );

	if ( !ape_editor_is_active() && ape_gameInterface->drawUI != nullptr )
	{
		ape_gameInterface->drawUI( viewport );
	}

	ape_editor_draw_gui_( viewport );

	PL_GET_CVAR( "renderer.showPortalVolumes", showPortalVolumes );
	if ( showPortalVolumes && showPortalVolumes->b_value )
	{
		const ApeCamera *camera = ape_rendererState_.camera;
		if ( camera != nullptr && camera->pvs.numRooms > 0 )
		{
			ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

			const ApeCameraVisibleRoom *room = &camera->pvs.rooms[ 0 ];
			for ( unsigned int i = 0; i < room->numPortals; ++i )
			{
				QmMathVector4f screenRect = room->portals[ i ].screenRect;
				screenRect.y              = viewport->height - screenRect.y - screenRect.w;
				PlgDrawLineRectangle( screenRect.x, screenRect.y, screenRect.z, screenRect.w, PL_COLOUR_GREEN );
			}
		}
	}

	if ( ape_config_.renderer.showFps && !guiProfilerOverlay )
	{
		char tmp[ 32 ];
		snprintf( tmp, sizeof( tmp ), "FPS: %u", ape_viewport_get_framerate( viewport ) );

		ApeGuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_MEDIUM );
		gui_font_draw_string( font, 10.0f, 10.0f, nullptr, nullptr, 1.0f, &PL_COLOUR_GOLD, tmp, strlen( tmp ), false );
		gui_font_display( font );
	}

	// todo: this should use GUI
	ape_console_draw_( viewport );

	draw_debug_overlay( viewport );

	ape_renderer_batch_display_();

	ape_clear_flare_queue_();

#if USE_GUI_CANVAS == 1

	ape_gui_canvas_display( guiBaseCanvas );

#endif

	COM_PROFILE_FUNCTION_END();
}
