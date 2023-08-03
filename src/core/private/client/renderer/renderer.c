/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plgraphics/plg_driver_interface.h>

#include "ape_private.h"
#include "legacy/actor.h"
#include "renderer_font.h"
#include "world/world.h"
#include "renderer.h"
#include "renderer_particle.h"

#include "client/ape_client_gui.h"
#include "editor/editor.h"

#include "post/post.h"

ApeRendererStats ape_rendererPerformance_;
ApeRendererPassState rendererState;

static PLGCamera *auxCamera = NULL;
PLGCamera *apeGetAuxCamera( void ) { return auxCamera; }

static PLGFrameBuffer *fboBuffer;

static bool isScreenshotPending = false;

/* Post Processing */

void apeSetupRenderTarget( PLGFrameBuffer **buffer, PLGTexture **attachment, PLGTexture **depthAttachment, unsigned int w, unsigned int h )
{
	unsigned int bw = 0, bh = 0;
	if ( *buffer != NULL )
		PlgGetFrameBufferResolution( *buffer, &bw, &bh );

	/* need to rebuild the framebuffer object
	 * todo: the library should provide us a func to perform a resize? */
	if ( bw != w || bh != h )
	{
		PlgDestroyFrameBuffer( *buffer );
		*buffer = PlgCreateFrameBuffer( w, h, PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL );
		if ( *buffer == NULL )
			PRINT_ERROR( "Failed to create framebuffer: %s\n", PlGetError() );

		PlgDestroyTexture( *attachment );
		*attachment = PlgGetFrameBufferTextureAttachment( *buffer, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR );
		if ( *attachment == NULL )
			PRINT_ERROR( "Failed to create texture attachment: %s\n", PlGetError() );

		if ( depthAttachment != NULL )
		{
			PlgDestroyTexture( *depthAttachment );
			*depthAttachment = PlgGetFrameBufferTextureAttachment( *buffer, PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL, PLG_TEXTURE_FILTER_LINEAR );
			if ( *depthAttachment == NULL )
				PRINT_ERROR( "Failed to create depth attachment: %s\n", PlGetError() );
		}
	}

	/* reset */
	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );
}

/**********************************************************/

static PLGTexture *numTextureTable[ 10 ];

void R_DrawDigit( float x, float y, int digit )
{
	if ( digit < 0 )
		digit = 0;
	else if ( digit > 9 )
		digit = 9;

	PlgDrawTexturedRectangle( x, y, ( float ) numTextureTable[ digit ]->w, ( float ) numTextureTable[ digit ]->h, numTextureTable[ digit ] );
}

void R_DrawNumber( float x, float y, int number )
{
	/* restrict it for sanity */
	if ( number < 0 )
		number = 0;
	else if ( number > 999 )
		number = 999;

	if ( number >= 100 )
	{
		int digit = number / 100;
		R_DrawDigit( x, y, digit );
		x += ( float ) numTextureTable[ digit ]->w + 1;
	}

	if ( number >= 10 )
	{
		int digit = ( number / 10 ) % 10;
		R_DrawDigit( x, y, digit );
		x += ( float ) numTextureTable[ digit ]->w + 1;
	}

	R_DrawDigit( x, y, number % 10 );
}

void apeSetupDefaultRenderState( const ApeViewport *viewport )
{
	PLColour clearColour = { 50, 50, 50, 255 };

	ApeWorld *world = apeGetCurrentWorld();
	if ( world != NULL && ( viewport->camera == NULL || viewport->camera->mode == APE_CAMERA_MODE_PERSPECTIVE ) )
		clearColour = PlColourF32ToU8( &world->clearColour );

	PlgSetClearColour( clearColour );

	PlgEnableGraphicsState( PLG_GFX_STATE_SCISSORTEST );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgDepthMask( true );

	PlgSetCullMode( PLG_CULL_POSITIVE );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );
}

void apeBeginDraw( ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	double newTime = PlGetCurrentSeconds();

	viewport->perf.frameReadings[ viewport->perf.frameIndex++ ] = 1.0 / ( newTime - viewport->perf.oldTime );
	if ( viewport->perf.frameIndex >= APE_MAX_FPS_READINGS )
	{
		viewport->perf.frameIndex = 0;
	}
	viewport->perf.oldTime = newTime;

	apeSetupDefaultRenderState( viewport );

	PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );
	PlgClearBuffers( PLG_BUFFER_DEPTH | PLG_BUFFER_COLOUR );

	COM_PROFILE_FUNCTION_END();
}

static void WriteScreenshot( void )
{
	uint32_t w, h;
	PlgGetFrameBufferResolution( fboBuffer, &w, &h );

	size_t bufSize = ( ( w * h ) * 4 );
	uint8_t *buf   = PL_NEW_( uint8_t, bufSize );
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
			{
				PlSetupPath( path, true, "%s/screen%u.png", comGetAppDataDirectory(), ++num );
			}

			PlFlipImageVertical( image );

			PlWriteImage( image, path );
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

void apeEndDraw( ApeViewport *viewport )
{
	PL_ZERO_( ape_rendererPerformance_ );

	viewport->perf.numBatches   = 0;
	viewport->perf.numTriangles = 0;
	viewport->perf.numPolygons  = 0;
	viewport->perf.numPortals   = 0;

	if ( isScreenshotPending )
	{
		WriteScreenshot();
		isScreenshotPending = false;
	}
}

void apeInitializeShaders_( void );  /* renderer/shaders.c */
void apeInitializeTextures_( void ); /* texture.c */

/* renderer_rendertarget.c */
void apeInitializeRenderTargets( void );
void apeShutdownRenderTargets( void );

static void PrepareScreenshotCapture( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	isScreenshotPending = true;
}

void Renderer_RegisterConsoleVariables( void )
{
	PlRegisterConsoleCommand( "screenshot", "Take a screenshot.", 0, PrepareScreenshotCapture );

	PlRegisterConsoleVariable( "r/cullMode", "Face culling mode.", "1", PL_VAR_I32, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/superSampling", "Resolution multiplier.", "1", PL_VAR_I32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/showActorBounds", "Toggle actor bounds.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/showFPS", "Toggle FPS counter.",
#if !defined( NDEBUG )
	                           "true",
#else
	                           "false",
#endif
	                           PL_VAR_BOOL, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/wireframe", "Enable wireframe mode.", "0", PL_VAR_BOOL, &ape_config_.renderer.wireframe, NULL, false );
	PlRegisterConsoleVariable( "r/skyHeightOffset", "Height of the sky relative to the camera.", "10", PL_VAR_F32, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/skyCull", "Cull backfaces for the sky. Only useful if you set the offset lower than the camera.", "1", PL_VAR_BOOL, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/skipDiffuse", "Skip diffuse map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/skipNormal", "Skip normal map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/skipSpecular", "Skip specular map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r/driver", "Sets the default graphics driver. Requires restart.", "opengl", PL_VAR_STRING, NULL, NULL, true );
	PlRegisterConsoleVariable( "renderer/useStencilShadowVolumes", "Use stencil shadow volumes.", "true", PL_VAR_BOOL, &ape_config_.renderer.useStencilShadowVolumes, NULL, true );
	PlRegisterConsoleVariable( "renderer/showShadowWireframe", "Show the wireframe of the stencil shadow volume.", "false", PL_VAR_BOOL, &ape_config_.renderer.showShadowWireframe, NULL, false );

	// Camera
	PlRegisterConsoleVariable( "r/fov", "", "75", PL_VAR_F32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/near", "", "0.1", PL_VAR_F32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r/far", "", "1000.0", PL_VAR_F32, NULL, NULL, true );
}

void apeInitializeRenderer_( void )
{
	PRINT( "Initializing renderer\n" );

	PL_ZERO_( rendererState );

	apeInitializeTextures_();

	apeInitializeShaders_();
	apeInitializeRenderTargets();
	apeInitializeMaterialSystem();
	YR_Font_Initialize();

	auxCamera = PlgCreateCamera();
	if ( auxCamera == NULL )
		PRINT_ERROR( "Failed to create auxiliary camera: %s\n", PlGetError() );

	auxCamera->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
	auxCamera->near = -10000.0f;
	auxCamera->far  = 10000.0f;

	//SetupShadowMap();
	apeSetupDefaultRenderState( NULL );

	R_PP_SetupEffects();
}

void apeShutdownRenderer_( void )
{
	Font_Shutdown();
	apeShutdownMaterialSystem();
	apeShutdownRenderTargets();
}

/**
 * Where the magic of post processing happens.
 */
static void DrawScenePost( const ApeViewport *viewport )
{
	PL_GET_CVAR( "r/postProcessing", postProcessingVar );
	if ( postProcessingVar == NULL || !postProcessingVar->b_value )
	{
		return;
	}

	R_PP_Draw( viewport );
}

void YR_DrawGraph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max )
{
	if ( numPoints < 2 )
	{
		return;
	}

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

	double oa = min, ob = max;
	for ( unsigned int i = 0; i < numPoints; ++i )
	{
		if ( values[ i ] > max )
		{
			max = values[ i ];
		}
		if ( values[ i ] < min )
		{
			min = values[ i ];
		}
	}

#if 0
	bool outOfBounds = false;
	if ( oa != min || max != ob )
	{
		outOfBounds = true;
	}
#endif

	static PLColour colours[ 3 ] = {
	        PL_COLOUR_GREEN,
	        PL_COLOUR_YELLOW,
	        PL_COLOUR_RED,
	};

	unsigned int numOutPoints = ( numPoints - 1 ) * 2;
	PLVector3 *points         = PlCAllocA( numOutPoints, sizeof( PLVector3 ) );

	/* convert the values we've been provided into points in our graph */
	for ( unsigned int i = 0, j = 1; j < numPoints; i++, j++ )
	{
		points[ i ].x = x + ( ( w / ( numPoints - 1 ) ) * ( j - 1 ) );
		if ( min != max )
		{
			points[ i ].y = y + h - 1 - ( ( values[ j - 1 ] - min ) * ( h / ( max - min ) ) );
		}
		++i;

		points[ i ].x = x + ( ( w / ( numPoints - 1 ) ) * j );
		if ( min != max )
		{
			points[ i ].y = y + h - 1 - ( ( values[ j ] - min ) * ( h / ( max - min ) ) );
		}
		/* leave z, it'll be initialized as 0 */
	}

	PlgSetBlendMode( PLG_BLEND_DEFAULT );
	PlgDrawRectangle( x, y, w, h, PLColour( 0, 0, 0, 200 ) );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

#if 0
	PLVector3 border[] = {
	        { x, y, 0 },
	        { x + w, y, 0 },// top
	        { x, y, 0 },
	        { x, y + h, 0 },// left
	        { x + w, y + h, 0 },
	        { x, y + h, 0 },// bottom
	        { x + w, y + h, 0 },
	        { x + w, y, 0 },// right
	};
	PlgDrawLines( border, PL_ARRAY_ELEMENTS( border ), PL_COLOUR_GOLD, 1.0f );
#endif

	PlgDrawLines( points, numOutPoints, PL_COLOUR_WHITE, 1.0f );

	ApeBitmapFont *font = apeGetDefaultSmallBitmapFont();
	apeBeginBitmapFontDraw( font );

	if ( heading != NULL )
	{
		size_t len = strlen( heading );
		float cPos = ( x + w - ( len * font->cw ) ) - 2.0f;
		apeAddBitmapStringToBatch( font, cPos, y + 2.0f, 1.0f, PL_COLOUR_VIOLET, heading, len, false );
	}

	// Calculate the average sum of all the points
	double avg = 0.0;
	for ( unsigned int i = 0; i < numPoints; ++i )
	{
		avg += values[ i ];
	}
	avg /= numPoints;

	char buf[ 128 ];

	// Current and average readings
	snprintf( buf, sizeof( buf ), "CUR %02f", values[ numPoints - 1 ] );
	apeAddBitmapStringToBatch( font, x + 2.0f, y + ( h / 2 ) - ( font->ch / 2 ) + font->ch, 1.0f, /*outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN*/ PL_COLOUR_VIOLET, buf, strlen( buf ), true );
	snprintf( buf, sizeof( buf ), "AVG %02f", avg );
	apeAddBitmapStringToBatch( font, x + 2.0f, y + ( h / 2 ) - ( font->ch / 2 ) - font->ch, 1.0f, /*outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN*/ PL_COLOUR_VIOLET, buf, strlen( buf ), true );

#if 0
	snprintf( buf, sizeof( buf ), "y+:%02f", max );
	Font_AddBitmapStringToPass( font, x + 2.0f, y + 2.0f, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "y-:%02f", min );
	Font_AddBitmapStringToPass( font, x + 2.0f, y + ( h - font->ch ) - 2.0f, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );
#endif

	apeDrawBitmapFont( font );

	PL_DELETE( points );
}

static void DrawDebugOverlay( const ApeViewport *viewport )
{
	PL_GET_CVAR( "debug/overlay", debugOverlay );
	if ( debugOverlay->i_value <= 0 )
	{
		return;
	}

	ApeBitmapFont *defaultFont = apeGetDefaultSmallBitmapFont();
	assert( defaultFont != NULL );
	if ( defaultFont == NULL )
	{
		return;
	}

	apeBeginBitmapFontDraw( defaultFont );

	static const float sy = 8;
	static const float sx = 8;
	static const float tx = 8 + 4;
	float y               = sy;

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
		static const float Y_SPACING    = 4.0f;
		static const float X_SPACING    = 4.0f;
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
			const char *name    = comGetProfilingGroupName( group );
			YR_DrawGraph( name, x, y, bw, GRAPH_HEIGHT, graph, numPoints, .0f, 1.0f );
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

void apeDrawMenu( const ApeViewport *viewport )
{
	if ( viewport == NULL )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	apeSet2DViewportSize( viewport->width, viewport->height );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	COM_PROFILE_FUNCTION_CALL( "DrawScenePost", DrawScenePost( viewport ) );

	//YRCamera camera;
	//PL_ZERO_( camera );
	//camera.internal = auxCamera;
	//YR_World_DrawWireframe( Game_GetCurrentWorld(), &camera );

	apeDrawGUI_( viewport );
	apeDrawEditorGUI_( viewport );

	DrawDebugOverlay( viewport );

	PlgSetTexture( NULL, 0 );

	PlPopMatrix();

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );

	COM_PROFILE_FUNCTION_END();
}

void apeDraw2DQuad( ApeMaterial *material, int x, int y, int w, int h, const PLColour *colour )
{
	static PLGMesh *mesh = NULL;
	if ( mesh == NULL )
		mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_STRIP, PLG_DRAW_DYNAMIC, 2, 4 );

	PlgClearMesh( mesh );

	PlgAddMeshVertex( mesh, &PlVector3( x, y + h, 0 ), &pl_vecOrigin3, colour, &PlVector2( 0, 0 ) );
	PlgAddMeshVertex( mesh, &PlVector3( x, y, 0 ), &pl_vecOrigin3, colour, &PlVector2( 0, 1 ) );
	PlgAddMeshVertex( mesh, &PlVector3( x + w, y + h, 0 ), &pl_vecOrigin3, colour, &PlVector2( 1, 0 ) );
	PlgAddMeshVertex( mesh, &PlVector3( x + w, y, 0 ), &pl_vecOrigin3, colour, &PlVector2( 1, 1 ) );

	apeDrawMesh( material, mesh, NULL, 0 );
}

void apeDrawAxesPivot( PLVector3 position, PLVector3 rotation, float scale )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

	PLVector3 angles;
	angles.x = PL_DEG2RAD( rotation.x );
	angles.y = PL_DEG2RAD( rotation.y );
	angles.z = PL_DEG2RAD( rotation.z );

	PlTranslateMatrix( position );

	PlRotateMatrix( angles.x, 1.0f, 0.0f, 0.0f );
	PlRotateMatrix( angles.y, 0.0f, 1.0f, 0.0f );
	PlRotateMatrix( angles.z, 0.0f, 0.0f, 1.0f );

	PLMatrix4 transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	PlgDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( scale, 0, 0 ), PLColour( 255, 0, 0, 255 ) );
	PlgDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 0, scale, 0 ), PLColour( 0, 255, 0, 255 ) );
	PlgDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, scale ), PLColour( 0, 0, 255, 255 ) );
	//printf( "%s\n", PlPrintVector3( &position, pl_int_var ) );

	PlPopMatrix();
}

static PLGTexture *colourTexture;
PLGTexture *apeGetPrimaryColourAttachment( void )
{
	return colourTexture;
}

static PLGTexture *depthTexture;
PLGTexture *apeGetPrimaryDepthAttachment( void )
{
	return depthTexture;
}

void apeDebugPlaceLight( PLVector3 position )
{
	ApeWorld *world = apeGetCurrentWorld();
	ApeLight *light = PlGetVectorArrayElementAt( world->lights, 0 );
	light->position = position;
}

static void RenderScene( ApeCamera *camera, const ApeViewport *viewport )
{
	ApeWorld *world = apeGetCurrentWorld();
	if ( world == NULL )
		return;

#if 1// test lights

	static float tick = 0.0f;

	PLVector3 forward;
	PlAnglesAxes( camera->internal->angles, NULL, NULL, &forward );

	int w, h;
	apeGet2DViewportSize( &w, &h );

	ApeLight *light = PlGetVectorArrayElementAt( world->lights, 0 );
	light->flags |= APE_LIGHT_FLAG_RUNTIME_SHADOWS;

	PLCollisionSphere sphere = PlSetupCollisionSphere( light->position, light->radius );
	if ( PlgIsSphereInsideView( camera->internal, &sphere ) )
	{
		PLMatrix4 viewProj  = PlMultiplyMatrix4( camera->internal->internal.proj, &camera->internal->internal.view );
		PLVector2 screenPos = PlConvertWorldToScreen( &light->position, &viewProj, w, h, 0, 0 );
		//screenPosTest       = screenPos;
	}

	light->position = PlAddVector3( apeGetCameraPosition( camera ), PlScaleVector3F( forward, 5.0f ) );
	//static PLVector3 position = PLVector3( -26.0f, -3.5f, 58.0f );
	//light->position           = position;
	light->position = PlAddVector3( light->position, ( PLVector3 ){
	                                                         ( sinf( ( float ) apeGetNumTicks() / 100.0f ) / 100.0f ) * 100.0f,
	                                                         ( cosf( ( float ) apeGetNumTicks() / 100.0f ) / 100.0f ) * 50.0f,
	                                                         ( sinf( ( float ) apeGetNumTicks() / 100.0f ) / 100.0f ) * 100.0f } );

	light->colour.r = 1.0f;
	light->colour.g = 1.0f;
	light->colour.b = 1.0f;
	light->radius   = 16.0f;

	apeDrawAxesPivot( light->position, light->angles, 1.0f );

	tick += 0.5f;

#endif

	apeDrawWorld_( world, false );

	if ( ape_config_.renderer.useStencilShadowVolumes && !ape_config_.renderer.wireframe )
	{
		PlgEnableGraphicsState( PLG_GFX_STATE_STENCILTEST );
		PlgEnableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
		PlgColourMask( false, false, false, false );
		PlgDepthMask( false );

		rendererState.cullMode = APE_RENDERER_CULL_NONE;
		PlgStencilBufferFunction( PLG_COMPARE_ALWAYS, 0x0, 0xFF );
		PlgStencilOp( PLG_STENCIL_FACE_FRONT, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_INCRWRAP, PLG_STENCIL_OP_KEEP );
		PlgStencilOp( PLG_STENCIL_FACE_BACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_DECRWRAP, PLG_STENCIL_OP_KEEP );

		apeDrawWorldStencilShadowPass_( world, camera, light );

		PlgDisableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
		PlgColourMask( true, true, true, true );

		PlgDepthBufferFunction( PLG_COMPARE_EQUAL );

		PlgStencilBufferFunction( PLG_COMPARE_NOTEQUAL, 0x0, 0xFF );
		PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP );

		apeDrawWorld_( world, true );

		rendererState.cullMode = APE_RENDERER_CULL_DEFAULT;

		PlgDisableGraphicsState( PLG_GFX_STATE_STENCILTEST );

		PlgDepthMask( true );
		PlgDepthBufferFunction( PLG_COMPARE_LESS );

		if ( ape_config_.renderer.showShadowWireframe )
		{
			rendererState.cullMode = APE_RENDERER_CULL_NONE;

			PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
			apeDrawWorldStencilShadowPass_( world, camera, NULL );
			PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );

			rendererState.cullMode = APE_RENDERER_CULL_DEFAULT;
		}
	}
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

void apeDrawScene_( ApeCamera *camera, const ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	ape_rendererPerformance_.cameraPos = camera->internal->position;

	// We're going to draw into a texture, so set that up first
	apeSetupRenderTarget( &fboBuffer, &colourTexture, &depthTexture, viewport->width, viewport->height );
	PlgBindFrameBuffer( fboBuffer, PLG_FRAMEBUFFER_DRAW );

	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL );

	if ( ( camera != NULL && camera->drawMode == APE_CAMERA_DRAW_MODE_WIREFRAME ) || ape_config_.renderer.wireframe )
		PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );

	RenderScene( camera, viewport );

	if ( ( camera != NULL && camera->drawMode == APE_CAMERA_DRAW_MODE_WIREFRAME ) || ape_config_.renderer.wireframe )
		PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );

	COM_PROFILE_FUNCTION_END();
}
