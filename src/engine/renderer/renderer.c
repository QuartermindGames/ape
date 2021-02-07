/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>
#include <3rdparty/platform/platform/3rdparty/glew-2.2.0/include/GL/glew.h>

#include "yin.h"
#include "actor.h"
#include "font.h"
#include "game.h"
#include "image.h"
#include "map.h"
#include "renderer.h"

/* config vars */
extern PLConsoleVariable *gVarGraphicsFXAA;

static PLCamera *auxCamera = NULL;

#define SHADOW_MAP_RESOLUTION 2048
static PLFrameBuffer *smDepthBuffer = NULL;
static PLTexture *smTexture;
static PLCamera *smCamera;

/* Post Processing */

static void GenerateScreenBuffer( PLFrameBuffer **buffer, PLTexture **attachment, unsigned int w, unsigned int h ) {
	unsigned int bw = 0, bh = 0;
	if ( *buffer != NULL ) {
		plGetFrameBufferResolution( *buffer, &bw, &bh );
	}

	/* need to rebuild the framebuffer object
	 * todo: the library should provide us a func to perform a resize? */
	if ( bw != w || bh != h ) {
		plDestroyFrameBuffer( *buffer );
		*buffer = plCreateFrameBuffer( w, h, PL_BUFFER_COLOUR | PL_BUFFER_DEPTH );
		if ( *buffer == NULL ) {
			PrintError( "Failed to create framebuffer!\nPL: %s\n", plGetError() );
		}

		plDestroyTexture( *attachment );
		*attachment = plGetFrameBufferTextureAttachment( *buffer, PL_BUFFER_COLOUR, PL_TEXTURE_FILTER_LINEAR );
		if ( *attachment == NULL ) {
			PrintError( "Failed to create texture attachment!\nPL: %s\n", plGetError() );
		}
	}

	/* reset */
	plBindFrameBuffer( NULL, PL_FRAMEBUFFER_DRAW );
}

typedef struct RGBMap {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGBMap;
static RGBMap playPal[ 256 ];

static PLLinkedList *textures;

static PLTexture *fallbackTexture = NULL;
static PLTexture *numTextureTable[ 10 ];
static PLTexture *demoOverlayLogo;

PLTexture *Gfx_GetFallbackTexture( void ) {
	return fallbackTexture;
}

PLTexture *Gfx_GenerateTextureFromData( uint8_t *data, unsigned int w, unsigned int h, unsigned int numChannels,
                                        bool generateMipMap ) {
	PLColourFormat cFormat;
	PLImageFormat iFormat;

	switch ( numChannels ) {
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

	PLImage *imageData = plCreateImage( data, w, h, cFormat, iFormat );
	if ( imageData == NULL ) {
		PrintWarn( "Failed to generate image data!\nPL: %s\n", plGetError() );
	}

#if 0
	char outName[ 64 ];
	snprintf( outName, sizeof( outName ), "test_%dx%d-%d.png", w, h, numChannels );
	plWriteImage( imageData, outName );
#endif

	PLTexture *texture = plCreateTexture();
	if ( texture == NULL ) {
		PrintError( "Failed to create texture!\nPL: %s\n", plGetError() );
	}

	if ( !generateMipMap ) {
		texture->flags &= PL_TEXTURE_FLAG_NOMIPS;
		texture->filter = PL_TEXTURE_FILTER_LINEAR;
	} else {
		texture->filter = PL_TEXTURE_FILTER_MIPMAP_LINEAR;
	}

	if ( !plUploadTextureImage( texture, imageData ) ) {
		PrintError( "Failed to generate texture from image!\nPL: %s\n", plGetError() );
	}

	plDestroyImage( imageData );

	return texture;
}

static void RT_InitializeTextures( void ) {
	textures = plCreateLinkedList();

	/* generate fallback texture */
	static PLColour fallbackData[] = {
	        { 128, 0, 128, 255 },
	        { 0, 128, 128, 255 },
	        { 0, 128, 128, 255 },
	        { 128, 0, 128, 255 },
	};
	fallbackTexture = Gfx_GenerateTextureFromData( ( uint8_t * ) fallbackData, 2, 2, 4, false );
	if ( fallbackTexture == NULL ) {
		PrintError( "Failed to create fallback texture!\n" );
	}

	/* register the standard image loaders, and our package image loader */
	plRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );
	plRegisterImageLoader( "gfx", Image_LoadPackedImage );

	/* fetch our required texture set */
	demoOverlayLogo = Gfx_LoadTexture( "materials/textures/ui/demo_logo.png" );

	/* load the numbers */
	/*
	for ( unsigned int i = 0; i < 10; ++i ) {
		char numName[ 16 ];
		snprintf( numName, sizeof( numName ), "WNUMBER%d", i );
		numTextureTable[ i ] = Gfx_LoadLumpTexture( titlePal, numName );
	}
	*/

	Font_Initialize();
}

PLTexture *Gfx_GetTexture( const char *path ) {
	PLLinkedListNode *node = plGetRootNode( textures );
	while ( node != NULL ) {
		PLTexture *texture = plGetLinkedListNodeUserData( node );
		if ( pl_strcasecmp( path, texture->path ) == 0 ) {
			return texture;
		}

		node = plGetNextLinkedListNode( node );
	}

	return NULL;
}

PLTexture *Gfx_LoadTexture( const char *path ) {
	/* check if it's already loaded */
	PLTexture *texture = Gfx_GetTexture( path );
	if ( texture != NULL ) {
		return texture;
	}

	texture = plLoadTextureFromImage( path, PL_TEXTURE_FILTER_MIPMAP_LINEAR );
	if ( texture == NULL ) {
		PrintWarn( "Failed to load texture \"%s\"!\nPL: %s\n", path, plGetError() );
		return fallbackTexture;
	}

	plInsertLinkedListNode( textures, texture );
	return texture;
}

/**********************************************************/
/** Shaders **/

typedef struct ShaderProgramIndex {
	char internalName[ GFX_PROGRAM_NAME_LENGTH ];
	PLShaderProgram *internalPtr;
	PLLinkedListNode *node;
} ShaderProgramIndex;

static PLLinkedList *gfxShaderPrograms;
PLShaderProgram *gfxDefaultShaderPrograms[ GFX_MAX_DEFAULT_SHADERS ];

static void Gfx_RegisterShaderStage( PLShaderProgram *program, PLShaderStageType type, const char *path ) {
	PLFile *filePtr = plOpenFile( path, true );
	if ( filePtr == NULL ) {
		PrintError( "Failed to find shader \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	const char *buffer = ( const char * ) plGetFileData( filePtr );
	size_t length = plGetFileSize( filePtr );

	if ( !plRegisterShaderStageFromMemory( program, buffer, length, type ) ) {
		PrintError( "Failed to register stage, \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	plCloseFile( filePtr );
}

static ShaderProgramIndex *Gfx_ParseShaderProgram( PLFile *file ) {
	ShaderProgramIndex program;

	char buffer[ 256 ];

	plReadString( file, buffer, sizeof( buffer ) );
	if ( sscanf( buffer, "program %s\n", program.internalName ) != 1 ) {
		PrintWarn( "Failed to fetch program name!\n" );
		return NULL;
	}

	char vertexPath[ PL_SYSTEM_MAX_PATH ];
	char fragmentPath[ PL_SYSTEM_MAX_PATH ];

	while ( plReadString( file, buffer, sizeof( buffer ) ) != NULL ) {
		if ( pl_strncasecmp( buffer, "vertex ", 7 ) == 0 ) {
			/* read in the vertex stage */
			sscanf( buffer, "vertex %s\n", vertexPath );
			continue;
		} else if ( pl_strncasecmp( buffer, "fragment ", 9 ) == 0 ) {
			/* read in the fragment stage */
			sscanf( buffer, "fragment %s\n", fragmentPath );
			continue;
		}
	}

	if ( vertexPath[ 0 ] == '\0' || fragmentPath[ 0 ] == '\0' ) {
		PrintWarn( "No vertex/fragment stage defined in program!\n" );
		return NULL;
	}

	program.internalPtr = plCreateShaderProgram();
	if ( program.internalPtr == NULL ) {
		PrintWarn( "Failed to create shader program!\nPL: %s\n", plGetError() );
		return NULL;
	}

	Gfx_RegisterShaderStage( program.internalPtr, PL_SHADER_TYPE_VERTEX, vertexPath );
	Gfx_RegisterShaderStage( program.internalPtr, PL_SHADER_TYPE_FRAGMENT, fragmentPath );

	if ( !plLinkShaderProgram( program.internalPtr ) ) {
		PrintError( "Failed to link shader stages!\nPL: %s\n", plGetError() );
	}

	/* allocate and return our program index */
	ShaderProgramIndex *out = Sys_malloc( sizeof( ShaderProgramIndex ) );
	*out = program;
	return out;
}

static void Gfx_LoadShaderProgram( const char *path, void *userData ) {
	PLFile *file = plOpenFile( path, false );
	if ( file == NULL ) {
		PrintWarn( "Failed to load shader program \"%s\"!\nPL: %s\n", path, plGetError() );
		return;
	}

	PrintMsg( "Parsing \"%s\"\n", path );
	ShaderProgramIndex *program = Gfx_ParseShaderProgram( file );

	plCloseFile( file );

	if ( program != NULL ) {
		program->node = plInsertLinkedListNode( gfxShaderPrograms, program );
	}
}

PLShaderProgram *Gfx_GetShaderProgram( const char *name ) {
	PLLinkedListNode *root = plGetRootNode( gfxShaderPrograms );
	while ( root != NULL ) {
		ShaderProgramIndex *programIndex = plGetLinkedListNodeUserData( root );
		if ( strcmp( name, programIndex->internalName ) == 0 ) {
			return programIndex->internalPtr;
		}

		root = plGetNextLinkedListNode( root );
	}

	return NULL;
}

static void Gfx_InitializeShaderPrograms( void ) {
	gfxShaderPrograms = plCreateLinkedList();

	plScanDirectory( "materials/shaders/", "prg", Gfx_LoadShaderProgram, false, NULL );

	PrintMsg( "%d shader programs indexed\n", plGetNumLinkedListNodes( gfxShaderPrograms ) );

	/* now fetch the default programs */
	const char *defaultShaderNames[ GFX_MAX_DEFAULT_SHADERS ] = {
	        [GFX_SHADER_DEFAULT] = "default",
	        [GFX_SHADER_LIGHTING_PASS] = "base_lighting",
	        [GFX_SHADER_DEFAULT_VERTEX] = "default_vertex",
	        [GFX_SHADER_DEFAULT_ALPHA] = "default_alpha",
	        [GFX_SHADER_POST_PROCESS] = "postprocess",
	};
	for ( unsigned int i = 0; i < GFX_MAX_DEFAULT_SHADERS; ++i ) {
		gfxDefaultShaderPrograms[ i ] = Gfx_GetShaderProgram( defaultShaderNames[ i ] );
		if ( gfxDefaultShaderPrograms[ i ] == NULL ) {
			PrintError( "Failed to find default shader program, \"%s\"!\n", defaultShaderNames[ i ] );
		}
	}
}

/**********************************************************/

void Gfx_DrawAnimationFrame( GfxAnimationFrame *frame, const PLVector3 *position, float spriteAngle ) {
#if 0
	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();

	plLoadIdentityMatrix();

	plRotateMatrix( plDegreesToRadians( 0.0f ), 1.0f, 0.0f, 0.0f );
	plRotateMatrix( plDegreesToRadians( spriteAngle ), 0.0f, 1.0f, 0.0f );
	plRotateMatrix( plDegreesToRadians( 180.0f ), 0.0f, 0.0f, 1.0f );

	plPopMatrix();

	plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT_LIT ] );

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

void Gfx_DrawAnimation( GfxAnimationFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle ) {
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

void Gfx_DrawDigit( float x, float y, int digit ) {
	if ( digit < 0 ) {
		digit = 0;
	} else if ( digit > 9 ) {
		digit = 9;
	}

	PLMatrix4 transform = plMatrix4Identity();
	plDrawTexturedRectangle( &transform, x, y, ( float ) numTextureTable[ digit ]->w, ( float ) numTextureTable[ digit ]->h, numTextureTable[ digit ] );
}

void Gfx_DrawNumber( float x, float y, unsigned int number ) {
	/* restrict it to 999 for sanity */
	if ( number > 999 ) { number = 999; }

	if ( number >= 100 ) {
		int digit = number / 100;
		Gfx_DrawDigit( x, y, digit );
		x += ( signed ) numTextureTable[ digit ]->w + 1;
	}

	if ( number >= 10 ) {
		int digit = ( number / 10 ) % 10;
		Gfx_DrawDigit( x, y, digit );
		x += ( signed ) numTextureTable[ digit ]->w + 1;
	}

	Gfx_DrawDigit( x, y, number % 10 );
}

void Gfx_InitializeCameras( void ); /* gfx_camera.c */
void Gfx_ShutdownCameras( void );   /* gfx_camera.c */

void Gfx_SetupDefaultState( void ) {
	plSetClearColour( PLColour( 128, 212, 255, 255 ) );

	plEnableGraphicsState( PL_GFX_STATE_SCISSORTEST );

	plSetDepthBufferMode( PL_DEPTHBUFFER_ENABLE );
	plSetDepthMask( true );

	plSetCullMode( PL_CULL_POSTIVE );

	plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT ] );
}

static void Gfx_SetupShadowMap( void ) {
	smDepthBuffer = plCreateFrameBuffer( SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, PL_BUFFER_DEPTH );
	if ( smDepthBuffer == NULL ) {
		PrintError( "Failed to create depth buffer!\nPL: %s\n", plGetError() );
	}

	glDrawBuffer( GL_NONE );

	smTexture = plGetFrameBufferTextureAttachment( smDepthBuffer, PL_BUFFER_DEPTH, PL_TEXTURE_FILTER_LINEAR );
	if ( smTexture == NULL ) {
		PrintError( "Failed to get texture attachment of depth buffer!\nPL: %s\n", plGetError() );
	}

	/* unbind the buffer we just created */
	plBindFrameBuffer( NULL, PL_FRAMEBUFFER_DEFAULT );

	smCamera = plCreateCamera();
	smCamera->viewport.w = SHADOW_MAP_RESOLUTION;
	smCamera->viewport.h = SHADOW_MAP_RESOLUTION;
}

void Gfx_Initialize( void ) {
	PrintMsg( "Initializing Gfx...\n" );

	plSetGraphicsMode( PL_GFX_MODE_OPENGL_CORE );

	/* create both the interface camera and player camera */

	Gfx_InitializeShaderPrograms();

	RT_InitializeTextures();
	RM_InitializeMaterialSystem();

	Gfx_InitializeCameras();

	auxCamera = plCreateCamera();
	if ( auxCamera == NULL ) {
		PrintError( "Failed to create auxiliary camera!\nPL: %s\n", plGetError() );
	}
	auxCamera->mode = PL_CAMERA_MODE_ORTHOGRAPHIC;
	auxCamera->near = 0.0f;
	auxCamera->far = 1000.0f;

	Gfx_SetupShadowMap();
	Gfx_SetupDefaultState();
}

void Gfx_Shutdown( void ) {
	Gfx_ShutdownCameras();
	Font_Shutdown();
	RM_ShutdownMaterialSystem();
}

static void Gfx_DrawViewSprite( void ) {
#if 0
	int gunWidth = 320 / 1.5;
	int gunHeight = 200 / 1.5;
	plDrawTexturedRectangle(
		YIN_DISPLAY_WIDTH / 2 - ( gunWidth / 2 ),
		YIN_DISPLAY_HEIGHT - gunHeight,
		gunWidth, gunHeight,
		testSprite );
#endif
}

static PLFrameBuffer *ppBuffer = NULL;
static PLTexture *ppAttachment = NULL;

void Gfx_DrawMenu( void ) {
	SysWindow *window = Engine_GetMainWindow();
	if ( window == NULL ) {
		return;
	}

	int w, h;
	g_system.GetWindowSize( window, &w, &h );
	auxCamera->viewport.w = w;
	auxCamera->viewport.h = h;

	plSetupCamera( auxCamera );

	plSetDepthMask( false );

	/* and now display the scene onto the screen */
	plPushMatrix();
	plLoadIdentityMatrix();
	if ( gVarGraphicsFXAA->b_value ) {
		plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_POST_PROCESS ] );
		plSetShaderUniformValue( gfxDefaultShaderPrograms[ GFX_SHADER_POST_PROCESS ], "uViewportSize", &PLVector2( w, h ), false );
	} else {
		plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT ] );
	}
	/* todo: TEMP HACK HERE WITH SCALE, FIX UV COORDS!!!! */
	plDrawTexturedRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), w, h, -w, -h, ppAttachment );
	plPopMatrix();

#ifndef DEBUG_CAM
	switch ( Game_GetMenuState() ) {
		default:
			PrintError( "Invalid menu state!\n" );
		case MENU_STATE_START:
			plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT ] );
			//plDrawTexturedRectangle( &transform, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, titlePicTexture );
			break;
		case MENU_STATE_HUD:
			plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT_ALPHA ] );
			Gfx_DrawViewSprite();
			break;
	}
#endif

	plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT ] );
	plSetBlendMode( PL_BLEND_DEFAULT );

	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();

	plLoadIdentityMatrix();

	plDrawTexturedRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 0, h - demoOverlayLogo->h + 32, demoOverlayLogo->w, demoOverlayLogo->h, demoOverlayLogo );

	plPopMatrix();

	static const char spinning[] = {
	        '\\', '|', '/', '-', '/', '-' };
	static int pos = 0;
	Font_DrawBitmapCharacter( 2.0f, 2.0f, 1.0f, PLColourRGB( 0, 255, 0 ), spinning[ pos++ ] );
	if ( pos >= sizeof( spinning ) ) {
		pos = 0;
	}

	char buf[ 256 ];
	snprintf( buf, sizeof( buf ),
	          "Position:        %d %d %d\n"
	          "Current Node:    0\n"
	          "Num Faces Drawn: %d\n"
	          "Num Batches:     %d\n"
	          "Map Draw Time:   %lf\n",
	          ( int ) g_gfxPerfStats.cameraPos.x,
	          ( int ) g_gfxPerfStats.cameraPos.y,
	          ( int ) g_gfxPerfStats.cameraPos.z,
	          g_gfxPerfStats.numFacesDrawn,
	          g_gfxPerfStats.numBatches,
	          CPUTimer_GetMeasure( PROFILE_DRAW_MAP ) );
	Font_DrawBitmapString( 2.0f, 16.0f, 1.0f, 1.0f, PLColourRGB( 0, 255, 0 ), buf, false );

	plSetBlendMode( PL_BLEND_DISABLE );

	Con_Draw( &auxCamera->viewport );

	plSetDepthMask( true );
}

void Gfx_DrawAxesPivot( PLVector3 position, PLVector3 rotation ) {
	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();

	plLoadIdentityMatrix();

	PLVector3 angles;
	angles.x = plDegreesToRadians( rotation.x );
	angles.y = plDegreesToRadians( rotation.y );
	angles.z = plDegreesToRadians( rotation.z );

	plRotateMatrix( angles.x, 1.0f, 0.0f, 0.0f );
	plRotateMatrix( angles.y, 0.0f, 1.0f, 0.0f );
	plRotateMatrix( angles.z, 0.0f, 0.0f, 1.0f );

	plTranslateMatrix( position );

	PLMatrix4 transform = *plGetMatrix( PL_MODELVIEW_MATRIX );
	plDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 10, 0, 0 ), PLColour( 255, 0, 0, 255 ) );
	plDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 0, 10, 0 ), PLColour( 0, 255, 0, 255 ) );
	plDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, 10 ), PLColour( 0, 0, 255, 255 ) );
	//printf( "%s\n", plPrintVector3( &position, pl_int_var ) );

	plPopMatrix();
}

static void Gfx_RenderScene( PLCamera *camera, bool smPass ) {
	Map_Draw( camera, smPass );
	Act_DrawActors();
}

static void Gfx_RenderSceneDepth( PLCamera *camera, const PLVector3 *lightPos, const PLVector3 *lightAngles ) {
	smCamera->position = *lightPos;
	smCamera->angles = *lightAngles;

	plSetupCamera( smCamera );
	plBindFrameBuffer( smDepthBuffer, PL_FRAMEBUFFER_DRAW );
	plClearBuffers( PL_BUFFER_DEPTH );

	Gfx_RenderScene( camera, true );

	plBindFrameBuffer( NULL, PL_FRAMEBUFFER_DEFAULT );
}

static void Gfx_RenderSceneFinal( PLCamera *camera ) {
	/* set everything up for post-processing */
	GenerateScreenBuffer( &ppBuffer, &ppAttachment, camera->viewport.w, camera->viewport.h );
	plBindFrameBuffer( ppBuffer, PL_FRAMEBUFFER_DRAW );

	plSetupCamera( camera );
	plClearBuffers( PL_BUFFER_COLOUR | PL_BUFFER_DEPTH );

	Gfx_RenderScene( camera, false );
}

void Gfx_DrawScene( PLCamera *camera ) {
	g_gfxPerfStats.cameraPos = camera->position;

	Gfx_RenderSceneDepth( camera, &PLVector3( 0, 128, -128 ), &PLVector3( 10, 128, 0 ) );

	CVar( "graphics.wireframe", wireframeMode );
	if ( wireframeMode->b_value ) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}

	Gfx_RenderSceneFinal( camera );

	if ( wireframeMode->b_value ) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	plBindFrameBuffer( NULL, PL_FRAMEBUFFER_DRAW );
}
