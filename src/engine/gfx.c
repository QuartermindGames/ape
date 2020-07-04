/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "gfx.h"
#include "game.h"
#include "act.h"
#include "map.h"
#include "packed_image.h"

static PLShaderProgram *shaderPrograms[MAX_SHADER_TYPES];

static PLCamera *auxCamera = NULL;

typedef struct RGBMap {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGBMap;
static RGBMap playPal[256], titlePal[256];

static PLTexture *fallbackTexture = NULL;
static PLTexture *titlePicTexture = NULL;

static PLTexture *numTextureTable[10];
static PLTexture *defaultFont;

static GfxAnimationFrame **wallTextures;
static unsigned int      numWallTextures;
static PLTexture         **floorTextures;
static unsigned int      numFloorTextures;

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
	if ( imageData == NULL) {
		PrintWarn( "Failed to generate image data!\nPL: %s\n", plGetError());
	}

#if 0
	char outName[ 64 ];
	snprintf( outName, sizeof( outName ), "test_%dx%d-%d.png", w, h, numChannels );
	plWriteImage( imageData, outName );
#endif

	PLTexture *texture = plCreateTexture();
	if ( texture == NULL) {
		PrintError( "Failed to create texture!\nPL: %s\n", plGetError());
	}

	if ( !generateMipMap ) {
		texture->flags &= PL_TEXTURE_FLAG_NOMIPS;
	}

	texture->filter = PL_TEXTURE_FILTER_NEAREST;

	if ( !plUploadTextureImage( texture, imageData )) {
		PrintError( "Failed to generate texture from image!\nPL: %s\n", plGetError());
	}

	plDestroyImage( imageData );

	return texture;
}

GfxAnimationFrame *Gfx_LoadPictureByIndex( const RGBMap *palette, unsigned int index ) {
	PLFile *filePtr = plLoadPackageFileByIndex( globalWad, index );
	if ( filePtr == NULL ) {
		const char *fileName = plGetPackageFileName( globalWad, index );
		if ( fileName == NULL ) {
			fileName = "Unknown";
		}

		PrintError( "Failed to load picture %d (%s)!\nPL: %s\n", index, fileName, plGetError() );
	}

	bool status;
	uint8_t w = plReadInt8( filePtr, &status );
	uint8_t h = plReadInt8( filePtr, &status );
	uint8_t leftOffset = plReadInt8( filePtr, &status );
	uint8_t topOffset = plReadInt8( filePtr, &status );
	if ( !status ) {
		plCloseFile( filePtr );
		PrintError( "Failed to read in width and height for picture %d!\nPL: %s\n", index, plGetError() );
	}

	/* read in the column offsets */

	int16_t *columnOffsets = g_system.calloc( w, sizeof( int16_t ) );
	for ( unsigned int i = 0; i < w; ++i ) {
		columnOffsets[ i ] = plReadInt16( filePtr, false, &status );
	}

	if ( !status ) {
		plCloseFile( filePtr );
		PrintError( "Failed to read in column offsets for picture %d!\nPL: %s\n", index, plGetError() );
	}

	PLColour *colourBuffer = g_system.calloc( (size_t) w * h, sizeof( PLColour ) );
	for ( unsigned int i = 0; i < w; ++i ) {
		plFileSeek( filePtr, columnOffsets[ i ], PL_SEEK_SET );

		uint8_t rowStart = 0;
		while ( 1 ) {
			rowStart = plReadInt8( filePtr, &status );
			if ( rowStart == 255 ) {
				break;
			}

			uint8_t pixelCount = plReadInt8( filePtr, &status );
			if ( !status ) {
				PrintError( "Failed to read pixel count (i=%d)\nPL: %s\n", i, plGetError() );
			}

			for ( unsigned int j = 0; j < pixelCount; ++j ) {
				uint8_t pixel = plReadInt8( filePtr, &status );

				unsigned int pos = ( j + rowStart ) * w + i;
				colourBuffer[ pos ].r = playPal[ pixel ].r;
				colourBuffer[ pos ].g = playPal[ pixel ].g;
				colourBuffer[ pos ].b = playPal[ pixel ].b;

				/* unlike others, cyan denotes transparency here */
				bool isCyan =
						colourBuffer[ pos ].r == 0 &&
						colourBuffer[ pos ].g == 255 &&
						colourBuffer[ pos ].b == 255;
				colourBuffer[ pos ].a = isCyan ? 0 : 255;
			}
		}
	}

	plCloseFile( filePtr );

	/* setup the animation frame we're going to use */
	GfxAnimationFrame *frame = g_system.calloc( 1, sizeof( GfxAnimationFrame ) );
	frame->texture    = Gfx_GenerateTextureFromData(( uint8_t * ) colourBuffer, w, h, 4, false );
	frame->leftOffset = leftOffset;
	frame->topOffset  = topOffset;

	free( colourBuffer );

	return frame;
}

GfxAnimationFrame *Gfx_LoadPictureByName( const RGBMap *palette, const char *indexName ) {
	return Gfx_LoadPictureByIndex( palette, plGetPackageTableIndex( globalWad, indexName ) );
}

PLTexture *Gfx_GetWallTexture( unsigned int index ) {
	if ( index >= numWallTextures ) {
		PrintWarn( "Invalid wall texture slot (%d/%d)!\n", index, numWallTextures );
		return fallbackTexture;
	}

	return wallTextures[ index ]->texture;
}

void Gfx_LoadWallTextures( void ) {
	unsigned int posStart = plGetPackageTableIndex( globalWad, "P_START" ) + 1;
	if ( plGetFunctionResult() != PL_RESULT_SUCCESS ) {
		PrintError( "Failed to find the start of the wall table!\nPL: %s\n", plGetError() );
	}

	unsigned int posEnd = plGetPackageTableIndex( globalWad, "P_END" );
	if ( plGetFunctionResult() != PL_RESULT_SUCCESS ) {
		PrintError( "Failed to find the end of the wall table!\nPL: %s\n", plGetError() );
	}

	numWallTextures = posEnd - posStart;
	wallTextures = g_system.calloc( numWallTextures, sizeof( PLTexture* ) );

	for ( unsigned int i = 0; i < numWallTextures; ++i ) {
		unsigned int fileIndex = posStart + i;
		wallTextures[ i ] = Gfx_LoadPictureByIndex( playPal, fileIndex );
	}
}

PLTexture *Gfx_LoadFlatByIndex( const RGBMap *palette, unsigned int index ) {
	PLFile *filePtr = plLoadPackageFileByIndex( globalWad, index );
	if ( filePtr == NULL ) {
		const char *fileName = plGetPackageFileName( globalWad, index );
		if ( fileName == NULL ) {
			fileName = "Unknown";
		}

		PrintWarn( "Failed to load flat %d (%s)!\nPL: %s\n", index, fileName, plGetError() );
		return fallbackTexture;
	}

	/* all flats are assumed to be 64x64 */
	size_t flatSize = plGetFileSize( filePtr );
	if ( flatSize != 4096 ) {
		PrintWarn( "Unexpected flat size for %d, %d/64!\n", index, flatSize );
		plCloseFile( filePtr );
		return fallbackTexture;
	}

	PLColour *colourBuffer = g_system.calloc( flatSize, sizeof( PLColour ) );
	for ( unsigned int i = 0; i < flatSize; ++i ) {
		bool status;
		uint8_t pixel = plReadInt8( filePtr, &status );
		if ( !status ) {
			PrintError( "Failed to read pixel %d for flat %d!\nPL: %s\n", i, index, plGetError() );
		}

		colourBuffer[ i ].r = palette[ pixel ].r;
		colourBuffer[ i ].g = palette[ pixel ].g;
		colourBuffer[ i ].b = palette[ pixel ].b;
		/* transparency isn't supported here? */
		colourBuffer[ i ].a = 255;
	}

	plCloseFile( filePtr );

	PLTexture *texture = Gfx_GenerateTextureFromData(( uint8_t * ) colourBuffer, 64, 64, 4, false );

	free( colourBuffer );

	return texture;
}

PLTexture *Gfx_GetFloorTexture( unsigned int index ) {
	if ( index >= numFloorTextures ) {
		PrintWarn( "Invalid floor texture slot (%d/%d)!\n", index, numFloorTextures );
		return fallbackTexture;
	}

	return floorTextures[ index ];
}

void Gfx_LoadFloorTextures( void ) {
	unsigned int posStart = plGetPackageTableIndex( globalWad, "F_START" ) + 1;
	if ( plGetFunctionResult() != PL_RESULT_SUCCESS ) {
		PrintError( "Failed to find the start of the floor table!\nPL: %s\n", plGetError() );
	}

	unsigned int posEnd = plGetPackageTableIndex( globalWad, "F_END" );
	if ( plGetFunctionResult() != PL_RESULT_SUCCESS ) {
		PrintError( "Failed to find the end of the floor table!\nPL: %s\n", plGetError() );
	}

	numFloorTextures = posEnd - posStart;
	floorTextures = g_system.calloc( numFloorTextures, sizeof( PLTexture* ) );

	for ( unsigned int i = 0; i < numFloorTextures; ++i ) {
		unsigned int fileIndex = posStart + i;
		floorTextures[ i ] = Gfx_LoadFlatByIndex( playPal, fileIndex );
	}
}

void Gfx_LoadPalette( RGBMap *palette, const char *indexName ) {
	/* clear out the palette before we continue */
	memset( palette, 0, 256 );

	PLFile *filePtr = plLoadPackageFile( globalWad, indexName );
	if ( filePtr == NULL) {
		PrintWarn( "Failed to find \"%s\"!\nPL: %s\n", indexName, plGetError());
		return;
	}

	if ( plReadFile( filePtr, palette, sizeof( RGBMap ), 256 ) != 256 ) {
		PrintWarn( "Failed to read in palette data for \"%s\"!\nPL: %s\n", indexName, plGetError());
	}

	plCloseFile( filePtr );
}

void Gfx_LoadAnimationFrames( const char **frameList, GfxAnimationFrame **destination, unsigned int numFrames ) {
	for ( unsigned int i = 0; i < numFrames; ++i ) {
		PrintMsg( "Loading frame %d (%s)...\n", i, frameList[ i ] );
		destination[ i ] = Gfx_LoadPictureByName( playPal, frameList[ i ] );
		if ( destination[ i ] == NULL ) {
			PrintError( "Failed to load frame %d (%s)!\n", i, frameList[ i ] );
		}
	}
}

PLTexture *Gfx_LoadTexture( const char *path ) {
	PLImage *image = Image_LoadPackedImage( path );
	if ( image == NULL ) {
		PLTexture *texture = plLoadTextureFromImage( path, PL_TEXTURE_FILTER_MIPMAP_NEAREST );
		if ( texture == NULL ) {
			PrintWarn( "Failed to load texture \"%s\"!\nPL: %s\n", path, plGetError() );
			return fallbackTexture;
		}

		return texture;
	}

	PLTexture *texture = plCreateTexture();
	if ( !plUploadTextureImage( texture, image ) ) {
		PrintWarn( "Failed to load texture \"%s\"!\nPL: %s\n", path, plGetError() );
		texture = fallbackTexture;
	}
	plDestroyImage( image );

	return texture;
}

PLTexture *Gfx_LoadLumpTexture( const RGBMap *palette, const char *indexName ) {
	PLFile *filePtr = plLoadPackageFile( globalWad, indexName );
	if ( filePtr == NULL) {
		PrintWarn( "Failed to find \"%s\"!\nPL: %s\n", indexName, plGetError());
		return fallbackTexture;
	}

	bool status;
	uint16_t width = plReadInt16( filePtr, false, &status );
	if ( width == 0 ) {
		PrintError( "Invalid image width for \"%s\"!\n", indexName );
	}

	uint16_t height = plReadInt16( filePtr, false, &status );
	if ( height == 0 ) {
		PrintError( "Invalid image height \"%s\"!\n", indexName );
	}

	if ( !status ) {
		PrintError( "Failed to read in lump width and height for \"%s\"!\nPL: %s\n", indexName, plGetError());
	}

	unsigned int lumpDataSize = width * height;

	/* seems to be totally unused... */
	plFileSeek( filePtr, 4, PL_SEEK_CUR );

	uint8_t *imageBuffer = g_system.calloc( lumpDataSize, sizeof( uint8_t ) );
	if ( plReadFile( filePtr, imageBuffer, 1, lumpDataSize ) != lumpDataSize ) {
		PrintError( "Failed to read in lump data for \"%s\"!\nPL: %s\n", indexName, plGetError());
	}

	plCloseFile( filePtr );

	/* now convert using the palette (I'm lazy, so we'll just convert to rgba) */
	PLColour *colourBuffer = g_system.calloc( lumpDataSize, sizeof( PLColour ) );
	for ( unsigned int i = 0; i < lumpDataSize; ++i ) {
		colourBuffer[ i ].r = palette[ imageBuffer[ i ] ].r;
		colourBuffer[ i ].g = palette[ imageBuffer[ i ] ].g;
		colourBuffer[ i ].b = palette[ imageBuffer[ i ] ].b;
		colourBuffer[ i ].a = ( imageBuffer[ i ] == 255 ) ? 0 : 255;
	}
	free( imageBuffer );

	PLTexture *texture = Gfx_GenerateTextureFromData(( uint8_t * ) colourBuffer, width, height, 4, false );

	free( colourBuffer );

	return texture;
}

static void Gfx_RegisterShaderStage( PLShaderProgram *program, PLShaderStageType type, const char *path ) {
	PLFile *filePtr = plOpenFile( path, true );
	if ( filePtr == NULL ) {
		PrintError( "Failed to find shader \"%s\" in WAD!\nPL: %s\n", path, plGetError() );
	}

	const char *buffer = ( const char* ) plGetFileData( filePtr );
	size_t length = plGetFileSize( filePtr );

	if ( !plRegisterShaderStageFromMemory( program, buffer, length, type ) ) {
		PrintError( "Failed to register stage!\nPL: %s\n", plGetError() );
	}

	plCloseFile( filePtr );
}

static void Gfx_RegisterShader( GfxShaderType type, const char *vertPath, const char *fragPath ) {
	shaderPrograms[ type ] = plCreateShaderProgram();
	if ( shaderPrograms[ type ] == NULL) {
		PrintError( "Failed to create shader program!\nPL: %s\n", plGetError());
	}

	Gfx_RegisterShaderStage( shaderPrograms[ type ], PL_SHADER_TYPE_VERTEX, vertPath );
	Gfx_RegisterShaderStage( shaderPrograms[ type ], PL_SHADER_TYPE_FRAGMENT, fragPath );

	if ( !plLinkShaderProgram( shaderPrograms[ type ] )) {
		PrintError( "Failed to link shader stages!\nPL: %s\n", plGetError());
	}
}

void Gfx_EnableShaderProgram( GfxShaderType type ) {
	plSetShaderProgram( shaderPrograms[ type ] );
}

void Gfx_DrawAnimationFrame( GfxAnimationFrame *frame, const PLVector3 *position, float spriteAngle ) {
	PLMatrix4 transform;
	transform = plMatrix4Identity();
	transform = plMultiplyMatrix4( transform,
								   plRotateMatrix4( plDegreesToRadians( 0.0f ), PLVector3( 1, 0, 0 ) ));
	transform = plMultiplyMatrix4( transform,
								   plRotateMatrix4( plDegreesToRadians( spriteAngle ), PLVector3( 0, 1, 0 ) ));
	transform = plMultiplyMatrix4( transform,
								   plRotateMatrix4( plDegreesToRadians( 180.0f ), PLVector3( 0, 0, 1 ) ));
	transform = plMultiplyMatrix4( transform, plTranslateMatrix4( *position ) );

	Gfx_EnableShaderProgram( SHADER_LIT );

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

	plDrawTexturedRectangle( &transform, x, y, w, h, frame->texture );
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
	if ( digit < 0 ) { digit = 0; }
	else if ( digit > 9 ) { digit = 9; }

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
void Gfx_ShutdownCameras( void );	/* gfx_camera.c */

void Gfx_SetupDefaultState( void ) {
	plSetClearColour(PLColour( 0, 0, 0, 255 ) );

	plSetDepthBufferMode( PL_DEPTHBUFFER_ENABLE );
	plSetDepthMask( true );

	Gfx_EnableShaderProgram( SHADER_GENERIC );
}

void Gfx_Initialize( void ) {
	PrintMsg( "Initializing Gfx...\n" );

	plSetGraphicsMode( PL_GFX_MODE_OPENGL_CORE );

	/* create both the interface camera and player camera */

	auxCamera = plCreateCamera();
	if ( auxCamera == NULL) {
		PrintError( "Failed to create auxiliary camera!\nPL: %s\n", plGetError());
	}
	auxCamera->mode = PL_CAMERA_MODE_ORTHOGRAPHIC;
	auxCamera->near = 0.0f;
	auxCamera->far = 1000.0f;
	auxCamera->viewport.w = DISPLAY_WIDTH;
	auxCamera->viewport.h = DISPLAY_HEIGHT;

	Gfx_InitializeCameras();

	/* create the default shader programs */
	Gfx_RegisterShader( SHADER_GENERIC, "Shader:vertex.glsl", "Shader:colour.glsl" );
	Gfx_RegisterShader( SHADER_TEXTURE, "Shader:vertex.glsl", "Shader:texture.glsl" );
	Gfx_RegisterShader( SHADER_ALPHA_TEST, "Shader:vertex.glsl", "Shader:alpha.glsl" );
	Gfx_RegisterShader( SHADER_LIT, "Shader:vertex.glsl", "Shader:lit.glsl" );

	Gfx_LoadPalette( titlePal, "TITLEPAL" );
	Gfx_LoadPalette( playPal, "PLAYPAL" );

	/* generate fallback texture */
	PLColour fallbackData[] = {
			{ 128, 0, 128, 255 },
			{ 0, 128, 128, 255 },
			{ 0, 128, 128, 255 },
			{ 128, 0, 128, 255 },
	};
	fallbackTexture = Gfx_GenerateTextureFromData(( uint8_t * ) fallbackData, 2, 2, 4, false );
	if ( fallbackTexture == NULL) {
		PrintError( "Failed to create fallback texture!\n" );
	}

	defaultFont = Gfx_LoadTexture( "Engine:DefaultFont.gfx" );

	/* and now, finally, load in the splash screen! */
	titlePicTexture = Gfx_LoadLumpTexture( titlePal, "TITLEPIC" );

	/* load the numbers */
	for ( unsigned int i = 0; i < 10; ++i ) {
		char numName[16];
		snprintf( numName, sizeof( numName ), "WNUMBER%d", i );
		numTextureTable[ i ] = Gfx_LoadLumpTexture( titlePal, numName );
	}

	Gfx_LoadWallTextures();
	Gfx_LoadFloorTextures();

	Gfx_SetupDefaultState();
}

void Gfx_Shutdown( void ) {
	Gfx_ShutdownCameras();
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

void Gfx_DrawMenu( void ) {
	plSetupCamera( auxCamera );

	PLMatrix4 transform = plMatrix4Identity();

#ifndef DEBUG_CAM
	switch ( Game_GetMenuState()) {
		default:
		PrintError( "Invalid menu state!\n" );

		case MENU_STATE_START:
			Gfx_EnableShaderProgram( SHADER_TEXTURE );
			plDrawTexturedRectangle( &transform, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, titlePicTexture );
			break;

		case MENU_STATE_HUD:
			Gfx_EnableShaderProgram( SHADER_ALPHA_TEST );
			Gfx_DrawViewSprite();
			break;
	}
#endif
}

void Gfx_DrawAxesPivot( PLVector3 position, PLVector3 rotation ) {
	PLMatrix4 transform;
	transform = plMatrix4Identity();
	transform = plMultiplyMatrix4( transform,
								   plRotateMatrix4( plDegreesToRadians( rotation.x ), PLVector3( 1, 0, 0 ) ));
	transform = plMultiplyMatrix4( transform,
								   plRotateMatrix4( plDegreesToRadians( rotation.y ), PLVector3( 0, 1, 0 ) ));
	transform = plMultiplyMatrix4( transform,
								   plRotateMatrix4( plDegreesToRadians( rotation.z ), PLVector3( 0, 0, 1 ) ));
	transform = plMultiplyMatrix4( transform, plTranslateMatrix4( position ) );
	plDrawSimpleLine( &transform, &PLVector3( 0, 0, 0 ), &PLVector3( 10, 0, 0 ), &PLColour( 255, 0, 0, 255 ) );
	plDrawSimpleLine( &transform, &PLVector3( 0, 0, 0 ), &PLVector3( 0, 10, 0 ), &PLColour( 0, 255, 0, 255 ) );
	plDrawSimpleLine( &transform, &PLVector3( 0, 0, 0 ), &PLVector3( 0, 0, 10 ), &PLColour( 0, 0, 255, 255 ) );
	//printf( "%s\n", plPrintVector3( &position, pl_int_var ) );
}

void Gfx_DrawScene( GfxCamera *camera ) {
#ifdef DEBUG_CAM
	PLMatrix4 mat = plMatrix4Identity();

	PLVector3 forward, left;
	plAnglesAxes( PLVector3( 0, Act_GetAngle( player ), 0 ), &left, NULL, &forward );

	PLVector3 startPos = Act_GetPosition( player );
	startPos.y += Act_GetViewOffset( player );
	PLVector3 endPos = plAddVector3( startPos, plScaleVector3f( forward, 64.0f ) );
	plDrawLine( &mat, &startPos, &PLColour( 0, 0, 255, 255 ), &endPos, &PLColour( 0, 0, 255, 255 ) );

	startPos = endPos;
	endPos = plAddVector3( startPos, plScaleVector3f( left, 512.0f ) );
	plDrawLine( &mat, &startPos, &PLColour( 0, 0, 255, 255 ), &endPos, &PLColour( 255, 0, 0, 255 ) );

	endPos = plSubtractVector3( startPos, plScaleVector3f( left, 512.0f ) );
	plDrawLine( &mat, &startPos, &PLColour( 0, 0, 255, 255 ), &endPos, &PLColour( 255, 0, 0, 255 ) );
#endif

#if 0
	if ( ( g_system.GetLaunchMode() == LAUNCH_MODE_DEFAULT ) && ( Game_GetMenuState() == MENU_STATE_START ) ) {
		return;
	}
#endif

	Map_Draw();
	Act_DrawActors();
}

/********************************************/
/* Font Render */
/********************************************/

void Gfx_DrawCharacter( PLTexture *baseTexture, char character, float x, float y, float scale ) {
	if ( baseTexture == NULL) {
		baseTexture = defaultFont;
	}

	Gfx_EnableShaderProgram( SHADER_ALPHA_TEST );

	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();
	plLoadIdentityMatrix();

	plScaleMatrix( PLVector3( scale, scale, scale ) );

	plDrawTexturedRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), x, y, 8, 8, baseTexture );

	plPopMatrix();
}

void Gfx_DrawString( PLTexture *baseTexture, const char *string, float x, float y, float scale ) {
	if ( baseTexture == NULL ) {
		baseTexture = defaultFont;
	}

	Gfx_EnableShaderProgram( SHADER_ALPHA_TEST );


}
