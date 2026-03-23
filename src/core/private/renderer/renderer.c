// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <pthread.h>

#include "qmos/public/qm_os_time.h"
#include "qmmath/public/qm_math_plane.h"

#include "ape_private.h"
#include "world/world.h"

#include "renderer.h"

#include "core_console.h"
#include "renderer_render_target.h"
#include "camera/camera.h"

#include "editor/editor.h"
#include "material/material.h"

#include "post/post.h"

ApeRendererStats ape_rendererPerformance_ = {};
//TODO: kill this, caller instead sets up state and passes it into draw call
ApeRendererPassState ape_rendererState_;

static ApeCamera *currentCamera;

static ApeRenderTarget *defaultRenderTarget;

static bool isScreenshotPending = false;

/////////////////////////////////////////////////////////////////////////////////////
// Frame Capture

typedef struct CaptureFrame
{
	unsigned int   w;
	unsigned int   h;
	unsigned char *buf;
} CaptureFrame;

static bool                  useCaptureToQoi = true;
static unsigned int          captureQuality  = 90;
static volatile bool         isCapturing;
static volatile unsigned int numCaptureFrames;

static PLLinkedList *captureQueue;//CaptureFrame

#define MAX_CAPTURE_THREADS 16
static unsigned int    numCaptureThreads = 4;
static pthread_mutex_t captureMutex;
static pthread_t       captureThread[ MAX_CAPTURE_THREADS ];

static void destroy_capture_frame( void *ptr )
{
	CaptureFrame *frame = ptr;
	qm_os_memory_free( frame->buf );
	qm_os_memory_free( frame );
}

static void *process_capture_queue( void * )
{
	while ( true )
	{
		pthread_mutex_lock( &captureMutex );

		PLLinkedListNode *node = PlGetFirstNode( captureQueue );
		if ( node == NULL )
		{
			pthread_mutex_unlock( &captureMutex );
			if ( !isCapturing )
				break;

			nanosleep( &( struct timespec ) { .tv_nsec = 1000000 }, NULL );
			continue;
		}

		CaptureFrame *frame = PlGetLinkedListNodeUserData( node );
		PlDestroyLinkedListNode( node );

		unsigned int frameNum = numCaptureFrames++;

		pthread_mutex_unlock( &captureMutex );

		PLImage *image = PlCreateImage( frame->buf, frame->w, frame->h, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );
		if ( image != NULL )
		{
			PlFlipImageVertical( image );
			PlClearImageAlpha( image );

			PLPath path;
			PlSetupPath( path, true, "%s/captures/%u.%s", com_get_app_data_directory(), frameNum, useCaptureToQoi ? "qoi" : "jpg" );
			PlWriteImage( image, path, captureQuality );

			PlDestroyImage( image );
		}
		else
		{
			ape_console_warning_( "Failed to create image: %s\n", PlGetError() );
		}

		destroy_capture_frame( frame );
	}

	return NULL;
}

static void capture_command( [[maybe_unused]] unsigned int argc, [[maybe_unused]] char **argv )
{
	numCaptureFrames = 0;
	isCapturing      = !isCapturing;
	if ( isCapturing )
	{
		// Create a folder to store the captures if one doesn't already exist
		PLPath captureDirectory;
		PlSetupPath( captureDirectory, true, "%s/captures/", com_get_app_data_directory() );
		PlCreateDirectory( captureDirectory );

		pthread_mutex_init( &captureMutex, nullptr );
		captureQueue = PlCreateLinkedList();

		for ( unsigned int i = 0; i < numCaptureThreads; ++i )
		{
			pthread_create( &captureThread[ i ], nullptr, process_capture_queue, NULL );
		}
	}
	else
	{
		for ( unsigned int i = 0; i < numCaptureThreads; ++i )
		{
			pthread_join( captureThread[ i ], nullptr );
		}

		pthread_mutex_destroy( &captureMutex );
		PlDestroyLinkedListEx( captureQueue, destroy_capture_frame );
	}
}

bool ape_get_capture_state_( void )
{
	return isCapturing;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/**********************************************************/

void ape_setup_default_draw_state_( const ApeViewport *viewport )
{
	QmMathColour4ub clearColour = viewport != nullptr ? viewport->clearColour : QM_MATH_COLOUR4UB( 0, 0, 0, 255 );
	PlgSetClearColour( clearColour );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgDepthBufferFunction( APE_RENDERER_DEFAULT_DEPTH_FUNCTION );
	PlgDepthMask( true );

	PlgSetCullMode( APE_RENDERER_DEFAULT_CULL_FUNCTION );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	ApeShaderProgram *program = ape_get_default_shader( APE_SHADER_DEFAULT );
	PlgSetShaderProgram( program->internal );
}

void ape_draw_begin_( ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	double newTime = qm_os_time_get_seconds();

	viewport->perf.frameReadings[ viewport->perf.frameIndex++ ] = 1.0 / ( newTime - viewport->perf.oldTime );
	if ( viewport->perf.frameIndex >= APE_MAX_FPS_READINGS )
	{
		viewport->perf.frameIndex = 0;
	}

	viewport->perf.oldTime = newTime;

	PL_ZERO_( ape_rendererPerformance_ );

	ape_viewport_make_active( viewport );

	ape_setup_default_draw_state_( viewport );

	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL );

	COM_PROFILE_FUNCTION_END();
}

static void write_screenshot( void )
{
	ApeRenderTarget *renderTarget = ape_postfx_get_render_target_();
	if ( renderTarget == NULL )
	{
		return;
	}

	unsigned int w, h;
	ape_render_target_get_size_( renderTarget, &w, &h );

	PLGFrameBuffer *fboBuffer = ape_render_target_get_frame_buffer_( renderTarget );
	assert( fboBuffer != NULL );

	size_t         bufSize = ( ( w * h ) * 4 );
	unsigned char *buf     = QM_OS_MEMORY_NEW_( unsigned char, bufSize );
	if ( PlgReadFrameBufferRegion( nullptr, 0, 0, w, h, bufSize, buf ) != NULL )
	{
		if ( isCapturing )
		{
			pthread_mutex_lock( &captureMutex );

			CaptureFrame *frame = QM_OS_MEMORY_NEW( CaptureFrame );
			PlInsertLinkedListNode( captureQueue, frame );
			frame->buf = buf;
			frame->w   = w;
			frame->h   = h;

			pthread_mutex_unlock( &captureMutex );
			return;
		}

		PLImage *image = PlCreateImage( buf, w, h, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );
		if ( image != NULL )
		{
			PlFlipImageVertical( image );
			PlClearImageAlpha( image );

			char filename[ 64 ];
			PlGetFormattedTime( "%Y-%m-%d %H-%M-%S", filename, sizeof( filename ) );

			PLPath       path;
			unsigned int num = 0;
			PlSetupPath( path, true, "%s/%s.png", com_get_app_data_directory(), filename );
			while ( PlFileExists( path ) )
			{
				PlSetupPath( path, true, "%s/%s (%u).png", com_get_app_data_directory(), filename, ++num );
			}

			PlWriteImage( image, path, 90 );
			PlDestroyImage( image );
		}
		else
		{
			ape_console_warning_( "Failed to create image for screenshot: %s\n", PlGetError() );
		}
	}
	else
	{
		ape_console_warning_( "Failed to read framebuffer for screenshot: %s\n", PlGetError() );
	}

	qm_os_memory_free( buf );
}

void ape_draw_end_( ApeViewport *viewport )
{
	PlgBindFrameBuffer( nullptr, PLG_FRAMEBUFFER_DEFAULT );

	PLGFrameBuffer *src = ape_render_target_get_frame_buffer_( viewport->renderTarget );
	PlgBlitFrameBuffers( src, src->width, src->height, nullptr, viewport->width, viewport->height, PLG_BUFFER_COLOUR, true );

	ape_rendererPerformance_.numBatches    = 0;
	ape_rendererPerformance_.numTriangles  = 0;
	ape_rendererPerformance_.numFacesDrawn = 0;

	viewport->perf.numBatches   = 0;
	viewport->perf.numTriangles = 0;
	viewport->perf.numPolygons  = 0;
	viewport->perf.numPortals   = 0;

	if ( isScreenshotPending || isCapturing )
	{
		write_screenshot();
		isScreenshotPending = false;
	}
}

void ape_initialize_textures_( void ); /* texture.c */

// renderer_flare.c
void ape_initialize_flares_( void );
void ape_shutdown_flares_( void );
void ape_register_flare_console_variables_( void );

// renderer_rendertarget.c
void ape_initialize_render_targets_( void );
void ape_shutdown_render_targets_( void );

// renderer_shader.c
void ape_initialize_shaders_( void );
void ape_shutdown_shaders_();
void ape_register_shader_console_variables_();

void ape_renderer_batch_initialize_();
void ape_renderer_batch_shutdown_();

void ape_prepare_screenshot_capture_( void )
{
	if ( isCapturing )
	{
		return;
	}

	isScreenshotPending = true;
}

static void prepare_screenshot_capture_command( [[maybe_unused]] unsigned int argc, [[maybe_unused]] char **argv )
{
	ape_prepare_screenshot_capture_();
}

void ape_register_renderer_console_variables_( void )
{
	PlRegisterConsoleCommand( "screenshot", "Take a screenshot.", 0, prepare_screenshot_capture_command );

	PlRegisterConsoleCommand( "capture", "Capture frames continuously until called again.", 0, capture_command );
	PlRegisterConsoleVariable( "capture.numThreads", "Specify the number of threads to use for capturing.", "4", PL_VAR_I32, &numCaptureThreads, nullptr, true );
	PlRegisterConsoleVariable( "capture.useQoi", "Capture to qoi format, rather than jpeg, which is faster but less supported.", "true", PL_VAR_BOOL, &useCaptureToQoi, nullptr, true );
	PlRegisterConsoleVariable( "capture.quality", "Set the quality of the capture. Only applies if using jpeg.", "90", PL_VAR_I32, &captureQuality, nullptr, true );

	PlRegisterConsoleVariable( "renderer.framebufferScale", "Framebuffer resolution multiplier.", "1.0", PL_VAR_F32, &ape_config_.renderer.framebufferScale, nullptr, true );
	PlRegisterConsoleVariable( "renderer.showFps", "Toggle FPS counter.", "false", PL_VAR_BOOL, &ape_config_.renderer.showFps, nullptr, true );
	PlRegisterConsoleVariable( "renderer.wireframe", "Enable wireframe mode.", "0", PL_VAR_BOOL, &ape_config_.renderer.wireframe, nullptr, false );

	//TODO: clamp msaa level - add a callback to check if it's valid before we regen render targets!
	PlRegisterConsoleVariable( "renderer.msaaSamples", "Set number of MSAA samples.", "4", PL_VAR_I32, &ape_config_.renderer.msaaSamples, nullptr, true );

	PlRegisterConsoleVariable( "renderer.showFaceBounds", "Show the bounding volumes for each face.", "0", PL_VAR_BOOL, &ape_config_.renderer.showFaceBounds, nullptr, false );
	PlRegisterConsoleVariable( "renderer.showFaceNormals", "Show normals for each face.", "0", PL_VAR_BOOL, &ape_config_.renderer.showFaceNormals, nullptr, false );
	PlRegisterConsoleVariable( "renderer.skipRoomCull", "Skip room culling; means that rooms are always visible.", "0", PL_VAR_BOOL, &ape_config_.renderer.skipRoomCull, nullptr, false );

	PlRegisterConsoleVariable( "renderer.maxLightDistance", "Maximum distance before lights are culled.", "1024", PL_VAR_F32, &ape_config_.renderer.maxLightDistance, nullptr, true );
	PlRegisterConsoleVariable( "renderer.showLights", "Display a sprite showing where lights are.", "false", PL_VAR_BOOL, &ape_config_.renderer.showLights, nullptr, false );

	PlRegisterConsoleVariable( "renderer.useStencilShadowVolumes", "Use stencil shadow volumes.", "true", PL_VAR_BOOL, &ape_config_.renderer.useStencilShadowVolumes, nullptr, true );
	PlRegisterConsoleVariable( "renderer.showShadowWireframe", "Show the wireframe of the stencil shadow volume.", "false", PL_VAR_BOOL, &ape_config_.renderer.showShadowWireframe, nullptr, false );
	PlRegisterConsoleVariable( "renderer.forceShadows", "Force all lights to emit shadows (not recommended).", "false", PL_VAR_BOOL, &ape_config_.renderer.forceShadows, nullptr, false );

	PlRegisterConsoleVariable( "renderer.showSelectionBuffer", "Show the selection buffer.", "false", PL_VAR_BOOL, &ape_config_.renderer.showSelectionBuffer, nullptr, false );

	//TODO: move these into the material system
	ape_console_var_register( "renderer.fogNearOverride", "Override fog near value.", "-1", PL_VAR_F32, &ape_config_.renderer.fogNearOverride, nullptr, APE_CONSOLE_VAR_FLAG_CHEAT );
	ape_console_var_register( "renderer.fogFarOverride", "Override fog far value.", "-1", PL_VAR_F32, &ape_config_.renderer.fogFarOverride, nullptr, APE_CONSOLE_VAR_FLAG_CHEAT );

	PlRegisterConsoleVariable( "renderer.lightJitterSamples", "Jitter lights to emulate smooth shadows.", "0", PL_VAR_I32, &ape_config_.renderer.lightJitterSamples, nullptr, false );

	//TODO: move into flare code...
	PlRegisterConsoleVariable( "renderer.testFlares", "Test the lens flare effect.", "false", PL_VAR_BOOL, nullptr, nullptr, false );

	PlRegisterConsoleVariable( "renderer.maxPortalDepth", "Maximum depth that portals can recurse.", "1", PL_VAR_I32, nullptr, nullptr, true );
	PlRegisterConsoleVariable( "renderer.showPortalVolumes", "Shows the screen-space volume that's produced from a visible portal.", "false", PL_VAR_BOOL, nullptr, nullptr, false );

	ape_material_register_console_variables_();

	ape_register_shader_console_variables_();
	ape_register_flare_console_variables_();

	// Register variables which we'll use for post-processing. Uh, this also inits... Sorry!
	ape_register_postfx_console_variables_();
}

void ape_renderer_initialize_( void )
{
	ape_console_print_( "Initializing renderer\n" );

	PL_ZERO_( ape_rendererState_ );

	ape_initialize_textures_();
	ape_initialize_render_targets_();
	ape_initialize_shaders_();
	ape_initialize_materials_();
	ape_renderer_batch_initialize_();
	ape_initialize_flares_();

	ape_draw_initialize_debug_mesh_();

	ape_setup_default_draw_state_( nullptr );

	defaultRenderTarget = ape_render_target_create_( "default",
	                                                 800, 600,
	                                                 PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL,
	                                                 PLG_BUFFER_COLOUR,
	                                                 PLG_TEXTURE_FILTER_LINEAR, 0 );
	if ( defaultRenderTarget == NULL )
	{
		ape_console_error_( true, "Failed to create default render target!\n" );
	}

	ape_postfx_setup_();
}

void ape_shutdown_renderer_( void )
{
	ape_postfx_cleanup_();

	ape_draw_destroy_debug_mesh_();

	ape_shutdown_flares_();
	ape_renderer_batch_shutdown_();
	ape_shutdown_materials_();
	ape_shutdown_shaders_();
	ape_shutdown_render_targets_();
}

/****************************************
 ****************************************/

void ape_draw_scene_( ApeCamera *camera, const ApeViewport *viewport )
{
	assert( camera != nullptr && viewport != nullptr );

	COM_PROFILE_FUNCTION_START();

	currentCamera = camera;

	//TODO: this should be during tick, not render!
	ape_camera_build_pvs_( camera, viewport );

	ape_editor_pre_render_scene_( camera );

	if ( ape_config_.renderer.wireframe )
	{
		PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	if ( !ape_config_.world.skipDraw )
	{
		ApeRoom *room = ape_camera_get_room( camera );
		if ( room != NULL )
		{
			COM_PROFILE_START( "ape_room_draw_" );

			ape_room_draw_( camera, &camera->pvs.rooms[ 0 ], viewport );

			COM_PROFILE_END( "ape_room_draw_" );
		}
	}

	if ( ape_config_.renderer.wireframe )
	{
		PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	ape_editor_post_render_scene_();

	PlgBindFrameBuffer( nullptr, PLG_FRAMEBUFFER_DRAW );

	currentCamera = nullptr;

	COM_PROFILE_FUNCTION_END();
}

ApeCamera *ape_renderer_get_current_camera_()
{
	return currentCamera;
}

/////////////////////////////////////////////////////////////////////////////////////
// Utility Methods
/////////////////////////////////////////////////////////////////////////////////////

unsigned int ape_renderer_clip_polygon( const QmMathVector3f *vertices, unsigned int numVertices, const QmMathPlane *plane, QmMathVector3f *dstVertices, unsigned int dstSize )
{
	unsigned int numClippedVertices = 0;

	const QmMathVector3f *prev     = &vertices[ numVertices - 1 ];
	float                 prevDist = qm_math_plane_distance( plane, prev );
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		const QmMathVector3f *cur     = &vertices[ i ];
		float                 curDist = qm_math_plane_distance( plane, cur );
		if ( curDist >= 0.f )
		{
			if ( prevDist < 0.f )
			{
				float t = prevDist / ( prevDist - curDist );

				dstVertices[ numClippedVertices++ ] = qm_math_vector3f(
				        prev->x + t * ( cur->x - prev->x ),
				        prev->y + t * ( cur->y - prev->y ),
				        prev->z + t * ( cur->z - prev->z ) );

				if ( numClippedVertices >= dstSize )
				{
					ape_console_warning_( "Hit max clip limit for polygon!\n" );
					break;
				}
			}

			dstVertices[ numClippedVertices++ ] = *cur;
			if ( numClippedVertices >= dstSize )
			{
				ape_console_warning_( "Hit max clip limit for polygon!\n" );
				break;
			}
		}
		else if ( prevDist >= 0.f )
		{
			float t = prevDist / ( prevDist - curDist );

			dstVertices[ numClippedVertices++ ] = qm_math_vector3f(
			        prev->x + t * ( cur->x - prev->x ),
			        prev->y + t * ( cur->y - prev->y ),
			        prev->z + t * ( cur->z - prev->z ) );

			if ( numClippedVertices >= dstSize )
			{
				ape_console_warning_( "Hit max clip limit for polygon!\n" );
				break;
			}
		}

		prev     = cur;
		prevDist = curDist;
	}

	return numClippedVertices;
}
