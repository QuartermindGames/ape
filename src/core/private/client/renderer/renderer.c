/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plgraphics/plg_driver_interface.h>

#include <pthread.h>
#include <unistd.h>

#include "ape_private.h"
#include "legacy/actor.h"
#include "world/world.h"

#include "renderer.h"
#include "renderer_font.h"
#include "renderer_particle.h"
#include "renderer_render_target.h"

#include "client/ape_client_gui.h"
#include "editor/editor.h"

#include "post/post.h"

#include "game/game_interface.h"

ApeRendererStats ape_rendererPerformance_;
SSArlRendererPassState arl_rendererState_;

static SSArlRenderTarget *defaultRenderTarget;

static bool isScreenshotPending = false;

/////////////////////////////////////////////////////////////////////////////////////
// Frame Capture

typedef struct CaptureFrame
{
	unsigned int w;
	unsigned int h;
	unsigned char *buf;
} CaptureFrame;

static bool useCaptureToQoi = true;
static unsigned int captureQuality = 90;
static volatile bool isCapturing = false;
static volatile unsigned int numCaptureFrames = 0;

static PLLinkedList *captureQueue = NULL;//CaptureFrame

#define MAX_CAPTURE_THREADS 16
static unsigned int numCaptureThreads = 4;
static pthread_mutex_t captureMutex = {};
static pthread_t captureThread[ MAX_CAPTURE_THREADS ] = {};

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
			PlSetupPath( path, true, "%s/captures/%u.%s", ss_com_get_app_data_directory(), frameNum, useCaptureToQoi ? "qoi" : "jpg" );
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
	isCapturing = !isCapturing;
	if ( isCapturing )
	{
		// Create a folder to store the captures if one doesn't already exist
		PLPath captureDirectory;
		PlSetupPath( captureDirectory, true, "%s/captures/", ss_com_get_app_data_directory() );
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

bool ss_arl_get_capture_state_( void )
{
	return isCapturing;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/**********************************************************/

void ss_arl_setup_default_state( const SSArlViewport *viewport )
{
	PLColour clearColour = { 50, 50, 50, 255 };

	ApeWorld *world = ss_game_get_current_world();
	if ( world != NULL && ( viewport->camera == NULL || viewport->camera->mode == SS_ARL_CAMERA_MODE_PERSPECTIVE ) )
		clearColour = PlColourF32ToU8( &world->clearColour );

	PlgSetClearColour( clearColour );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgDepthMask( true );

	PlgSetCullMode( PLG_CULL_POSITIVE );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );
}

void ss_arl_draw_begin_( SSArlViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	double newTime = PlGetCurrentSeconds();

	viewport->perf.frameReadings[ viewport->perf.frameIndex++ ] = 1.0 / ( newTime - viewport->perf.oldTime );
	if ( viewport->perf.frameIndex >= APE_MAX_FPS_READINGS )
		viewport->perf.frameIndex = 0;

	viewport->perf.oldTime = newTime;

	ss_arl_setup_default_state( viewport );
	ss_arl_viewport_make_active( viewport );

	PlgClearBuffers( PLG_BUFFER_DEPTH | PLG_BUFFER_COLOUR );

	COM_PROFILE_FUNCTION_END();
}

static void write_screenshot( void )
{
	SSArlRenderTarget *renderTarget = ss_arl_postfx_get_render_target();
	if ( renderTarget == NULL )
		return;

	unsigned int w, h;
	ss_arl_render_target_get_size( renderTarget, &w, &h );

	PLGFrameBuffer *fboBuffer = ss_arl_render_target_get_frame_buffer( renderTarget );
	assert( fboBuffer != NULL );
	if ( fboBuffer == NULL )
		return;

	size_t bufSize = ( ( w * h ) * 4 );
	unsigned char *buf = PL_NEW_( unsigned char, bufSize );
	if ( PlgReadFrameBufferRegion( fboBuffer, 0, 0, w, h, bufSize, buf ) != NULL )
	{
		if ( isCapturing )
		{
			pthread_mutex_lock( &captureMutex );

			CaptureFrame *frame = PL_NEW( CaptureFrame );
			PlInsertLinkedListNode( captureQueue, frame );
			frame->buf = buf;
			frame->w = w;
			frame->h = h;

			pthread_mutex_unlock( &captureMutex );
			return;
		}

		PLImage *image = PlCreateImage( buf, w, h, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );
		assert( image != NULL );
		if ( image != NULL )
		{
			PlFlipImageVertical( image );
			PlClearImageAlpha( image );

			PLPath path;
			unsigned int num = 0;
			PlSetupPath( path, true, "%s/screen%u.png", ss_com_get_app_data_directory(), num );
			while ( PlFileExists( path ) )
				PlSetupPath( path, true, "%s/screen%u.png", ss_com_get_app_data_directory(), ++num );

			PlWriteImage( image, path, 90 );
			PlDestroyImage( image );
		}
		else
			PRINT_WARNING( "Failed to create image for screenshot: %s\n", PlGetError() );
	}
	else
		PRINT_WARNING( "Failed to read framebuffer for screenshot: %s\n", PlGetError() );
}

void ss_arl_draw_end_( SSArlViewport *viewport )
{
	PL_ZERO_( ape_rendererPerformance_ );

	viewport->perf.numBatches = 0;
	viewport->perf.numTriangles = 0;
	viewport->perf.numPolygons = 0;
	viewport->perf.numPortals = 0;

	if ( isScreenshotPending || isCapturing )
	{
		write_screenshot();
		isScreenshotPending = false;
	}
}

void ss_arl_initialize_shaders_( void );  /* renderer/shaders.c */
void ss_arl_initialize_textures_( void ); /* texture.c */

/* renderer_rendertarget.c */
void ss_arl_initialize_render_targets_( void );
void ss_arl_shutdown_render_targets_( void );

static void prepare_screenshot_capture( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	if ( isCapturing )
		return;

	isScreenshotPending = true;
}

void apeRegisterRendererConsoleVariables_( void )
{
	PlRegisterConsoleCommand( "screenshot", "Take a screenshot.", 0, prepare_screenshot_capture );

	PlRegisterConsoleCommand( "capture", "Capture frames continuously until called again.", 0, capture_command );
	PlRegisterConsoleVariable( "capture_threads", "Specify the number of threads to use for capturing.", "4", PL_VAR_I32, &numCaptureThreads, NULL, true );
	PlRegisterConsoleVariable( "capture_qoi", "Capture to qoi format, rather than jpeg, which is faster but less supported.", "true", PL_VAR_BOOL, &useCaptureToQoi, NULL, true );
	PlRegisterConsoleVariable( "capture_quality", "Set the quality of the capture. Only applies if using jpeg.", "90", PL_VAR_I32, &captureQuality, NULL, true );

	PlRegisterConsoleVariable( "r/superSampling", "Resolution multiplier.", "1.0", PL_VAR_F32, &ape_config_.renderer.superSampling, NULL, true );
	PlRegisterConsoleVariable( "fps", "Toggle FPS counter.",
#if !defined( NDEBUG )
	                           "true",
#else
	                           "false",
#endif
	                           PL_VAR_BOOL, &ape_config_.renderer.showFps, NULL, true );
	PlRegisterConsoleVariable( "wireframe", "Enable wireframe mode.", "0", PL_VAR_BOOL, &ape_config_.renderer.wireframe, NULL, false );
	PlRegisterConsoleVariable( "r/skipDiffuse", "Skip diffuse map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/skipNormal", "Skip normal map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/skipSpecular", "Skip specular map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "skip_ambience", "Skip ambient pass.", "0", PL_VAR_BOOL, &ape_config_.renderer.skipAmbience, NULL, false );
	PlRegisterConsoleVariable( "ape/r/useStencilShadowVolumes", "Use stencil shadow volumes.", "true", PL_VAR_BOOL, &ape_config_.renderer.useStencilShadowVolumes, NULL, true );
	PlRegisterConsoleVariable( "ape/r/showShadowWireframe", "Show the wireframe of the stencil shadow volume.", "false", PL_VAR_BOOL, &ape_config_.renderer.showShadowWireframe, NULL, false );
	PlRegisterConsoleVariable( "ape/r/maxLightDistance", "Maximum distance before lights are culled.", "1024", PL_VAR_F32, &ape_config_.renderer.maxLightDistance, NULL, true );
	PlRegisterConsoleVariable( "show_face_bounds", "Show the bounding volumes for each face.", "0", PL_VAR_BOOL, &ape_config_.renderer.showFaceBounds, NULL, false );
	PlRegisterConsoleVariable( "skip_room_cull", "Skip room culling; means that rooms are always visible.", "0", PL_VAR_BOOL, &ape_config_.renderer.skipRoomCull, NULL, false );
	PlRegisterConsoleVariable( "show_lights", "Display a sprite showing where lights are.", "false", PL_VAR_BOOL, &ape_config_.renderer.showLights, NULL, false );

	// Camera
	PlRegisterConsoleVariable( "r/fov", "", "75", PL_VAR_F32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/near", "", "0.1", PL_VAR_F32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/far", "", "50.0", PL_VAR_F32, NULL, NULL, true );

	PlRegisterConsoleVariable( "ape/r/fogNear", "Fog near value.", "-1", PL_VAR_F32, NULL, NULL, false );
	PlRegisterConsoleVariable( "ape/r/fogFar", "Fog far value.", "-1", PL_VAR_F32, NULL, NULL, false );

	// Register variables which we'll use for post-processing. Uh, this also inits... Sorry!
	ss_arl_postfx_register_console_variables_();
}

void ss_arl_initialize_( void )
{
	PRINT( "Initializing renderer\n" );

	PL_ZERO_( arl_rendererState_ );

	ss_arl_initialize_textures_();
	ss_arl_initialize_render_targets_();
	ss_arl_initialize_shaders_();
	ss_arl_initialize_materials_();
	ss_arl_initialize_bitmap_fonts_();

	ss_arl_setup_default_state( NULL );

	defaultRenderTarget = ss_arl_render_target_create( "default",
	                                                   800, 600,
	                                                   PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL,
	                                                   PLG_BUFFER_COLOUR,
	                                                   PLG_TEXTURE_FILTER_LINEAR );
	if ( defaultRenderTarget == NULL )
		PRINT_ERROR( "Failed to create default render target!\n" );

	ss_arl_postfx_setup_();
}

void ss_arl_shutdown_( void )
{
	ss_arl_postfx_cleanup_();

	ss_arl_shutdown_bitmap_fonts_();
	ss_arl_shutdown_materials_();
	ss_arl_shutdown_render_targets_();
}

/****************************************
 * SKY
 * Scrapped and then reintroduced for the
 * ALIVE event... *sigh*
 ****************************************/

typedef struct SkyLayer
{
	ApeMaterial *material;
	float scale;
	float alpha;
	float y;

	PLVector2 offset;
} SkyLayer;

#define MAX_SKY_LAYERS 4
static unsigned int numSkyLayers = 0;
static SkyLayer skyLayers[ MAX_SKY_LAYERS ];

static void draw_sky_layer( PLGMesh *mesh, ApeMaterial *material, const PLVector3 *location, float x, float y, float scale )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlTranslateMatrix( *location );

	/* todo: do this in shader... */
	PlgGenerateTextureCoordinates( mesh->vertices, mesh->num_verts, PLVector2( x, y ), PLVector2( scale, scale ) );
	PlgUploadMesh( mesh );

	ss_arl_material_draw( material, mesh, NULL, 0 );

	PlPopMatrix();
}

unsigned int ss_arl_sky_add_layer( const char *path, float scale, float y, float alpha )
{
	assert( numSkyLayers < MAX_SKY_LAYERS );
	if ( numSkyLayers >= MAX_SKY_LAYERS )
	{
		PRINT_WARNING( "Hit sky layer limit (%u >= %u)!\n", numSkyLayers, MAX_SKY_LAYERS );
		return ( unsigned int ) -1;
	}

	skyLayers[ numSkyLayers ].material = ss_arl_material_cache( path, APE_CACHE_WORLD, true, false );
	skyLayers[ numSkyLayers ].scale = scale;
	skyLayers[ numSkyLayers ].alpha = alpha;
	skyLayers[ numSkyLayers ].y = y;

	numSkyLayers++;
	return ( numSkyLayers - 1 );
}

void ss_arl_sky_set_layer_alpha( unsigned int slot, float alpha )
{
	assert( slot < numSkyLayers );
	if ( slot >= numSkyLayers )
	{
		PRINT_WARNING( "Invalid sky layer slot (%u)!\n", slot );
		return;
	}

	skyLayers[ slot ].alpha = alpha;
}

void ss_arl_sky_set_layer_offset( unsigned int slot, float x, float y )
{
	assert( slot < numSkyLayers );
	if ( slot >= numSkyLayers )
	{
		PRINT_WARNING( "Invalid sky layer slot (%u)!\n", slot );
		return;
	}

	skyLayers[ slot ].offset = ( PLVector2 ){ x, y };
}

void arl_sky_clear_layers( void )
{
	for ( unsigned int i = 0; i < numSkyLayers; ++i )
	{
		if ( skyLayers[ i ].material == NULL )
			continue;

		ss_arl_material_release( skyLayers[ i ].material );
		skyLayers[ i ].material = NULL;
	}

	numSkyLayers = 0;
}

/**
 * Draw scrolling clouds.
 */
void arl_sky_draw( SSArlCamera *camera )
{
	if ( numSkyLayers == 0 )
		return;

	static unsigned int indices[][ 3 ] = {
  /* corners */
	        {2,  1, 0},
	        { 3, 1, 2},
	        { 4, 3, 2},
	        { 5, 3, 4},
	        { 6, 5, 4},
	        { 7, 5, 6},
	        { 0, 7, 6},
	        { 1, 7, 0},
 /* middle */
	        { 4, 2, 0},
	        { 6, 4, 0},
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

	PLVector3 location;
	location = camera->internal->position;
	location.y += 10.0f;

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );
	PlgDepthMask( false );

	//TODO: this is all very slow and very gross, but cobbled together to meet a deadline...

	double ticks = ss_acl_get_num_ticks();
	for ( unsigned int i = 0; i < numSkyLayers; ++i )
	{
		location.y = skyLayers[ i ].y;

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
			PlgAddMeshTriangle( mesh, indices[ j ][ 0 ], indices[ j ][ 1 ], indices[ j ][ 2 ] );

		draw_sky_layer( mesh, skyLayers[ i ].material, &location, skyLayers[ i ].offset.x, skyLayers[ i ].offset.y, skyLayers[ i ].scale );
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgDepthMask( true );
}

/****************************************
 ****************************************/

PLVector2 screenPosTest;
static void render_scene( SSArlCamera *camera, const SSArlViewport *viewport )
{
	ape_rendererPerformance_.numLights = 0;

	ApeWorld *world = ss_game_get_current_world();
	if ( world == NULL )
		return;

	// Ambient pass ...

	//PlgDepthBufferFunction( PLG_COMPARE_LEQUAL );

	PlgDepthMask( true );

	arl_sky_draw( camera );
	arl_level_draw( world, camera, NULL, true );

	PlgDepthMask( false );

	unsigned int numLights;
	SSArlLight **lights = ss_acl_camera_get_visible_lights_( camera, &numLights );

	for ( unsigned int i = 0; i < numLights; ++i )
	{
		if ( lights[ i ]->colour.a == 0.0f )
			continue;

#if 0

			PLCollisionSphere sphere = PlSetupCollisionSphere( lights[ i ]->position, lights[ i ]->radius );
			PLMatrix4 viewProj = PlMultiplyMatrix4( camera->internal->internal.proj, &camera->internal->internal.view );
			PLVector2 lightScreenPos = PlConvertWorldToScreen( &lights[ i ]->position, &viewProj, viewport->width, viewport->height, viewport->x, viewport->y, false );
			PLVector3 lightRadi = PlAddVector3F( lights[ i ]->position, lights[ i ]->radius );
			PLVector2 lightRadiusPos = PlConvertWorldToScreen( &lightRadi, &viewProj, viewport->width, viewport->height, viewport->x, viewport->y, false );

			static int radius = 256;

			PLRectangleI32 screenRect;
			screenRect.x = ( int ) lightScreenPos.x - ( radius / 2 );
			if ( screenRect.x < viewport->x ) screenRect.x = viewport->x;
			screenRect.y = ( int ) lightScreenPos.y - ( radius / 2 );
			if ( screenRect.y < viewport->y ) screenRect.y = viewport->y;

			screenRect.w = ( int ) radius;
			//if ( screenRect.x + screenRect.w > viewport->width ) screenRect.w = ( screenRect.x + screenRect.w ) - viewport->width;
			screenRect.h = ( int ) radius;
			//if ( screenRect.y + screenRect.h > viewport->height ) screenRect.h = ( screenRect.y + screenRect.h ) - viewport->height;

			//apeDrawAxesPivot( lights[ i ]->position, lights[ i ]->angles, 1.0f );

			if ( screenRect.w < 0 || screenRect.h < 0 )
				continue;

			//PlgClipViewport( screenRect.x, screenRect.y, screenRect.w, screenRect.h );

#endif

		//arl_draw_axis_pivot( lights[ i ]->position, lights[ i ]->angles, 1.0f );

		if ( lights[ i ]->flags & SS_ARL_LIGHT_FLAG_RUNTIME_SHADOWS )
		{
			if ( ape_config_.renderer.showShadowWireframe )
			{
				arl_rendererState_.cullMode = SS_ARL_CULL_MODE_NONE;
				arl_rendererState_.passStage = SS_ARL_RENDERER_PASS_DEFAULT;

				PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
				arl_level_draw_stencil_shadows( world, camera, lights[ i ] );
				PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );

				arl_rendererState_.cullMode = SS_ARL_CULL_MODE_DEFAULT;
			}

			PlgEnableGraphicsState( PLG_GFX_STATE_STENCILTEST );
			PlgEnableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
			PlgColourMask( false, false, false, false );

			PlgStencilBufferFunction( PLG_COMPARE_ALWAYS, 0x0, 0xFF );
			PlgStencilOp( PLG_STENCIL_FACE_FRONT, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_INCRWRAP, PLG_STENCIL_OP_KEEP );
			PlgStencilOp( PLG_STENCIL_FACE_BACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_DECRWRAP, PLG_STENCIL_OP_KEEP );

			arl_rendererState_.cullMode = SS_ARL_CULL_MODE_NONE;
			arl_level_draw_stencil_shadows( world, camera, lights[ i ] );
			arl_rendererState_.cullMode = SS_ARL_CULL_MODE_DEFAULT;

			PlgDisableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
			PlgColourMask( true, true, true, true );

			//PlgDepthBufferFunction( PLG_COMPARE_LEQUAL );
			PlgStencilBufferFunction( PLG_COMPARE_EQUAL, 0x0, 0xFF );
			PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP );

			arl_rendererState_.overrideBlendMode = true;
			arl_rendererState_.blendModeA = PLG_BLEND_ONE;
			arl_rendererState_.blendModeB = PLG_BLEND_ONE;

			arl_level_draw( world, camera, lights[ i ], false );

			arl_rendererState_.overrideBlendMode = false;

			PlgDisableGraphicsState( PLG_GFX_STATE_STENCILTEST );
		}
		else
		{
			arl_rendererState_.overrideBlendMode = true;
			arl_rendererState_.blendModeA = PLG_BLEND_ONE;
			arl_rendererState_.blendModeB = PLG_BLEND_ONE;

			arl_level_draw( world, camera, lights[ i ], false );

			arl_rendererState_.overrideBlendMode = false;
			arl_rendererState_.passStage = SS_ARL_RENDERER_PASS_DEFAULT;
		}

		ape_rendererPerformance_.numLights++;
	}

	PlgDepthMask( true );
	PlgDepthBufferFunction( PLG_COMPARE_LEQUAL );

	PlgClipViewport( viewport->x, viewport->y, viewport->width, viewport->height );
	arl_rendererState_.passStage = SS_ARL_RENDERER_PASS_DEFAULT;
}

#if 0
ApeEditorContext *editorInstance = apeGetCurrentEditorContext();
			if ( editorInstance != NULL && editorInstance->gridScale > 0 )
			{
				PlMatrixMode( PL_MODELVIEW_MATRIX );
				PlPushMatrix();

				PLVector3 angles;
				angles.x = PL_DEG2RAD( 90.0f );
				angles.y = PL_DEG2RAD( 0.0f );
				angles.z = PL_DEG2RAD( 0.0f );

				PlRotateMatrix( angles.x, 1.0f, 0.0f, 0.0f );
				PlRotateMatrix( angles.y, 0.0f, 1.0f, 0.0f );
				PlRotateMatrix( angles.z, 0.0f, 0.0f, 1.0f );

				PlTranslateMatrix( PLVector3( 0, -5, 0 ) );

				static const unsigned int gridW = 256;

				PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
				PlgDrawDottedGrid( -( gridW / 2 ), -( gridW / 2 ), gridW, gridW, editorInstance->gridScale, &PL_COLOUR_BLUE );

				PlPopMatrix();
			}
#endif

void ss_arl_draw_scene_( SSArlCamera *camera, const SSArlViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	ape_rendererPerformance_.cameraPos = camera->internal->position;

	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL );

	if ( ( camera != NULL && camera->drawMode == SS_ARL_CAMERA_DRAW_MODE_WIREFRAME ) || ape_config_.renderer.wireframe )
		PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );

	render_scene( camera, viewport );

	if ( ( camera != NULL && camera->drawMode == SS_ARL_CAMERA_DRAW_MODE_WIREFRAME ) || ape_config_.renderer.wireframe )
		PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );

	COM_PROFILE_FUNCTION_END();
}

SSArlRenderTarget *ss_arl_get_default_render_target( void )
{
	return defaultRenderTarget;
}
