// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <pthread.h>

#include "ape_private.h"
#include "world/world.h"

#include "renderer.h"
#include "renderer_font.h"

#include "editor/editor.h"

#include "post/post.h"

ApeRendererStats     ape_rendererPerformance_ = {};
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
	PL_DELETE( frame->buf );
	PL_DELETE( frame );
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

			usleep( 1000 );
			continue;
		}

		CaptureFrame *frame = PlGetLinkedListNodeUserData( node );
		PlDestroyLinkedListNode( node );

		unsigned int frameNum = numCaptureFrames++;

		pthread_mutex_unlock( &captureMutex );

		PLImage *image = PlCreateImage( frame->buf, frame->w, frame->h, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );
		assert( image != NULL );
		if ( image != NULL )
		{
			PlFlipImageVertical( image );
			PlClearImageAlpha( image );

			PLPath path;
			PlSetupPath( path, true, "%s/captures/%u.%s", com_get_app_data_directory(), frameNum, useCaptureToQoi ? "qoi" : "jpg" );
			PlWriteImage( image, path, captureQuality );

			PlDestroyImage( image );
		}

		destroy_capture_frame( frame );
	}

	return NULL;
}

static void capture_command( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
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
	PlgSetClearColour( PL_COLOURU8( 0, 0, 0, 255 ) );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgDepthMask( true );

	PlgSetCullMode( PLG_CULL_POSITIVE );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	ApeShaderProgram *program = ape_get_default_shader( APE_SHADER_DEFAULT );
	PlgSetShaderProgram( program->internal );

	//PlMatrixMode( PL_MODELVIEW_MATRIX );
}

void ape_draw_begin_( ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	double newTime = PlGetCurrentSeconds();

	viewport->perf.frameReadings[ viewport->perf.frameIndex++ ] = 1.0 / ( newTime - viewport->perf.oldTime );
	if ( viewport->perf.frameIndex >= APE_MAX_FPS_READINGS )
	{
		viewport->perf.frameIndex = 0;
	}

	viewport->perf.oldTime = newTime;

	ape_viewport_make_active( viewport );

	ApeRenderTarget *target = ape_viewport_get_render_target( viewport );
	ape_render_target_bind( target, PLG_FRAMEBUFFER_DEFAULT );

	ape_setup_default_draw_state_( viewport );

	COM_PROFILE_FUNCTION_END();
}

static void write_screenshot( void )
{
	ApeRenderTarget *renderTarget = ape_postfx_get_render_target();
	if ( renderTarget == NULL )
	{
		return;
	}

	unsigned int w, h;
	ape_render_target_get_size( renderTarget, &w, &h );

	PLGFrameBuffer *fboBuffer = ape_render_target_get_frame_buffer( renderTarget );
	assert( fboBuffer != NULL );

	size_t         bufSize = ( ( w * h ) * 4 );
	unsigned char *buf     = PL_NEW_( unsigned char, bufSize );
	if ( PlgReadFrameBufferRegion( nullptr, 0, 0, w, h, bufSize, buf ) != NULL )
	{
		if ( isCapturing )
		{
			pthread_mutex_lock( &captureMutex );

			CaptureFrame *frame = PL_NEW( CaptureFrame );
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
			ape_warning_( "Failed to create image for screenshot: %s\n", PlGetError() );
		}
	}
	else
	{
		ape_warning_( "Failed to read framebuffer for screenshot: %s\n", PlGetError() );
	}

	PL_DELETE( buf );
}

void ape_draw_end_( ApeViewport *viewport )
{
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

void ape_prepare_screenshot_capture_( void )
{
	if ( isCapturing )
	{
		return;
	}

	isScreenshotPending = true;
}

static void prepare_screenshot_capture_command( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
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
	//TODO: move these into the material system
	PlRegisterConsoleVariable( "r/skipDiffuse", "Skip diffuse map.", "0", PL_VAR_BOOL, nullptr, nullptr, false );
	PlRegisterConsoleVariable( "r/skipNormal", "Skip normal map.", "0", PL_VAR_BOOL, nullptr, nullptr, false );
	PlRegisterConsoleVariable( "r/skipSpecular", "Skip specular map.", "0", PL_VAR_BOOL, nullptr, nullptr, false );
	PlRegisterConsoleVariable( "renderer.skipAmbience", "Skip ambient pass.", "0", PL_VAR_BOOL, &ape_config_.renderer.skipAmbience, nullptr, false );
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
	PlRegisterConsoleVariable( "renderer.fogNearOverride", "Override fog near value.", "-1", PL_VAR_F32, &ape_config_.renderer.fogNearOverride, nullptr, false );
	PlRegisterConsoleVariable( "renderer.fogFarOverride", "Override fog far value.", "-1", PL_VAR_F32, &ape_config_.renderer.fogFarOverride, nullptr, false );

	PlRegisterConsoleVariable( "renderer.lightJitterSamples", "Jitter lights to emulate smooth shadows.", "0", PL_VAR_I32, &ape_config_.renderer.lightJitterSamples, nullptr, false );

	//TODO: move into flare code...
	PlRegisterConsoleVariable( "renderer.testFlares", "Test the lens flare effect.", "false", PL_VAR_BOOL, nullptr, nullptr, false );

	ape_register_shader_console_variables_();
	ape_register_flare_console_variables_();

	// Register variables which we'll use for post-processing. Uh, this also inits... Sorry!
	ape_register_postfx_console_variables_();
}

void ape_initialize_renderer_( void )
{
	PRINT( "Initializing renderer\n" );

	PL_ZERO_( ape_rendererState_ );

	ape_initialize_textures_();
	ape_initialize_render_targets_();
	ape_initialize_shaders_();
	ape_initialize_materials_();
	ape_initialize_bitmap_fonts_();
	ape_initialize_flares_();

	ape_draw_initialize_debug_mesh_();

	ape_setup_default_draw_state_( nullptr );

	defaultRenderTarget = ape_render_target_create( "default",
	                                                800, 600,
	                                                PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL,
	                                                PLG_BUFFER_COLOUR,
	                                                PLG_TEXTURE_FILTER_LINEAR, 0 );
	if ( defaultRenderTarget == NULL )
	{
		ape_error_( true, "Failed to create default render target!\n" );
	}

	ape_postfx_setup_();
}

void ape_shutdown_renderer_( void )
{
	ape_postfx_cleanup_();

	ape_draw_destroy_debug_mesh_();

	ape_shutdown_flares_();
	ape_shutdown_bitmap_fonts_();
	ape_shutdown_materials_();
	ape_shutdown_shaders_();
	ape_shutdown_render_targets_();
}

/****************************************
 ****************************************/

void ape_draw_scene_( ApeCamera *camera, const ApeViewport *viewport )
{
	assert( camera != NULL );
	assert( viewport != NULL );

	COM_PROFILE_FUNCTION_START();

	currentCamera = camera;

	ape_camera_get_visible_lights_( camera, &ape_rendererPerformance_.numLights );

	PlgDepthMask( true );
	PlgSetClearColour( viewport->clearColour );
	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL );

	ape_editor_pre_render_scene_( camera );

	if ( ape_config_.renderer.wireframe )
	{
		PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	if ( !ape_config_.world.skipDraw )
	{
		PlgDepthBufferFunction( PLG_COMPARE_LEQUAL );

		ApeRoom *room = ape_camera_get_room( camera );
		if ( room != NULL )
		{
			ape_room_draw_( room, camera, viewport );
		}

		PlgDepthBufferFunction( PLG_COMPARE_LESS );
	}

	if ( ape_config_.renderer.wireframe )
	{
		PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	ape_editor_post_render_scene_();

	ape_draw_debug_mesh_display_();

	PlgBindFrameBuffer( nullptr, PLG_FRAMEBUFFER_DRAW );

	currentCamera = nullptr;

	COM_PROFILE_FUNCTION_END();
}

ApeCamera *ape_renderer_get_current_camera_()
{
	return currentCamera;
}
