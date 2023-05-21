/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plgraphics/plg_driver_interface.h>

#include "core_private.h"
#include "legacy/actor.h"
#include "renderer_font.h"
#include "world.h"
#include "game_interface.h"
#include "renderer.h"
#include "renderer_particle.h"

#include "client/client_gui.h"
#include "editor/editor.h"

#include "post/post.h"

OgeRendererStats g_gfxPerfStats;
OgeRendererPassState rendererState;

static PLGCamera *auxCamera = NULL;
PLGCamera *ogeGetAuxCamera( void ) { return auxCamera; }

static PLGFrameBuffer *fboBuffer;

/* Post Processing */

void ogeSetupRenderTarget( PLGFrameBuffer **buffer, PLGTexture **attachment, PLGTexture **depthAttachment, unsigned int w, unsigned int h )
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

void YnCore_SetupDefaultRenderState( const OgeViewport *viewport )
{
	PLColour clearColour = { 50, 50, 50, 255 };

	OgeWorld *world = Game_GetCurrentWorld();
	if ( world != NULL && ( viewport->camera == NULL || viewport->camera->mode == YN_CORE_CAMERA_MODE_PERSPECTIVE ) )
	{
		clearColour = PlColourF32ToU8( &world->clearColour );
	}

	PlgSetClearColour( clearColour );

	PlgEnableGraphicsState( PLG_GFX_STATE_SCISSORTEST );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgSetDepthMask( true );

	PlgSetCullMode( PLG_CULL_POSITIVE );

	PlgSetShaderProgram( oge_defaultShaderPrograms[ OGE_SHADER_DEFAULT ] );
}

void YnCore_BeginDraw( OgeViewport *viewport )
{
	double newTime = PlGetCurrentSeconds();

	viewport->perf.frameReadings[ viewport->perf.frameIndex++ ] = 1.0 / ( newTime - viewport->perf.oldTime );
	if ( viewport->perf.frameIndex >= YN_CORE_MAX_FPS_READINGS )
	{
		viewport->perf.frameIndex = 0;
	}
	viewport->perf.oldTime = newTime;

	YnCore_SetupDefaultRenderState( viewport );

	PlgSetViewport( viewport->x, viewport->y, viewport->width, viewport->height );
	PlgClearBuffers( PLG_BUFFER_DEPTH | PLG_BUFFER_COLOUR );
}

void YnCore_EndDraw( OgeViewport *viewport )
{
	PL_ZERO_( g_gfxPerfStats );

	viewport->perf.numBatches   = 0;
	viewport->perf.numTriangles = 0;
	viewport->perf.numPolygons  = 0;
	viewport->perf.numPortals   = 0;
}

static OgeMaterial *ppFXAAMaterial = NULL;

void ogeInitializeShaders( void );  /* renderer/shaders.c */
void RT_InitializeTextures( void ); /* texture.c */

/* renderer_rendertarget.c */
void ogeInitializeRenderTargets( void );
void ogeShutdownRenderTargets( void );

void Renderer_RegisterConsoleVariables( void )
{
	PlRegisterConsoleVariable( "r.cullMode", "Face culling mode.", "1", PL_VAR_I32, NULL, NULL, false );
	PlRegisterConsoleVariable( "r.superSampling", "Resolution multiplier.", "1", PL_VAR_I32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r.showActorBounds", "Toggle actor bounds.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r.showFPS", "Toggle FPS counter.", "false", PL_VAR_BOOL, NULL, NULL, true );
	PlRegisterConsoleVariable( "r.wireframe", "Enable wireframe mode.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r.skyHeightOffset", "Height of the sky relative to the camera.", "10", PL_VAR_F32, NULL, NULL, false );
	PlRegisterConsoleVariable( "r.skyCull", "Cull backfaces for the sky. Only useful if you set the offset lower than the camera.", "1", PL_VAR_BOOL, NULL, NULL, true );
	PlRegisterConsoleVariable( "r.skipDiffuse", "Skip diffuse map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r.skipNormal", "Skip normal map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r.skipSpecular", "Skip specular map.", "0", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "r.driver", "Sets the default graphics driver. Requires restart.", "opengl", PL_VAR_STRING, NULL, NULL, true );

	// Camera
	PlRegisterConsoleVariable( "r.fov", "", "75", PL_VAR_F32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r.near", "", "0.1", PL_VAR_F32, NULL, NULL, true );
	PlRegisterConsoleVariable( "r.far", "", "1000.0", PL_VAR_F32, NULL, NULL, true );
}

void ogeInitializeRenderer( void )
{
	PRINT( "Initializing renderer\n" );

	PL_ZERO_( rendererState );

	RT_InitializeTextures();

	ogeInitializeShaders();
	ogeInitializeRenderTargets();
	ogeInitializeMaterialSystem();
	YR_Font_Initialize();

	auxCamera = PlgCreateCamera();
	if ( auxCamera == NULL )
		PRINT_ERROR( "Failed to create auxiliary camera: %s\n", PlGetError() );

	auxCamera->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
	auxCamera->near = -10000.0f;
	auxCamera->far  = 10000.0f;

	//SetupShadowMap();
	YnCore_SetupDefaultRenderState( NULL );

	R_PP_SetupEffects();
}

void ogeShutdownRenderer( void )
{
	Font_Shutdown();
	ogeShutdownMaterialSystem();
	ogeShutdownRenderTargets();
}

/**
 * Where the magic of post processing happens.
 */
static void DrawScenePost( const OgeViewport *viewport )
{
	PL_GET_CVAR( "r.postProcessing", postProcessingVar );
	if ( postProcessingVar == NULL || !postProcessingVar->b_value )
	{
		return;
	}

	R_PP_Draw( viewport );
}

void YR_DrawGraph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max )
{
	if ( numPoints < 2 )
		return;

	PlgSetShaderProgram( oge_defaultShaderPrograms[ OGE_SHADER_DEFAULT_VERTEX ] );

	double oa = min, ob = max;
	for ( unsigned int i = 0; i < numPoints; ++i )
	{
		if ( values[ i ] > max )
			max = values[ i ];
		if ( values[ i ] < min )
			min = values[ i ];
	}

	bool outOfBounds = false;
	if ( oa != min || max != ob )
		outOfBounds = true;

	unsigned int numOutPoints = ( numPoints - 1 ) * 2;
	PLVector3 *points         = PlCAllocA( numOutPoints, sizeof( PLVector3 ) );

	/* convert the values we've been provided into points in our graph */
	for ( unsigned int i = 0, j = 1; j < numPoints; i++, j++ )
	{
		points[ i ].x = x + ( ( w / ( numPoints - 1 ) ) * ( j - 1 ) );
		if ( min != max )
			points[ i ].y = y + h - 1 - ( ( values[ j - 1 ] - min ) * ( h / ( max - min ) ) );

		++i;

		points[ i ].x = x + ( ( w / ( numPoints - 1 ) ) * j );
		if ( min != max )
			points[ i ].y = y + h - 1 - ( ( values[ j ] - min ) * ( h / ( max - min ) ) );

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
	PlgDrawLines( border, PL_ARRAY_ELEMENTS( border ), PL_COLOUR_GOLD );
#endif

	PlgDrawLines( points, numOutPoints, PL_COLOUR_WHITE );

	BitmapFont *font = Font_GetDefaultSmall();
	Font_BeginDraw( font );

	if ( heading != NULL )
	{
		size_t len = strlen( heading );
		float cPos = ( x + w - ( len * font->cw ) ) - 2.0f;
		Font_AddBitmapStringToPass( font, cPos, y + 2.0f, 1.0f, PLColourRGB( 0, 255, 0 ), heading, len, false );
	}

	// Calculate the average sum of all the points
	double avg = 0.0;
	for ( unsigned int i = 0; i < numPoints; ++i )
		avg += values[ i ];
	avg /= numPoints;

	char buf[ 128 ];

	// Current and average readings
	snprintf( buf, sizeof( buf ), "CUR %02f", values[ numPoints - 1 ] );
	Font_AddBitmapStringToPass( font, x + 2.0f, y + ( h / 2 ) - ( font->ch / 2 ) + font->ch, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "AVG %02f", avg );
	Font_AddBitmapStringToPass( font, x + 2.0f, y + ( h / 2 ) - ( font->ch / 2 ) - font->ch, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "y+:%02f", max );
	Font_AddBitmapStringToPass( font, x + 2.0f, y + 2.0f, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "y-:%02f", min );
	Font_AddBitmapStringToPass( font, x + 2.0f, y + ( h - font->ch ) - 2.0f, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );

	Font_Draw( font );

	PlFree( points );
}

static void DrawDebugOverlay( const OgeViewport *viewport )
{
	PL_GET_CVAR( "debug.overlay", debugOverlay );
	if ( debugOverlay->i_value <= 0 )
	{
		return;
	}

	BitmapFont *defaultFont = Font_GetDefaultSmall();
	assert( defaultFont != NULL );
	if ( defaultFont == NULL )
	{
		return;
	}

	Font_BeginDraw( defaultFont );

	static const float sy = 8;
	static const float sx = 8;
	static const float tx = 8 + 4;
	float y               = sy;

	const OgeCamera *camera = viewport->camera;
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
		Font_AddBitmapStringToPass( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_WHITE, buf, strlen( buf ), false );
	}

	// Draw stats
	char buf[ 64 ];
	snprintf( buf, sizeof( buf ), "FPS:           " PL_FMT_uint32 "\n", YnCore_Viewport_GetAverageFPS( viewport ) );
	Font_AddBitmapStringToPass( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num faces:     " PL_FMT_uint32 "\n", g_gfxPerfStats.numFacesDrawn );
	Font_AddBitmapStringToPass( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num portals:   " PL_FMT_uint32 "\n", g_gfxPerfStats.numVisiblePortals );
	Font_AddBitmapStringToPass( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num triangles: " PL_FMT_uint32 "\n", g_gfxPerfStats.numTriangles );
	Font_AddBitmapStringToPass( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Num batches:   " PL_FMT_uint32 "\n", g_gfxPerfStats.numBatches );
	Font_AddBitmapStringToPass( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_GOLD, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Alloc memory:  %.2lfMB\n", PlBytesToMegabytes( PlGetTotalAllocatedMemory() ) );
	Font_AddBitmapStringToPass( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_ORCHID, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "Total memory:  %.2lfMB\n", PlBytesToMegabytes( PlGetCurrentMemoryUsage() ) );
	Font_AddBitmapStringToPass( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_ORCHID, buf, strlen( buf ), false );

	unsigned int numTasks = Sch_GetNumTasks();
	snprintf( buf, sizeof( buf ), "Num tasks:     " PL_FMT_uint32 "\n", numTasks );
	Font_AddBitmapStringToPass( defaultFont, tx, y += defaultFont->ch, 1.0f, PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	for ( unsigned int i = 0; i < numTasks; ++i )
	{
		double taskDelay;
		const char *taskDescription = Sch_GetTaskDescription( i, &taskDelay );
		snprintf( buf, sizeof( buf ), "%u %s\n", i, taskDescription );
		Font_AddBitmapStringToPass( defaultFont, tx + 8, y += defaultFont->ch, 1.0f, PL_COLOUR_MAGENTA, buf, strlen( buf ), false );
	}
	y += defaultFont->ch * 2;

	static const float bw = 128;

	PlgSetShaderProgram( oge_defaultShaderPrograms[ OGE_SHADER_DEFAULT_VERTEX ] );
	PlgSetBlendMode( PLG_BLEND_DEFAULT );
	PlgDrawRectangle( sx, sy, bw, y - sy, PLColour( 0, 0, 0, 200 ) );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	Font_Draw( defaultFont );

	if ( debugOverlay->i_value > 1 )
	{
		float x = sx;

		static const float graphHeight = 64;
		for ( unsigned int i = 0; i < MAX_PROFILER_GROUPS; ++i )
		{
			if ( y + graphHeight >= ( float ) viewport->height )
			{
				y = sy;
				x += bw;
			}

			uint8_t numPoints;
			const double *graph = Profiler_GetGraph( i, &numPoints );
			YR_DrawGraph( cpuProfilerDescriptions[ i ], x, y, bw, graphHeight, graph, numPoints, .0f, 1.0f );
			y += graphHeight;
		}
	}
}

void YnCore_Set2DViewportSize( int w, int h )
{
	PlgSetViewport( 0, 0, w, h );
	PlgSetupCamera( auxCamera );
}

void ogeGet2DViewportSize( int *width, int *height )
{
	PlgGetViewport( NULL, NULL, width, height );
}

void YnCore_DrawMenu( const OgeViewport *viewport )
{
	if ( viewport == NULL )
	{
		return;
	}

	YnCore_Set2DViewportSize( viewport->width, viewport->height );

	PlgSetDepthMask( false );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	DrawScenePost( viewport );

	//YRCamera camera;
	//PL_ZERO_( camera );
	//camera.internal = auxCamera;
	//YR_World_DrawWireframe( Game_GetCurrentWorld(), &camera );

	YnCore_DrawGUI( viewport );
	YnCore_DrawEditorGUI( viewport );

	DrawDebugOverlay( viewport );

	PlgSetTexture( NULL, 0 );

	PlPopMatrix();

	PlgSetDepthMask( true );
}

void YnCore_Draw2DQuad( OgeMaterial *material, int x, int y, int w, int h )
{
	static PLGMesh *mesh = NULL;
	if ( mesh == NULL )
	{
		mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_STRIP, PLG_DRAW_DYNAMIC, 2, 4 );
	}

	PlgClearMesh( mesh );

	PlgAddMeshVertex( mesh, &PlVector3( x, y + h, 0 ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &PlVector2( 0, 0 ) );
	PlgAddMeshVertex( mesh, &PlVector3( x, y, 0 ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &PlVector2( 0, 1 ) );
	PlgAddMeshVertex( mesh, &PlVector3( x + w, y + h, 0 ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &PlVector2( 1, 0 ) );
	PlgAddMeshVertex( mesh, &PlVector3( x + w, y, 0 ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &PlVector2( 1, 1 ) );

	ogeMaterial_DrawMesh( material, mesh, NULL, 0 );
}

void YnCore_DrawAxesPivot( PLVector3 position, PLVector3 rotation )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderProgram( oge_defaultShaderPrograms[ OGE_SHADER_DEFAULT_VERTEX ] );

	PLVector3 angles;
	angles.x = PL_DEG2RAD( rotation.x );
	angles.y = PL_DEG2RAD( rotation.y );
	angles.z = PL_DEG2RAD( rotation.z );

	PlRotateMatrix( angles.x, 1.0f, 0.0f, 0.0f );
	PlRotateMatrix( angles.y, 0.0f, 1.0f, 0.0f );
	PlRotateMatrix( angles.z, 0.0f, 0.0f, 1.0f );

	PlTranslateMatrix( position );

	PLMatrix4 transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	PlgDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 10, 0, 0 ), PLColour( 255, 0, 0, 255 ) );
	PlgDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 0, 10, 0 ), PLColour( 0, 255, 0, 255 ) );
	PlgDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 10 ), PLColour( 0, 0, 255, 255 ) );
	//printf( "%s\n", PlPrintVector3( &position, pl_int_var ) );

	PlPopMatrix();
}

static void YR_RenderScene( OgeCamera *camera, const OgeViewport *viewport )
{
	OgeWorldSector *currentSector = NULL;
	OgeWorld *world               = Game_GetCurrentWorld();
	if ( world != NULL )
	{
		currentSector = YnCore_World_GetSectorByGlobalOrigin( world, &camera->internal->position );
	}

	/* todo:	
		this needs to be restructured, as drawing is going to
		depend on whatever sector is currently being drawn.
		but for v2, this'll suffice...
	*/

	OGE_PROFILE_START( PROFILE_DRAW_WORLD );

	if ( viewport != NULL )
	{
		if ( camera->mode == YN_CORE_CAMERA_MODE_PERSPECTIVE )
		{
			if ( camera->drawMode == YN_CORE_CAMERA_DRAW_MODE_WIREFRAME )
			{
				YnCore_World_DrawWireframe( world, camera );
			}
			else
			{
				YnCore_World_Draw( world, currentSector, camera );
			}

			YNCoreEditorContext *editorInstance = YnCore_GetCurrentEditorContext();
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

				PlgSetShaderProgram( oge_defaultShaderPrograms[ OGE_SHADER_DEFAULT_VERTEX ] );
				PlgDrawDottedGrid( -( gridW / 2 ), -( gridW / 2 ), gridW, gridW, editorInstance->gridScale, &PL_COLOUR_BLUE );

				PlPopMatrix();
			}
		}
	}
	else
	{
		YnCore_World_Draw( world, currentSector, camera );
	}

	OGE_PROFILE_END( PROFILE_DRAW_WORLD );
}

static PLGTexture *colourTexture;
PLGTexture *ogeGetPrimaryColourAttachment( void )
{
	return colourTexture;
}

static PLGTexture *depthTexture;
PLGTexture *ogeGetPrimaryDepthAttachment( void )
{
	return depthTexture;
}

void YR_DrawScene( OgeCamera *camera, const OgeViewport *viewport )
{
	g_gfxPerfStats.cameraPos = camera->internal->position;

	// We're going to draw into a texture, so set that up first
	ogeSetupRenderTarget( &fboBuffer, &colourTexture, &depthTexture, viewport->width, viewport->height );
	PlgBindFrameBuffer( fboBuffer, PLG_FRAMEBUFFER_DRAW );

	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH | PLG_BUFFER_STENCIL );

	PL_GET_CVAR( "r.wireframe", wireframeMode );
	if ( ( camera != NULL && camera->drawMode == YN_CORE_CAMERA_DRAW_MODE_WIREFRAME ) || wireframeMode->b_value )
	{
		PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	YR_RenderScene( camera, viewport );

	if ( ( camera != NULL && camera->drawMode == YN_CORE_CAMERA_DRAW_MODE_WIREFRAME ) || wireframeMode->b_value )
	{
		PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );
}
