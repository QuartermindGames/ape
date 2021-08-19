/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plgraphics/plg_driver_interface.h>

#include "yin.h"
#include "actor.h"
#include "font.h"
#include "image.h"
#include "world.h"
#include "game_interface.h"
#include "renderer.h"
#include "particle.h"

#include "client/sgui.h"

RendererStats g_gfxPerfStats;

static PLGCamera *auxCamera = NULL;

#define SHADOW_MAP_RESOLUTION 2048
static PLGFrameBuffer *smDepthBuffer = NULL;
static PLGTexture *	   smTexture;
static PLGCamera *	   smCamera;

/* Post Processing */

static void GenerateScreenBuffer( PLGFrameBuffer **buffer, PLGTexture **attachment, unsigned int w, unsigned int h )
{
	unsigned int bw = 0, bh = 0;
	if ( *buffer != NULL )
		PlgGetFrameBufferResolution( *buffer, &bw, &bh );

	/* need to rebuild the framebuffer object
	 * todo: the library should provide us a func to perform a resize? */
	if ( bw != w || bh != h )
	{
		PlgDestroyFrameBuffer( *buffer );
		*buffer = PlgCreateFrameBuffer( w, h, PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
		if ( *buffer == NULL )
			PrintError( "Failed to create framebuffer!\nPL: %s\n", PlGetError() );

		PlgDestroyTexture( *attachment );
		*attachment = PlgGetFrameBufferTextureAttachment( *buffer, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR );
		if ( *attachment == NULL )
			PrintError( "Failed to create texture attachment!\nPL: %s\n", PlGetError() );
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

	PLMatrix4 transform = PlMatrix4Identity();
	PlgDrawTexturedRectangle( &transform, x, y, ( float ) numTextureTable[ digit ]->w, ( float ) numTextureTable[ digit ]->h, numTextureTable[ digit ] );
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

void R_SetupDefaultState( void )
{
	PlgSetClearColour( PLColour( 0, 0, 0, 255 ) ); //PLColour( 128, 212, 255, 255 ) );

	PlgEnableGraphicsState( PLG_GFX_STATE_SCISSORTEST );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgSetDepthMask( true );

	PlgSetCullMode( PLG_CULL_POSTIVE );

	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT ] );
}

static void R_SetupShadowMap( void )
{
	smDepthBuffer = PlgCreateFrameBuffer( SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, PLG_BUFFER_DEPTH );
	if ( smDepthBuffer == NULL )
		PrintError( "Failed to create depth buffer!\nPL: %s\n", PlGetError() );

	//glDrawBuffer( GL_NONE );

	smTexture = PlgGetFrameBufferTextureAttachment( smDepthBuffer, PLG_BUFFER_DEPTH, PLG_TEXTURE_FILTER_LINEAR );
	if ( smTexture == NULL )
		PrintError( "Failed to get texture attachment of depth buffer!\nPL: %s\n", PlGetError() );

	/* unbind the buffer we just created */
	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DEFAULT );

	smCamera			 = PlgCreateCamera();
	smCamera->viewport.w = SHADOW_MAP_RESOLUTION;
	smCamera->viewport.h = SHADOW_MAP_RESOLUTION;
}

void RS_InitializeShaderPrograms( void ); /* renderer/shaders.c */
void RT_InitializeTextures( void );		  /* texture.c */
void R_Initialize( void )
{
	Print( "Initializing renderer\n" );

	if ( PlgSetDriver( "vulkan" ) != PL_RESULT_SUCCESS &&
		 PlgSetDriver( "opengl" ) != PL_RESULT_SUCCESS &&
		 PlgSetDriver( "software" ) != PL_RESULT_SUCCESS )
		PrintError( "Failed to set graphics driver!\nPL: %s\n", PlGetError() );

	RT_InitializeTextures();
	RS_InitializeShaderPrograms();
	RM_InitializeMaterialSystem();

	Font_Initialize();

	auxCamera = PlgCreateCamera();
	if ( auxCamera == NULL )
		PrintError( "Failed to create auxiliary camera!\nPL: %s\n", PlGetError() );

	auxCamera->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
	auxCamera->near = 0.0f;
	auxCamera->far	= 1000.0f;

	R_SetupShadowMap();
	R_SetupDefaultState();
}

void R_Shutdown( void )
{
	Font_Shutdown();
	RM_ShutdownMaterialSystem();
}

static PLGFrameBuffer *auxBuffer		= NULL;
static PLGTexture *	   auxBufferTexture = NULL;

/**
 * Draw the auxillary buffer.
 */
static void R_DrawAuxBuffer( float x, float y, float w, float h )
{
}

static PLGFrameBuffer *ppBuffer		= NULL;
static PLGTexture *	   ppAttachment = NULL;

/**
 * Where the magic of post processing happens.
 */
static void R_DrawScreenBuffer( float x, float y, float w, float h )
{
	/* and now display the scene onto the screen */
	PlPushMatrix();
	PlLoadIdentityMatrix();

	CVar( "graphics.fxaa", fxaaMode );
	if ( fxaaMode->b_value )
	{
		PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_POST_PROCESS ] );
		PlgSetShaderUniformValue( defaultShaderPrograms[ RS_SHADER_POST_PROCESS ], "uViewportSize", &PLVector2( w, h ), false );
	}
	else
		PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT ] );

	/* todo: TEMP HACK HERE WITH SCALE, FIX UV COORDS!!!! */

	PlgDrawTexturedRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), x, y, w, h, ppAttachment );
	PlPopMatrix();
}

void R_DrawGraph( const char *heading, float x, float y, float w, float h, const float *values, unsigned int numPoints, float min, float max )
{
	if ( numPoints < 2 )
		return;

	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_VERTEX ] );

	float oa = min, ob = max;
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
	PLVector3 *	 points		  = globalSystem.CAlloc( numOutPoints, sizeof( PLVector3 ), true );

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

	PlgDrawRectangle( PlGetMatrix( PL_MODELVIEW_MATRIX ), x, y, w, h, PLColour( 0, 0, 0, 200 ) );
	PlgDrawLines( points, numOutPoints, PL_COLOUR_WHITE );

	BitmapFont *font = Font_GetDefaultSmall();
    size_t len	= strlen( heading );
    float  cPos = ( x + w - ( len * font->cw ) ) - 2.0f;
    Font_DrawBitmapString( font, cPos, y + 2.0f, 1.0f, 1.0f, PLColourRGB( 0, 255, 0 ), heading, true );

    /* metrics */
    char buf[ 128 ];
    snprintf( buf, sizeof( buf ),
              "min: %02f\n"
              "max: %02f\n"
              "cur: %02f",
              min, max,
              values[ numPoints - 1 ] );
    Font_DrawBitmapString( font, x + 2.0f, y + 2.0f, 1.0f, 1.0f, outOfBounds ? PL_COLOUR_RED : PL_COLOUR_GREEN, buf, false );

	globalSystem.Free( points );
}

static void R_DrawDebugOverlay( const PLGViewport *viewport )
{
	CVar( "debug.overlay", debugOverlay );
	if ( debugOverlay != NULL && debugOverlay->i_value <= 0 )
		return;

	float y = 0.0f;
	for ( uint8_t i = 0; i < MAX_PROFILER_GROUPS; ++i, y += 64.0f )
	{
		uint8_t		 numPoints;
		const float *graph = PF_GetGraph( i, &numPoints );
		R_DrawGraph( cpuProfilerDescriptions[ i ], viewport->w - 512.0f, y, 512.0f, 64.0f, graph, numPoints, -100.0f, 100.0f );
	}

	BitmapFont *defaultFont = Font_GetDefault();
	if ( defaultFont == NULL )
		return;

	Font_BeginDraw( defaultFont );

	static const char spinning[] = {
			'\\', '|', '/', '-', '/', '-' };
	static int pos = 0;
	Font_DrawBitmapCharacter( defaultFont, 2.0f, 2.0f, 1.0f, PLColourRGB( 0, 255, 0 ), spinning[ pos++ ] );
	if ( pos >= sizeof( spinning ) )
		pos = 0;

	char buf[ 256 ];
	snprintf( buf, sizeof( buf ),
			  "  Camera Position: %s\n"
			  "  Num Faces Drawn: " COM_FMT_uint32 "\n"
			  "  Num Batches:     " COM_FMT_uint32 "\n"
			  "  OS Memory Usage: %.2lfMB\n"
			  "  Internal Memory: %.2lfMB\n",
			  PlPrintVector3( &g_gfxPerfStats.cameraPos, pl_int_var ),
			  g_gfxPerfStats.numFacesDrawn,
			  g_gfxPerfStats.numBatches,
			  PlBytesToMegabytes( PlGetCurrentMemoryUsage() ),
			  PlBytesToMegabytes( globalSystem.GetInternalAllocatedMemory() ) );
	Font_DrawBitmapString( defaultFont, 2.0f, 16.0f, 1.0f, 1.0f, PL_COLOUR_WHITE, buf, true );

	/* print out details regarding running tasks */
	{
		y = 128.0f;
		Font_DrawBitmapString( defaultFont, 2.0f, y, 1.0f, 1.0f, PL_COLOUR_WHITE, "Task Manager\n===================", true );
		y += defaultFont->ch * 2;

		double		 taskDelay;
		unsigned int index	  = 0;
		const char * taskDesc = Sch_GetTaskDescription( index, &taskDelay );
		if ( taskDesc == NULL )
			Font_DrawBitmapString( defaultFont, 2.0f, y, 1.0f, 1.0f, PL_COLOUR_WHITE, "  No active tasks!", true );
		else
		{
			while ( taskDesc != NULL )
			{
				snprintf( buf, sizeof( buf ), "  %s : %lf", taskDesc, taskDelay - Engine_GetNumTicks() );
				Font_DrawBitmapString( defaultFont, 2.0f, y, 1.0f, 1.0f, PL_COLOUR_WHITE, buf, true );
				y += defaultFont->ch;
				taskDesc = Sch_GetTaskDescription( ++index, &taskDelay );
			}
		}
	}

	BitmapFont *smallFont = Font_GetDefaultSmall();
	Font_DrawBitmapString( smallFont, 2.0f, viewport->h - smallFont->ch - 2.0f, 1.0f, 1.0f, PLColourRGB( 0, 255, 0 ),
						   "v" ENGINE_VERSION_STR " [" GIT_BRANCH "." GIT_COMMIT_COUNT "]", true );
}

void R_DrawMenu( void )
{
	PROFILE_START( PROFILE_DRAW_UI );

	int w = globalSystem.viewport->w;
	int h = globalSystem.viewport->h;

	auxCamera->viewport.w = w;
	auxCamera->viewport.h = h;

	PlgSetupCamera( auxCamera );

	PlgSetDepthMask( false );

	R_DrawScreenBuffer( 0.0f, 0.0f, ( float ) w, ( float ) h );

	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT ] );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	//plDrawTexturedRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 0, h - demoOverlayLogo->h + 32, demoOverlayLogo->w, demoOverlayLogo->h, demoOverlayLogo );

	Menu_Draw( &auxCamera->viewport );
	R_DrawDebugOverlay( &auxCamera->viewport );
	Con_Draw( &auxCamera->viewport );

	PlPopMatrix();

	PlgSetDepthMask( true );

	PROFILE_END( PROFILE_DRAW_UI );

	memset( &g_gfxPerfStats, 0, sizeof( RendererStats ) );
}

void R_DrawAxesPivot( PLVector3 position, PLVector3 rotation )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_VERTEX ] );

	PLVector3 angles;
	angles.x = PlDegreesToRadians( rotation.x );
	angles.y = PlDegreesToRadians( rotation.y );
	angles.z = PlDegreesToRadians( rotation.z );

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

static void R_RenderScene( Camera *camera, bool smPass )
{
	WorldSector *curSector = NULL;
	Actor *		 player	   = Game_GetPlayer();
	if ( player != NULL )
		curSector = Act_GetWorldSector( player );

	/* todo:	
		this needs to be restructured, as drawing is going to
		depend on whatever sector is currently being drawn.
		but for v2, this'll suffice...
	*/

	W_Draw( Game_GetCurrentWorld(), curSector, camera );
	Act_DrawActors();

#if 0
	static PLVector3 rotation = PLVector3( 0.0f, 0.0f, 0.0f );
	R_DrawAxesPivot( PLVector3( 16.0f, -0.0f, 0.0f ), rotation );
	rotation.x += 0.5f;
	rotation.y += 0.5f;
	rotation.z += 0.5f;
#endif
}

static void R_RenderSceneDepth( Camera *camera, const PLVector3 *lightPos, const PLVector3 *lightAngles )
{
	smCamera->position = *lightPos;
	smCamera->angles   = *lightAngles;

	PlgSetupCamera( smCamera );
	PlgBindFrameBuffer( smDepthBuffer, PLG_FRAMEBUFFER_DRAW );
	PlgClearBuffers( PLG_BUFFER_DEPTH );

	R_RenderScene( camera, true );

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DEFAULT );
}

static void R_RenderSceneFinal( Camera *camera )
{
	/* set everything up for post-processing */
	//GenerateScreenBuffer( &ppBuffer, &ppAttachment, camera->internal->viewport.w, camera->internal->viewport.h );
	//PlgBindFrameBuffer( ppBuffer, PLG_FRAMEBUFFER_DRAW );

	PlgSetupCamera( camera->internal );
	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );

	R_RenderScene( camera, false );
}

void R_DrawScene( Camera *camera )
{
	g_gfxPerfStats.cameraPos = camera->internal->position;

//	R_RenderSceneDepth( camera, &PLVector3( 0, 128, -128 ), &PLVector3( 10, 128, 0 ) );

	CVar( "r_wireframe", wireframeMode );
	if ( wireframeMode->b_value ) {
        PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	R_RenderSceneFinal( camera );

	if ( wireframeMode->b_value ) {
        PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
	}

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );
}
