/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plgraphics/plg_driver_interface.h>

#include "ape_private.h"
#include "legacy/actor.h"
#include "renderer_font.h"
#include "world/world.h"
#include "renderer.h"
#include "renderer_particle.h"
#include "ar_render_target.h"

#include "client/ape_client_gui.h"
#include "editor/editor.h"

#include "post/post.h"

ApeRendererStats ape_rendererPerformance_;
ApeRendererPassState rendererState;

static ArRenderTarget *defaultRenderTarget;

static PLGCamera *auxCamera = NULL;
PLGCamera *apeGetAuxCamera( void ) { return auxCamera; }

static bool isScreenshotPending = false;

/**********************************************************/

void ar_setup_default_state( const ApeViewport *viewport )
{
	PLColour clearColour = { 50, 50, 50, 255 };

	ApeWorld *world = apeGetCurrentWorld();
	if ( world != NULL && ( viewport->camera == NULL || viewport->camera->mode == APE_CAMERA_MODE_PERSPECTIVE ) )
		clearColour = PlColourF32ToU8( &world->clearColour );

	PlgSetClearColour( clearColour );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgDepthMask( true );

	PlgSetCullMode( PLG_CULL_POSITIVE );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );
}

void ar_draw_begin( ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	double newTime = PlGetCurrentSeconds();

	viewport->perf.frameReadings[ viewport->perf.frameIndex++ ] = 1.0 / ( newTime - viewport->perf.oldTime );
	if ( viewport->perf.frameIndex >= APE_MAX_FPS_READINGS )
		viewport->perf.frameIndex = 0;

	viewport->perf.oldTime = newTime;

	ar_setup_default_state( viewport );

	PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );
	PlgClearBuffers( PLG_BUFFER_DEPTH | PLG_BUFFER_COLOUR );

	COM_PROFILE_FUNCTION_END();
}

static void write_screenshot( void )
{
	unsigned int w, h;
	ar_render_target_get_size( defaultRenderTarget, &w, &h );

	PLGFrameBuffer *fboBuffer = ar_render_target_get_frame_buffer( defaultRenderTarget );
	assert( fboBuffer != NULL );
	if ( fboBuffer == NULL )
		return;

	size_t bufSize = ( ( w * h ) * 4 );
	unsigned char *buf = PL_NEW_( unsigned char, bufSize );
	if ( PlgReadFrameBufferRegion( fboBuffer, 0, 0, w, h, bufSize, buf ) != NULL )
	{
		PLImage *image = PlCreateImage( buf, w, h, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );
		assert( image != NULL );
		if ( image != NULL )
		{
			uint16_t num = 0;
			PLPath path;
			PlSetupPath( path, true, "%s/screen%u.png", comGetAppDataDirectory(), num );
			while ( PlFileExists( path ) )
				PlSetupPath( path, true, "%s/screen%u.png", comGetAppDataDirectory(), ++num );

			PlFlipImageVertical( image );

			PlWriteImage( image, path );
			PlDestroyImage( image );
		}
		else
			PRINT_WARNING( "Failed to create image for screenshot: %s\n", PlGetError() );
	}
	else
		PRINT_WARNING( "Failed to read framebuffer for screenshot: %s\n", PlGetError() );

	PL_DELETE( buf );
}

void ar_draw_end( ApeViewport *viewport )
{
	PL_ZERO_( ape_rendererPerformance_ );

	viewport->perf.numBatches = 0;
	viewport->perf.numTriangles = 0;
	viewport->perf.numPolygons = 0;
	viewport->perf.numPortals = 0;

	if ( isScreenshotPending )
	{
		write_screenshot();
		isScreenshotPending = false;
	}
}

void apeInitializeShaders_( void );  /* renderer/shaders.c */
void apeInitializeTextures_( void ); /* texture.c */

/* renderer_rendertarget.c */
void ar_initialize_render_targets( void );
void ar_shutdown_render_targets( void );

static void prepare_screenshot_capture( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	isScreenshotPending = true;
}

void apeRegisterRendererConsoleVariables_( void )
{
	PlRegisterConsoleCommand( "screenshot", "Take a screenshot.", 0, prepare_screenshot_capture );

	PlRegisterConsoleVariable( "r/cullMode", "Face culling mode.", "1", PL_VAR_I32, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/superSampling", "Resolution multiplier.", "1", PL_VAR_I32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/showActorBounds", "Toggle actor bounds.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "fps", "Toggle FPS counter.",
#if !defined( NDEBUG )
	                           "true",
#else
	                           "false",
#endif
	                           PL_VAR_BOOL, &ape_config_.renderer.showFps, NULL, true );
	PlRegisterConsoleVariable( "ape/r/wireframe", "Enable wireframe mode.", "0", PL_VAR_BOOL, &ape_config_.renderer.wireframe, NULL, false );
	PlRegisterConsoleVariable( "ape/r/skyHeightOffset", "Height of the sky relative to the camera.", "10", PL_VAR_F32, NULL, NULL, false );
	PlRegisterConsoleVariable( "ape/r/skyCull", "Cull backfaces for the sky. Only useful if you set the offset lower than the camera.", "1", PL_VAR_BOOL, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/skipDiffuse", "Skip diffuse map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/skipNormal", "Skip normal map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/skipSpecular", "Skip specular map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "ape/r/useStencilShadowVolumes", "Use stencil shadow volumes.", "true", PL_VAR_BOOL, &ape_config_.renderer.useStencilShadowVolumes, NULL, true );
	PlRegisterConsoleVariable( "ape/r/showShadowWireframe", "Show the wireframe of the stencil shadow volume.", "false", PL_VAR_BOOL, &ape_config_.renderer.showShadowWireframe, NULL, false );
	PlRegisterConsoleVariable( "ape/r/maxLightDistance", "Maximum distance before lights are culled.", "1024", PL_VAR_F32, &ape_config_.renderer.maxLightDistance, NULL, true );

	// Camera
	PlRegisterConsoleVariable( "r/fov", "", "75", PL_VAR_F32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/near", "", "0.1", PL_VAR_F32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/far", "", "1000.0", PL_VAR_F32, NULL, NULL, true );

	PlRegisterConsoleVariable( "ape/r/fogNear", "Fog near value.", "-1", PL_VAR_F32, NULL, NULL, false );
	PlRegisterConsoleVariable( "ape/r/fogFar", "Fog far value.", "-1", PL_VAR_F32, NULL, NULL, false );
}

void ar_initialize_( void )
{
	PRINT( "Initializing renderer\n" );

	PL_ZERO_( rendererState );

	apeInitializeTextures_();

	ar_initialize_render_targets();
	apeInitializeShaders_();
	apeInitializeMaterialSystem();
	YR_Font_Initialize();

	apeInitializeWorldVisibilitySystem_();

	auxCamera = PlgCreateCamera();
	if ( auxCamera == NULL )
		PRINT_ERROR( "Failed to create auxiliary camera: %s\n", PlGetError() );

	auxCamera->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
	auxCamera->near = -10000.0f;
	auxCamera->far = 10000.0f;

	ar_setup_default_state( NULL );

	defaultRenderTarget = ar_render_target_create( "default",
	                                               800, 600,
	                                               PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL,
	                                               PLG_BUFFER_COLOUR,
	                                               PLG_TEXTURE_FILTER_LINEAR );
	if ( defaultRenderTarget == NULL )
		PRINT_ERROR( "Failed to create default render target!\n" );

	ar_postfx_setup_();
}

void ar_shutdown_( void )
{
	ar_postfx_cleanup_();

	Font_Shutdown();
	apeShutdownMaterialSystem();
	ar_shutdown_render_targets();
	apeShutdownWorldVisibilitySystem_();
}

static void draw_debug_overlay( const ApeViewport *viewport )
{
	PL_GET_CVAR( "debug/overlay", debugOverlay );
	if ( debugOverlay->i_value <= 0 )
		return;

	ApeBitmapFont *defaultFont = apeGetDefaultSmallBitmapFont();
	assert( defaultFont != NULL );
	if ( defaultFont == NULL )
		return;

	apeBeginBitmapFontDraw( defaultFont );

	static const float sy = 8;
	static const float sx = 8;
	static const float tx = 8 + 4;
	float y = sy;

	const ApeCamera *camera = viewport->camera;
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
		apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_WHITE, buf, strlen( buf ), false );
	}

	// Draw stats
	char buf[ 64 ];
	snprintf( buf, sizeof( buf ), "FPS:              " PL_FMT_uint32 "\n", apeGetViewportFramerate( viewport ) );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num rooms:        " PL_FMT_uint32 "\n", ape_rendererPerformance_.numRooms );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num detail rooms: " PL_FMT_uint32 "\n", ape_rendererPerformance_.numDetailRooms );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num portals:      " PL_FMT_uint32 "\n", ape_rendererPerformance_.numVisiblePortals );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num faces:        " PL_FMT_uint32 "\n", ape_rendererPerformance_.numFacesDrawn );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num lights:       " PL_FMT_uint32 "\n", ape_rendererPerformance_.numLights );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num triangles:    " PL_FMT_uint32 "\n", ape_rendererPerformance_.numTriangles );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num batches:      " PL_FMT_uint32 "\n", ape_rendererPerformance_.numBatches );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "---------------------\n" );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_WHITE, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Alloc memory:     %.2lfMB\n", PlBytesToMegabytes( PlGetTotalAllocatedMemory() ) );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_ORCHID, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Total memory:     %.2lfMB\n", PlBytesToMegabytes( PlGetCurrentMemoryUsage() ) );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_ORCHID, buf, strlen( buf ), false );

	unsigned int numTasks = apeGetNumScheduledTasks();
	snprintf( buf, sizeof( buf ), "Num tasks:     " PL_FMT_uint32 "\n", numTasks );
	apeAddBitmapStringToBatch( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	for ( unsigned int i = 0; i < numTasks; ++i )
	{
		double taskDelay;
		const char *taskDescription = apeGetScheduledTaskDescription( i, &taskDelay );
		snprintf( buf, sizeof( buf ), "%u %s\n", i, taskDescription );
		apeAddBitmapStringToBatch( defaultFont, tx + 8, y += defaultFont->ch, 1.0f, PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	}
	y += defaultFont->ch * 2;

	static const float bw = 128;

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
	PlgSetBlendMode( PLG_BLEND_DEFAULT );
	PlgDrawRectangle( sx, sy, bw, y - sy, PLColour( 0, 0, 0, 200 ) );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	apeDrawBitmapFont( defaultFont );

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
			ar_draw_graph( name, x, y, bw, GRAPH_HEIGHT, graph, numPoints, .0f, 1.0f );
			y += GRAPH_HEIGHT + Y_SPACING;

			group = comGetNextProfilingGroup( group );
		}
	}
}

void apeSet2DViewportSize( int w, int h )
{
	PlgSetViewport( 0, 0, w, h );
	PlgSetupCamera( auxCamera );
}

void apeGet2DViewportSize( int *width, int *height )
{
	PlgGetViewport( NULL, NULL, width, height );
}

void ar_draw_menu( const ApeViewport *viewport )
{
	if ( viewport == NULL )
		return;

	COM_PROFILE_FUNCTION_START();

	apeSet2DViewportSize( viewport->width, viewport->height );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	ar_postfx_draw_( viewport );

	apeDrawGUI_( viewport );
	apeDrawEditorGUI_( viewport );

	draw_debug_overlay( viewport );

	PlgSetTexture( NULL, 0 );

	PlPopMatrix();

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );

	COM_PROFILE_FUNCTION_END();
}

/****************************************
 * SKY
 * Scrapped and then reintroduced for the
 * ALIVE event... *sigh*
 ****************************************/

#define MAX_SKY_LAYERS 4
static unsigned int numSkyLayers = 0;
static ApeMaterial *skyMaterials[ MAX_SKY_LAYERS ];

static void draw_sky_layer( PLGMesh *mesh, ApeMaterial *material, const PLVector3 *location, float x, float y, float scale )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlTranslateMatrix( *location );

	/* todo: do this in shader... */
	PlgGenerateTextureCoordinates( mesh->vertices, mesh->num_verts, PLVector2( x, y ), PLVector2( scale, scale ) );
	mesh->isDirty = true;
	apeDrawMesh( material, mesh, NULL, 0 );

	PlPopMatrix();
}

void apeAddSkyLayer_( const char *path )
{
	skyMaterials[ numSkyLayers ] = apeCacheMaterial( path, APE_CACHE_WORLD, false, false );
	if ( skyMaterials[ numSkyLayers ] == NULL )
		return;

	numSkyLayers++;
}

void apeClearSkyLayers_( void )
{
	for ( unsigned int i = 0; i < numSkyLayers; ++i )
	{
		ar_material_release( skyMaterials[ i ] );
		skyMaterials[ i ] = NULL;
	}

	numSkyLayers = 0;
}

/**
 * Draw scrolling clouds.
 */
void apeDrawSky_( ApeCamera *camera )
{
	if ( numSkyLayers == 0 )
		return;

	static PLGMesh *skyMesh = NULL;
	if ( skyMesh == NULL )
	{
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

		skyMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, numTriangles, 8 );
		if ( skyMesh == NULL )
		{
			PRINT_WARNING( "Failed to create sky mesh: %s\n", PlGetError() );
			return;
		}

		PlgAddMeshVertex( skyMesh, &PLVector3( 100.0f, 10.0f, 100.0f ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &pl_vecOrigin2 );   /* top right */
		PlgAddMeshVertex( skyMesh, &PLVector3( 200.0f, 10.0f, 200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );    /* top right far */
		PlgAddMeshVertex( skyMesh, &PLVector3( 100.0f, 10.0f, -100.0f ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &pl_vecOrigin2 );  /* lower right */
		PlgAddMeshVertex( skyMesh, &PLVector3( 200.0f, 10.0f, -200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );   /* lower right far */
		PlgAddMeshVertex( skyMesh, &PLVector3( -100.0f, 10.0f, -100.0f ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &pl_vecOrigin2 ); /* lower left */
		PlgAddMeshVertex( skyMesh, &PLVector3( -200.0f, 10.0f, -200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );  /* lower left far */
		PlgAddMeshVertex( skyMesh, &PLVector3( -100.0f, 10.0f, 100.0f ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &pl_vecOrigin2 );  /* top left */
		PlgAddMeshVertex( skyMesh, &PLVector3( -200.0f, 10.0f, 200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );   /* top left far */

		for ( unsigned int i = 0; i < numTriangles; ++i )
			PlgAddMeshTriangle( skyMesh, indices[ i ][ 0 ], indices[ i ][ 1 ], indices[ i ][ 2 ] );

		PlgUploadMesh( skyMesh );
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );
	PlgDepthMask( false );

	PLVector3 location;
	location = camera->internal->position;
	location.y += 10.0f;

	float s = 0.15f;
	double ticks = apeGetNumTicks();
	double div = 400;

	for ( unsigned int i = 0; i < numSkyLayers; ++i )
	{
		draw_sky_layer( skyMesh, skyMaterials[ i ], &location, ticks / ( div + 200 ), ticks / div, s );
		location.y += 2.0f;
		s += 0.15f;
		div -= 100;
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgDepthMask( true );
}

/****************************************
 ****************************************/

PLVector2 screenPosTest;
static void render_scene( ApeCamera *camera, const ApeViewport *viewport )
{
	ape_rendererPerformance_.numLights = 0;

	ApeWorld *world = apeGetCurrentWorld();
	if ( world == NULL )
		return;

	// Ambient pass ...

	//PlgDepthBufferFunction( PLG_COMPARE_LEQUAL );

	apeDrawSky_( camera );
	apeDrawWorld_( world, camera, NULL, true );

	unsigned int numLights;
	ApeLight **lights = apeGetVisibleLights_( &numLights );

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

		ar_draw_axis_pivot( lights[ i ]->position, lights[ i ]->angles, 1.0f );

		if ( lights[ i ]->flags & APE_LIGHT_FLAG_RUNTIME_SHADOWS )
		{
			if ( ape_config_.renderer.showShadowWireframe )
			{
				rendererState.cullMode = APE_RENDERER_CULL_NONE;
				rendererState.passStage = APE_RENDERER_PASS_DEFAULT;

				PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
				apeDrawWorldStencilShadowPass_( world, camera, lights[ i ] );
				PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );

				rendererState.cullMode = APE_RENDERER_CULL_DEFAULT;
			}

			PlgEnableGraphicsState( PLG_GFX_STATE_STENCILTEST );
			PlgEnableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
			PlgColourMask( false, false, false, false );
			PlgDepthMask( false );

			rendererState.cullMode = APE_RENDERER_CULL_NONE;
			PlgStencilBufferFunction( PLG_COMPARE_ALWAYS, 0x0, 0xFF );
			PlgStencilOp( PLG_STENCIL_FACE_FRONT, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_INCRWRAP, PLG_STENCIL_OP_KEEP );
			PlgStencilOp( PLG_STENCIL_FACE_BACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_DECRWRAP, PLG_STENCIL_OP_KEEP );

			apeDrawWorldStencilShadowPass_( world, camera, lights[ i ] );

			PlgDisableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
			PlgColourMask( true, true, true, true );

			PlgDepthBufferFunction( PLG_COMPARE_EQUAL );
			PlgStencilBufferFunction( PLG_COMPARE_EQUAL, 0x0, 0xFF );
			PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP );

			rendererState.overrideBlendMode = true;
			rendererState.blendModeA = PLG_BLEND_ONE;
			rendererState.blendModeB = PLG_BLEND_ONE;

			apeDrawWorld_( world, camera, lights[ i ], false );

			rendererState.overrideBlendMode = false;

			PlgDisableGraphicsState( PLG_GFX_STATE_STENCILTEST );
			PlgDepthMask( true );

			rendererState.cullMode = APE_RENDERER_CULL_DEFAULT;
		}
		else
		{
			PlgDepthBufferFunction( PLG_COMPARE_EQUAL );

			rendererState.overrideBlendMode = true;
			rendererState.blendModeA = PLG_BLEND_ONE;
			rendererState.blendModeB = PLG_BLEND_ONE;

			apeDrawWorld_( world, camera, lights[ i ], false );

			PlgDepthBufferFunction( PLG_COMPARE_LESS );

			rendererState.overrideBlendMode = false;
			rendererState.passStage = APE_RENDERER_PASS_DEFAULT;
		}

		ape_rendererPerformance_.numLights++;
	}

	PlgClipViewport( viewport->x, viewport->y, viewport->width, viewport->height );
	rendererState.passStage = APE_RENDERER_PASS_DEFAULT;
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

void ar_draw_scene_( ApeCamera *camera, const ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	ape_rendererPerformance_.cameraPos = camera->internal->position;

	// We're going to draw into a texture, so set that up first
	ar_render_target_set_size( defaultRenderTarget, viewport->width, viewport->height );
	ar_render_target_bind( defaultRenderTarget, PLG_FRAMEBUFFER_DRAW );

	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL );

	if ( ( camera != NULL && camera->drawMode == APE_CAMERA_DRAW_MODE_WIREFRAME ) || ape_config_.renderer.wireframe )
		PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );

	render_scene( camera, viewport );

	if ( ( camera != NULL && camera->drawMode == APE_CAMERA_DRAW_MODE_WIREFRAME ) || ape_config_.renderer.wireframe )
		PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );

	COM_PROFILE_FUNCTION_END();
}

ArRenderTarget *ar_get_default_render_target( void ) { return defaultRenderTarget; }
