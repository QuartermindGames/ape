// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <pthread.h>

#include "ape_private.h"
#include "world/world.h"

#include "renderer.h"
#include "renderer_font.h"
#include "renderer_particle.h"
#include "renderer_render_target.h"

#include "client/ape_client_gui.h"
#include "editor/editor.h"
#include "model/model.h"

#include "post/post.h"

#include "game/game_public.h"

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

static bool                  useCaptureToQoi  = true;
static unsigned int          captureQuality   = 90;
static volatile bool         isCapturing      = false;
static volatile unsigned int numCaptureFrames = 0;

static PLLinkedList *captureQueue = NULL;//CaptureFrame

#define MAX_CAPTURE_THREADS 16
static unsigned int    numCaptureThreads                    = 4;
static pthread_mutex_t captureMutex                         = {};
static pthread_t       captureThread[ MAX_CAPTURE_THREADS ] = {};

static void destroy_capture_frame( void *ptr )
{
	CaptureFrame *frame = ( CaptureFrame * ) ptr;
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

		pthread_mutex_init( &captureMutex, NULL );
		captureQueue = PlCreateLinkedList();

		for ( unsigned int i = 0; i < numCaptureThreads; ++i )
			pthread_create( &captureThread[ i ], NULL, process_capture_queue, NULL );
	}
	else
	{
		for ( unsigned int i = 0; i < numCaptureThreads; ++i )
			pthread_join( captureThread[ i ], NULL );

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
	PLColour clearColour = { 0, 0, 0, 255 };

	ApeWorld *world = ss_game_get_current_world();
	if ( world != NULL && ( viewport->camera == NULL || viewport->camera->mode == APE_CAMERA_MODE_PERSPECTIVE ) )
	{
		clearColour = PlColourF32ToU8( &world->clearColour );
	}

	PlgSetClearColour( clearColour );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgDepthMask( true );

	PlgSetCullMode( PLG_CULL_POSITIVE );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	ApeShaderProgram *program = ape_get_default_shader( APE_SHADER_DEFAULT );
	PlgSetShaderProgram( program->internal );
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
	if ( fboBuffer == NULL )
	{
		return;
	}

	size_t         bufSize = ( ( w * h ) * 4 );
	unsigned char *buf     = PL_NEW_( unsigned char, bufSize );
	if ( PlgReadFrameBufferRegion( NULL, 0, 0, w, h, bufSize, buf ) != NULL )
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
		assert( image != NULL );
		if ( image != NULL )
		{
			PlFlipImageVertical( image );
			PlClearImageAlpha( image );

			PLPath       path;
			unsigned int num = 0;
			PlSetupPath( path, true, "%s/screen%u.png", com_get_app_data_directory(), num );
			while ( PlFileExists( path ) )
			{
				PlSetupPath( path, true, "%s/screen%u.png", com_get_app_data_directory(), ++num );
			}

			PlWriteImage( image, path, 90 );
			PlDestroyImage( image );
		}
		else
		{
			PRINT_WARNING( "Failed to create image for screenshot: %s\n", PlGetError() );
		}
	}
	else
	{
		PRINT_WARNING( "Failed to read framebuffer for screenshot: %s\n", PlGetError() );
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
	PlRegisterConsoleVariable( "capture.numThreads", "Specify the number of threads to use for capturing.", "4", PL_VAR_I32, &numCaptureThreads, NULL, true );
	PlRegisterConsoleVariable( "capture.useQoi", "Capture to qoi format, rather than jpeg, which is faster but less supported.", "true", PL_VAR_BOOL, &useCaptureToQoi, NULL, true );
	PlRegisterConsoleVariable( "capture.quality", "Set the quality of the capture. Only applies if using jpeg.", "90", PL_VAR_I32, &captureQuality, NULL, true );

	PlRegisterConsoleVariable( "renderer.superSampling", "Resolution multiplier.", "1.0", PL_VAR_F32, &ape_config_.renderer.superSampling, NULL, true );
	PlRegisterConsoleVariable( "renderer.showFps", "Toggle FPS counter.", "false", PL_VAR_BOOL, &ape_config_.renderer.showFps, NULL, true );
	PlRegisterConsoleVariable( "renderer.wireframe", "Enable wireframe mode.", "0", PL_VAR_BOOL, &ape_config_.renderer.wireframe, NULL, false );
	PlRegisterConsoleVariable( "r/skipDiffuse", "Skip diffuse map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/skipNormal", "Skip normal map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/skipSpecular", "Skip specular map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "renderer.skipAmbience", "Skip ambient pass.", "0", PL_VAR_BOOL, &ape_config_.renderer.skipAmbience, NULL, false );

	PlRegisterConsoleVariable( "renderer.showFaceBounds", "Show the bounding volumes for each face.", "0", PL_VAR_BOOL, &ape_config_.renderer.showFaceBounds, NULL, false );
	PlRegisterConsoleVariable( "renderer.skipRoomCull", "Skip room culling; means that rooms are always visible.", "0", PL_VAR_BOOL, &ape_config_.renderer.skipRoomCull, NULL, false );

	PlRegisterConsoleVariable( "renderer.maxLightDistance", "Maximum distance before lights are culled.", "1024", PL_VAR_F32, &ape_config_.renderer.maxLightDistance, NULL, true );
	PlRegisterConsoleVariable( "renderer.showLights", "Display a sprite showing where lights are.", "false", PL_VAR_BOOL, &ape_config_.renderer.showLights, NULL, false );

	PlRegisterConsoleVariable( "renderer.useStencilShadowVolumes", "Use stencil shadow volumes.", "true", PL_VAR_BOOL, &ape_config_.renderer.useStencilShadowVolumes, NULL, true );
	PlRegisterConsoleVariable( "renderer.showShadowWireframe", "Show the wireframe of the stencil shadow volume.", "false", PL_VAR_BOOL, &ape_config_.renderer.showShadowWireframe, NULL, false );
	PlRegisterConsoleVariable( "renderer.forceShadows", "Force all lights to emit shadows (not recommended).", "false", PL_VAR_BOOL, &ape_config_.renderer.forceShadows, NULL, false );

	PlRegisterConsoleVariable( "renderer.fogNearOverride", "Override fog near value.", "-1", PL_VAR_F32, &ape_config_.renderer.fogNearOverride, NULL, false );
	PlRegisterConsoleVariable( "renderer.fogFarOverride", "Override fog far value.", "-1", PL_VAR_F32, &ape_config_.renderer.fogFarOverride, NULL, false );

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

	ape_setup_default_draw_state_( NULL );

	defaultRenderTarget = ape_render_target_create( "default",
	                                                800, 600,
	                                                PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL,
	                                                PLG_BUFFER_COLOUR,
	                                                PLG_TEXTURE_FILTER_LINEAR );
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
 * SKY
 * Scrapped and then reintroduced for the
 * ALIVE event... *sigh*
 ****************************************/

typedef struct SkyLayer
{
	ApeMaterial *material;
	float        scale;
	float        alpha;
	float        y;

	PLVector2 offset;
} SkyLayer;

#define MAX_SKY_LAYERS 4
static unsigned int numSkyLayers = 0;
static SkyLayer     skyLayers[ MAX_SKY_LAYERS ];

static void draw_sky_layer( PLGMesh *mesh, ApeMaterial *material, const PLVector3 *location, float x, float y, float scale )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();
	PlTranslateMatrix( *location );

	/* todo: do this in shader... */
	PlgGenerateTextureCoordinates( mesh->vertices, mesh->num_verts, PLVector2( x, y ), PLVector2( scale * 500.0f, scale * 500.0f ) );

	ape_material_draw( material, mesh, NULL, 0 );

	PlPopMatrix();
}

unsigned int ape_sky_add_layer( const char *path, float scale, float y, float alpha )
{
	assert( numSkyLayers < MAX_SKY_LAYERS );
	if ( numSkyLayers >= MAX_SKY_LAYERS )
	{
		PRINT_WARNING( "Hit sky layer limit (%u >= %u)!\n", numSkyLayers, MAX_SKY_LAYERS );
		return ( unsigned int ) -1;
	}

	skyLayers[ numSkyLayers ].material = ape_material_cache( path, APE_CACHE_GROUP_WORLD, true, false );
	skyLayers[ numSkyLayers ].scale    = scale;
	skyLayers[ numSkyLayers ].alpha    = alpha;
	skyLayers[ numSkyLayers ].y        = y;

	numSkyLayers++;
	return ( numSkyLayers - 1 );
}

void ape_sky_set_layer_alpha( unsigned int slot, float alpha )
{
	assert( slot < numSkyLayers );
	if ( slot >= numSkyLayers )
	{
		PRINT_WARNING( "Invalid sky layer slot (%u)!\n", slot );
		return;
	}

	skyLayers[ slot ].alpha = alpha;
}

void ape_sky_set_layer_offset( unsigned int slot, float x, float y )
{
	assert( slot < numSkyLayers );
	if ( slot >= numSkyLayers )
	{
		PRINT_WARNING( "Invalid sky layer slot (%u)!\n", slot );
		return;
	}

	skyLayers[ slot ].offset = ( PLVector2 ){ x, y };
}

void ape_sky_clear_layers( void )
{
	for ( unsigned int i = 0; i < numSkyLayers; ++i )
	{
		if ( skyLayers[ i ].material == NULL )
			continue;

		ape_material_release( skyLayers[ i ].material );
		skyLayers[ i ].material = NULL;
	}

	numSkyLayers = 0;
}

/**
 * Draw scrolling clouds.
 */
void ape_sky_draw_( ApeCamera *camera )
{
	if ( numSkyLayers == 0 )
	{
		return;
	}

	static unsigned int indices[][ 3 ] = {
	        /* corners */
	        { 2, 1, 0 },
	        { 3, 1, 2 },
	        { 4, 3, 2 },
	        { 5, 3, 4 },
	        { 6, 5, 4 },
	        { 7, 5, 6 },
	        { 0, 7, 6 },
	        { 1, 7, 0 },
	        /* middle */
	        { 4, 2, 0 },
	        { 6, 4, 0 },
	};
	unsigned int numTriangles = PL_ARRAY_ELEMENTS( indices );

	static PLGMesh *mesh = NULL;
	if ( mesh == NULL )
	{
		mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, numTriangles, 8 );
		assert( mesh != NULL );
		if ( mesh == NULL )
		{
			PRINT_WARNING( "Failed to create sky mesh: %s\n", PlGetError() );
			return;
		}
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );
	PlgDepthMask( false );

	//TODO: this is all very slow and very gross, but cobbled together to meet a deadline...

	for ( unsigned int i = 0; i < numSkyLayers; ++i )
	{
		PLVector3 location = camera->internal->position;
		location.y += ( skyLayers[ i ].y + 10.0f );

		PlgClearMesh( mesh );

		PlgAddMeshVertex( mesh, &PLVector3( 100.0f, 10.0f, 100.0f ), &pl_vecOrigin3, &( PLColour ){ 255, 255, 255, PlFloatToByte( skyLayers[ i ].alpha ) }, &pl_vecOrigin2 );   /* top right */
		PlgAddMeshVertex( mesh, &PLVector3( 200.0f, 10.0f, 200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );                                                         /* top right far */
		PlgAddMeshVertex( mesh, &PLVector3( 100.0f, 10.0f, -100.0f ), &pl_vecOrigin3, &( PLColour ){ 255, 255, 255, PlFloatToByte( skyLayers[ i ].alpha ) }, &pl_vecOrigin2 );  /* lower right */
		PlgAddMeshVertex( mesh, &PLVector3( 200.0f, 10.0f, -200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );                                                        /* lower right far */
		PlgAddMeshVertex( mesh, &PLVector3( -100.0f, 10.0f, -100.0f ), &pl_vecOrigin3, &( PLColour ){ 255, 255, 255, PlFloatToByte( skyLayers[ i ].alpha ) }, &pl_vecOrigin2 ); /* lower left */
		PlgAddMeshVertex( mesh, &PLVector3( -200.0f, 10.0f, -200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );                                                       /* lower left far */
		PlgAddMeshVertex( mesh, &PLVector3( -100.0f, 10.0f, 100.0f ), &pl_vecOrigin3, &( PLColour ){ 255, 255, 255, PlFloatToByte( skyLayers[ i ].alpha ) }, &pl_vecOrigin2 );  /* top left */
		PlgAddMeshVertex( mesh, &PLVector3( -200.0f, 10.0f, 200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );                                                        /* top left far */

		for ( unsigned int j = 0; j < numTriangles; ++j )
		{
			PlgAddMeshTriangle( mesh, indices[ j ][ 0 ], indices[ j ][ 1 ], indices[ j ][ 2 ] );
		}

		draw_sky_layer( mesh, skyLayers[ i ].material, &location, skyLayers[ i ].offset.x, skyLayers[ i ].offset.y, skyLayers[ i ].scale );
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgDepthMask( true );
}

/****************************************
 ****************************************/

static void render_transparent_world( ApeWorld *world, ApeCamera *camera )
{
	unsigned int numLights;
	ApeLight   **lights = ape_camera_get_visible_lights_( camera, &numLights );
	for ( unsigned int i = 0; i < numLights; ++i )
	{
		if ( lights[ i ]->colour.a == 0.0f )
		{
			continue;
		}

		//TODO: viewport clipping per light volume

		ape_rendererState_.overrideBlendMode = true;
		ape_rendererState_.blendModeA        = PLG_BLEND_ONE;
		ape_rendererState_.blendModeB        = PLG_BLEND_ONE;

		ape_world_draw( world, camera, lights[ i ], false, true );

		ape_rendererState_.overrideBlendMode = false;
		ape_rendererState_.passStage         = SS_ARL_RENDERER_PASS_DEFAULT;
	}
}

static void render_solid_world( ApeWorld *world, ApeCamera *camera )
{
	// Ambient pass ...

	ape_world_draw( world, camera, NULL, true, false );

	PlgDepthMask( false );

	unsigned int numLights;
	ApeLight   **lights = ape_camera_get_visible_lights_( camera, &numLights );

	for ( unsigned int i = 0; i < numLights; ++i )
	{
		PlgClearBuffers( PLG_BUFFER_STENCIL );

		if ( lights[ i ]->colour.a == 0.0f )
		{
			continue;
		}

		//TODO: viewport clipping per light volume

		if ( ape_config_.renderer.showLights )
		{
			arl_draw_axis_pivot( lights[ i ]->base.position, lights[ i ]->base.angles, 1.0f );
		}

		bool drawShadows = ape_config_.renderer.useStencilShadowVolumes && ( ape_light_get_shadow_type( lights[ i ] ) == SS_APE_LIGHT_SHADOW_TYPE_DYNAMIC );
		if ( drawShadows )
		{
			if ( ape_config_.renderer.showShadowWireframe )
			{
				ape_rendererState_.cullMode  = SS_ARL_CULL_MODE_NONE;
				ape_rendererState_.passStage = SS_ARL_RENDERER_PASS_DEFAULT;

				PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );

				ape_world_draw_stencil_shadows( world, camera, lights[ i ] );

				PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );

				ape_rendererState_.cullMode = SS_ARL_CULL_MODE_DEFAULT;
			}

			PlgEnableGraphicsState( PLG_GFX_STATE_STENCILTEST );
			PlgEnableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
			PlgColourMask( false, false, false, false );

			PlgStencilBufferFunction( PLG_COMPARE_ALWAYS, 0x0, 0xFF );
			PlgStencilOp( PLG_STENCIL_FACE_FRONT, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_INCRWRAP, PLG_STENCIL_OP_KEEP );
			PlgStencilOp( PLG_STENCIL_FACE_BACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_DECRWRAP, PLG_STENCIL_OP_KEEP );

			ape_rendererState_.cullMode = SS_ARL_CULL_MODE_NONE;
			ape_world_draw_stencil_shadows( world, camera, lights[ i ] );
			ape_rendererState_.cullMode = SS_ARL_CULL_MODE_DEFAULT;

			PlgDisableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
			PlgColourMask( true, true, true, true );

			PlgDepthBufferFunction( PLG_COMPARE_LEQUAL );
			PlgStencilBufferFunction( PLG_COMPARE_EQUAL, 0x0, 0xFF );
			PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP );
		}

		ape_rendererState_.overrideBlendMode = true;
		ape_rendererState_.blendModeA        = PLG_BLEND_ONE;
		ape_rendererState_.blendModeB        = PLG_BLEND_ONE;

		ape_world_draw( world, camera, lights[ i ], false, false );

		ape_rendererState_.overrideBlendMode = false;
		ape_rendererState_.passStage         = SS_ARL_RENDERER_PASS_DEFAULT;

		if ( drawShadows )
		{
			PlgDisableGraphicsState( PLG_GFX_STATE_STENCILTEST );
		}

		ape_rendererPerformance_.numLights++;
	}

	PlgDepthMask( true );
}

PLVector2   screenPosTest;
static void render_scene( ApeCamera *camera, const ApeViewport *viewport )
{
	ape_rendererPerformance_.numLights = 0;

	ApeWorld *world = ape_camera_get_world( camera );
	ape_editor_pre_render_scene_( camera );

	if ( !ape_config_.world.skipDraw )
	{
		ape_sky_draw_( camera );
		if ( world != NULL )
		{
			switch ( camera->drawMode )
			{
				default:
					break;
				case APE_CAMERA_DRAW_MODE_WIREFRAME:
					ape_world_draw_wireframe( world, camera );
					break;
				case APE_CAMERA_DRAW_MODE_SOLID:
				case APE_CAMERA_DRAW_MODE_TEXTURED:
					ape_world_draw( world, camera, NULL, true, false );
					ape_world_draw( world, camera, NULL, true, true );
					break;
				case APE_CAMERA_DRAW_MODE_SHADED:
					render_solid_world( world, camera );
					render_transparent_world( world, camera );
					break;
			}
		}
	}

	PlgDepthBufferFunction( PLG_COMPARE_LESS );

	// for now, shove this here, but really it should be accounting for the room transform... :(
	ape_draw_debug_mesh_display_();

	ape_rendererState_.passStage = SS_ARL_RENDERER_PASS_DEFAULT;
}

void ape_draw_scene_( ApeCamera *camera, const ApeViewport *viewport )
{
	assert( camera != NULL );
	assert( viewport != NULL );

	COM_PROFILE_FUNCTION_START();

	currentCamera = camera;

	PlgDepthMask( true );
	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL );

	ape_grid_draw_( camera );

	if ( camera->drawMode == APE_CAMERA_DRAW_MODE_WIREFRAME || ape_config_.renderer.wireframe )
	{
		PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	render_scene( camera, viewport );

	if ( camera->drawMode == APE_CAMERA_DRAW_MODE_WIREFRAME || ape_config_.renderer.wireframe )
	{
		PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );

	currentCamera = nullptr;

	COM_PROFILE_FUNCTION_END();
}

ApeCamera *ape_renderer_get_current_camera_()
{
	return currentCamera;
}
