/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 OldTimes Software
 * ====================================================================*/

#include <plgraphics/plg_driver_interface.h>

#include "yin.h"
#include "actor.h"
#include "font.h"
#include "engine/image.h"
#include "map.h"
#include "renderer.h"

RendererStats g_gfxPerfStats;

static PLGCamera *auxCamera = NULL;

#define SHADOW_MAP_RESOLUTION 2048
static PLGFrameBuffer *smDepthBuffer = NULL;
static PLGTexture *    smTexture;
static PLGCamera *     smCamera;

#define NUM_GRAPH_POINTS 32
static float msWorldGraph[ NUM_GRAPH_POINTS ];
static float memoryGraph[ NUM_GRAPH_POINTS ];

/* Post Processing */

static void GenerateScreenBuffer( PLGFrameBuffer **buffer, PLGTexture **attachment, unsigned int w, unsigned int h )
{
	unsigned int bw = 0, bh = 0;
	if ( *buffer != NULL )
	{
		PlgGetFrameBufferResolution( *buffer, &bw, &bh );
	}

	/* need to rebuild the framebuffer object
	 * todo: the library should provide us a func to perform a resize? */
	if ( bw != w || bh != h )
	{
		PlgDestroyFrameBuffer( *buffer );
		*buffer = PlgCreateFrameBuffer( w, h, PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
		if ( *buffer == NULL )
		{
			PrintError( "Failed to create framebuffer!\nPL: %s\n", PlGetError() );
		}

		PlgDestroyTexture( *attachment );
		*attachment = PlgGetFrameBufferTextureAttachment( *buffer, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR );
		if ( *attachment == NULL )
		{
			PrintError( "Failed to create texture attachment!\nPL: %s\n", PlGetError() );
		}
	}

	/* reset */
	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );
}

typedef struct RGBMap
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGBMap;
static RGBMap playPal[ 256 ];

static PLLinkedList *textures;

static PLGTexture *fallbackTexture = NULL;
static PLGTexture *numTextureTable[ 10 ];

PLGTexture *R_GetFallbackTexture( void )
{
	return fallbackTexture;
}

PLGTexture *Gfx_GenerateTextureFromData( uint8_t *data, unsigned int w, unsigned int h, unsigned int numChannels,
                                         bool generateMipMap )
{
	PLColourFormat cFormat;
	PLImageFormat  iFormat;

	switch ( numChannels )
	{
		default:
			PrintWarn( "Invalid number of colour channels specified!\n" );
			return NULL;
		case 3:
			cFormat = PL_COLOURFORMAT_RGB;
			iFormat = PL_IMAGEFORMAT_RGB8;
			break;
		case 4:
			cFormat = PL_COLOURFORMAT_RGBA;
			iFormat = PL_IMAGEFORMAT_RGBA8;
			break;
	}

	PLImage *imageData = PlCreateImage( data, w, h, cFormat, iFormat );
	if ( imageData == NULL )
	{
		PrintWarn( "Failed to generate image data!\nPL: %s\n", PlGetError() );
	}

#if 0
	char outName[ 64 ];
	snprintf( outName, sizeof( outName ), "test_%dx%d-%d.png", w, h, numChannels );
	plWriteImage( imageData, outName );
#endif

	PLGTexture *texture = PlgCreateTexture();
	if ( texture == NULL )
	{
		PrintError( "Failed to create texture!\nPL: %s\n", PlGetError() );
	}

	if ( !generateMipMap )
	{
		texture->flags &= PLG_TEXTURE_FLAG_NOMIPS;
		texture->filter = PLG_TEXTURE_FILTER_LINEAR;
	}
	else
	{
		texture->filter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;
	}

	if ( !PlgUploadTextureImage( texture, imageData ) )
	{
		PrintError( "Failed to generate texture from image!\nPL: %s\n", PlGetError() );
	}

	PlDestroyImage( imageData );

	return texture;
}

static void RT_InitializeTextures( void )
{
	textures = PlCreateLinkedList();

	/* generate fallback texture */
	static PLColour fallbackData[] = {
	        { 128, 0, 128, 255 },
	        { 0, 128, 128, 255 },
	        { 0, 128, 128, 255 },
	        { 128, 0, 128, 255 },
	};
	fallbackTexture = Gfx_GenerateTextureFromData( ( uint8_t * ) fallbackData, 2, 2, 4, false );
	if ( fallbackTexture == NULL )
	{
		PrintError( "Failed to create fallback texture!\n" );
	}

	/* register the standard image loaders, and our package image loader */
	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );
	PlRegisterImageLoader( "gfx", Image_LoadPackedImage );

	/* load the numbers */
	/*
	for ( unsigned int i = 0; i < 10; ++i ) {
		char numName[ 16 ];
		snprintf( numName, sizeof( numName ), "WNUMBER%d", i );
		numTextureTable[ i ] = Gfx_LoadLumpTexture( titlePal, numName );
	}
	*/
}

PLGTexture *Gfx_GetTexture( const char *path )
{
	PLLinkedListNode *node = PlGetFirstNode( textures );
	while ( node != NULL )
	{
		PLGTexture *texture = PlGetLinkedListNodeUserData( node );
		if ( pl_strcasecmp( path, texture->path ) == 0 )
		{
			return texture;
		}

		node = PlGetNextLinkedListNode( node );
	}

	return NULL;
}

PLGTexture *R_LoadTexture( const char *path )
{
	/* check if it's already loaded */
	PLGTexture *texture = Gfx_GetTexture( path );
	if ( texture != NULL )
	{
		return texture;
	}

	texture = PlgLoadTextureFromImage( path, PLG_TEXTURE_FILTER_MIPMAP_LINEAR );
	if ( texture == NULL )
	{
		PrintWarn( "Failed to load texture \"%s\"!\nPL: %s\n", path, PlGetError() );
		return fallbackTexture;
	}

	PlInsertLinkedListNode( textures, texture );
	return texture;
}

/**********************************************************/

void RSpr_DrawAnimationFrame( SprAnimationFrame *frame, const PLVector3 *position, float spriteAngle )
{
#if 0
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlRotateMatrix( PlDegreesToRadians( 0.0f ), 1.0f, 0.0f, 0.0f );
	PlRotateMatrix( PlDegreesToRadians( spriteAngle ), 0.0f, 1.0f, 0.0f );
	PlRotateMatrix( PlDegreesToRadians( 180.0f ), 0.0f, 0.0f, 1.0f );

	PlPopMatrix();

	PlgSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT_LIT ] );

#if 0
	int w = frame->texture->w; //* 1.7;
	int h = frame->texture->h; //* 1.7;
	int x = -frame->leftOffset;
	int y = -frame->topOffset;
#else /* for the sake of time, let's botch it! */
	float w = frame->texture->w * 1.7f;
	float h = frame->texture->h * 1.7f;
	float x = -( w / 2.0f );
	float y = -h;
#endif

	plDrawTexturedRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), x, y, w, h, frame->texture );
#endif
}

void RSpr_DrawAnimation( SprAnimationFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle )
{
#if 0
	const GfxCamera *camera = Gfx_GetCurrentCamera();
	if( camera == NULL ) {
		return;
	}

	/* here we go, dumb maths written by dumb me... */
	PLVector2 a = PLVector2( position->x, position->z );
	PLVector2 b = PLVector2( camera->cameraPtr->position.x, camera->cameraPtr->position.z );
	PLVector2 normal = plComputeLineNormal( &a, &b );

	float spriteAngle = atan2f( normal.y, normal.x ) * PL_180_DIV_PI;

	/* should really account for the intended angle, but hey ho...
	 * there are some further improvements to make here - again, running out of time! */
	unsigned int frameColumn = 1;
	float frameAngle = spriteAngle < 0 ? spriteAngle + 360 : spriteAngle;
	if( frameAngle > 315.0f ) {
		frameColumn = 2;
	} else if( frameAngle > 270.0f ) {
		frameColumn = 3;
	} else if( frameAngle > 225.0f ) {
		frameColumn = 4;
	} else if( frameAngle > 180.0f ) {
		frameColumn = 5;
	} else if( frameAngle > 135.0f ) {
		frameColumn = 6;
	} else if( frameAngle > 90.0f ) {
		frameColumn = 7;
	} else if( frameAngle > 45.0f ) {
		frameColumn = 8;
	}

	curFrame *= GFX_NUM_SPRITE_ANGLES;
	unsigned int actualFrame = curFrame + ( frameColumn - 1 );
	if( actualFrame > numFrames ) {
		PrintWarn( "Out of scope frame, %d/%d!\n", actualFrame, numFrames );
		actualFrame = numFrames;
	}

	Gfx_DrawAnimationFrame( animation[ actualFrame ], position, spriteAngle );
#endif
}

void Gfx_DrawDigit( float x, float y, int digit )
{
	if ( digit < 0 )
		digit = 0;
	else if ( digit > 9 )
		digit = 9;

	PLMatrix4 transform = PlMatrix4Identity();
	PlgDrawTexturedRectangle( &transform, x, y, ( float ) numTextureTable[ digit ]->w, ( float ) numTextureTable[ digit ]->h, numTextureTable[ digit ] );
}

void Gfx_DrawNumber( float x, float y, unsigned int number )
{
	/* restrict it to 999 for sanity */
	if ( number > 999 ) { number = 999; }

	if ( number >= 100 )
	{
		int digit = number / 100;
		Gfx_DrawDigit( x, y, digit );
		x += ( signed ) numTextureTable[ digit ]->w + 1;
	}

	if ( number >= 10 )
	{
		int digit = ( number / 10 ) % 10;
		Gfx_DrawDigit( x, y, digit );
		x += ( signed ) numTextureTable[ digit ]->w + 1;
	}

	Gfx_DrawDigit( x, y, number % 10 );
}

void Gfx_InitializeCameras( void ); /* gfx_camera.c */
void Gfx_ShutdownCameras( void );   /* gfx_camera.c */

void R_SetupDefaultState( void )
{
	PlgSetClearColour( PLColour( 128, 212, 255, 255 ) );

	PlgEnableGraphicsState( PLG_GFX_STATE_SCISSORTEST );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgSetDepthMask( true );

	PlgSetCullMode( PLG_CULL_POSTIVE );

	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT ] );
}

static void Gfx_SetupShadowMap( void )
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

	smCamera             = PlgCreateCamera();
	smCamera->viewport.w = SHADOW_MAP_RESOLUTION;
	smCamera->viewport.h = SHADOW_MAP_RESOLUTION;
}

void RS_InitializeShaderPrograms( void ); /* renderer/shaders.c */
void R_Initialize( void )
{
	Print( "Initializing renderer\n" );

	if ( PlgSetDriver( "vulkan" ) != PL_RESULT_SUCCESS &&
	     PlgSetDriver( "opengl" ) != PL_RESULT_SUCCESS &&
	     PlgSetDriver( "software" ) != PL_RESULT_SUCCESS )
		PrintError( "Failed to set graphics driver!\nPL: %s\n", PlGetError() );

	memset( msWorldGraph, 0, sizeof( float ) * NUM_GRAPH_POINTS );
	memset( memoryGraph, 0, sizeof( float ) * NUM_GRAPH_POINTS );

	/* create both the interface camera and player camera */

	RT_InitializeTextures();
	RS_InitializeShaderPrograms();
	RM_InitializeMaterialSystem();

	Font_Initialize();

	Gfx_InitializeCameras();

	auxCamera = PlgCreateCamera();
	if ( auxCamera == NULL )
		PrintError( "Failed to create auxiliary camera!\nPL: %s\n", PlGetError() );

	auxCamera->mode = PLG_CAMERA_MODE_ORTHOGRAPHIC;
	auxCamera->near = 0.0f;
	auxCamera->far  = 1000.0f;

	Gfx_SetupShadowMap();
	R_SetupDefaultState();
}

void R_Shutdown( void )
{
	Gfx_ShutdownCameras();
	Font_Shutdown();
	RM_ShutdownMaterialSystem();
}

static PLGFrameBuffer *ppBuffer     = NULL;
static PLGTexture *    ppAttachment = NULL;

/**
 * Where the magic of post processing happens.
 */
static void R_DrawScreenBuffer( int x, int y, int w, int h )
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

void R_DrawGraph( const char *heading, float x, float y, float w, float h, float *values, unsigned int numPoints, float min, float max )
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
	PLVector3 *  points       = globalSystem.CAlloc( numOutPoints, sizeof( PLVector3 ), true );

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

	BitmapFont *font = Font_GetDefault();
	if ( font != NULL )
	{
		size_t len  = strlen( heading );
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
	}

	globalSystem.Free( points );
}

void R_DrawMenu( void )
{
	int w = globalSystem.viewport->w;
	int h = globalSystem.viewport->h;

	auxCamera->viewport.w = w;
	auxCamera->viewport.h = h;

	PlgSetupCamera( auxCamera );

	PlgSetDepthMask( false );

	R_DrawScreenBuffer( 0, 0, w, h );

	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT ] );
	PlgSetBlendMode( PLG_BLEND_DEFAULT );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	//plDrawTexturedRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 0, h - demoOverlayLogo->h + 32, demoOverlayLogo->w, demoOverlayLogo->h, demoOverlayLogo );

	CVar( "debug.overlay", debugOverlay );
	if ( debugOverlay->i_value > 0 )
	{
		for ( unsigned int i = 0; i < NUM_GRAPH_POINTS - 1; ++i ) msWorldGraph[ i ] = msWorldGraph[ i + 1 ];
		msWorldGraph[ NUM_GRAPH_POINTS - 1 ] = CPUTimer_GetMeasure( PROFILE_DRAW_MAP );
		R_DrawGraph( PL_TOSTRING( PROFILE_DRAW_MAP ), w - 512.0f, 0.0f, 512.0f, 64.0f, msWorldGraph, NUM_GRAPH_POINTS, 0.0f, 5.0f );

		for ( unsigned int i = 0; i < NUM_GRAPH_POINTS - 1; ++i ) memoryGraph[ i ] = memoryGraph[ i + 1 ];
		memoryGraph[ NUM_GRAPH_POINTS - 1 ] = PlBytesToMegabytes( PlGetCurrentMemoryUsage() );
		R_DrawGraph( PL_TOSTRING( MEMORY_USAGE ), w - 512.0f, 64.0f, 512.0f, 64.0f, memoryGraph, NUM_GRAPH_POINTS, 0.0f, PlBytesToMegabytes( PlGetTotalSystemMemory() ) );

		BitmapFont *defaultFont = Font_GetDefault();
		if ( defaultFont != NULL )
		{
			static const char spinning[] = {
			        '\\', '|', '/', '-', '/', '-' };
			static int pos = 0;
			Font_DrawBitmapCharacter( defaultFont, 2.0f, 2.0f, 1.0f, PLColourRGB( 0, 255, 0 ), spinning[ pos++ ] );
			if ( pos >= sizeof( spinning ) )
				pos = 0;

			char buf[ 256 ];
			snprintf( buf, sizeof( buf ),
			          "Rendering State\n"
			          "===================\n"
			          "  Camera Position: %s\n"
			          "  Num Faces Drawn: %d\n"
			          "  Num Batches:     %d\n",
			          PlPrintVector3( &g_gfxPerfStats.cameraPos, pl_int_var ),
			          g_gfxPerfStats.numFacesDrawn,
			          g_gfxPerfStats.numBatches );
			Font_DrawBitmapString( defaultFont, 2.0f, 16.0f, 1.0f, 1.0f, PL_COLOUR_WHITE, buf, true );

			/* print out details regarding running tasks */
			{
				float y = 128.0f;
				Font_DrawBitmapString( defaultFont, 2.0f, y, 1.0f, 1.0f, PL_COLOUR_WHITE, "Tasks\n===================", true );
				y += defaultFont->ch * 2;

				double       taskDelay;
				unsigned int index    = 0;
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

			Font_DrawBitmapString( defaultFont, 2.0f, h - defaultFont->ch - 2, 1.0f, 1.0f, PLColourRGB( 0, 255, 0 ),
			                       "v" ENGINE_VERSION_STR " [" GIT_BRANCH "." GIT_COMMIT_COUNT "]", true );
		}
	}

	PlgSetBlendMode( PLG_BLEND_DISABLE );

	PlPopMatrix();

	Con_Draw( &auxCamera->viewport );

	PlgSetDepthMask( true );

	memset( &g_gfxPerfStats, 0, sizeof( RendererStats ) );
}

void R_DrawAxesPivot( PLVector3 position, PLVector3 rotation )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

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

static void Gfx_RenderScene( PLGCamera *camera, bool smPass )
{
	Map_Draw( camera, smPass );
	Act_DrawActors();
}

static void Gfx_RenderSceneDepth( PLGCamera *camera, const PLVector3 *lightPos, const PLVector3 *lightAngles )
{
	smCamera->position = *lightPos;
	smCamera->angles   = *lightAngles;

	PlgSetupCamera( smCamera );
	PlgBindFrameBuffer( smDepthBuffer, PLG_FRAMEBUFFER_DRAW );
	PlgClearBuffers( PLG_BUFFER_DEPTH );

	Gfx_RenderScene( camera, true );

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DEFAULT );
}

static void Gfx_RenderSceneFinal( PLGCamera *camera )
{
	/* set everything up for post-processing */
	GenerateScreenBuffer( &ppBuffer, &ppAttachment, camera->viewport.w, camera->viewport.h );
	PlgBindFrameBuffer( ppBuffer, PLG_FRAMEBUFFER_DRAW );

	PlgSetupCamera( camera );
	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );

	Gfx_RenderScene( camera, false );
}

void Gfx_DrawScene( PLGCamera *camera )
{
	g_gfxPerfStats.cameraPos = camera->position;

	Gfx_RenderSceneDepth( camera, &PLVector3( 0, 128, -128 ), &PLVector3( 10, 128, 0 ) );

	CVar( "graphics.wireframeMode", wireframeMode );
	//if ( wireframeMode->b_value ) {
	//	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	//}

	Gfx_RenderSceneFinal( camera );

	//if ( wireframeMode->b_value ) {
	//	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	//}

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DRAW );
}
