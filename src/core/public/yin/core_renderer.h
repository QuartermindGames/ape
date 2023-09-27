// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plgraphics/plg.h>
#include <plgraphics/plg_mesh.h>

typedef struct ApeCamera ApeCamera;
typedef struct ApeViewport ApeViewport;
typedef struct ApeLight ApeLight;
typedef struct ApeTexture ApeTexture;
typedef struct ApeMaterial ApeMaterial;

// TODO: retire this...
typedef enum ApeCacheGroup {
	APE_CACHE_EDITOR,
	APE_CACHE_WORLD, /* everything that is cached during level load */

	APE_MAX_CACHE_GROUPS
} ApeCacheGroup;

PL_EXTERN_C

ApeViewport *apeCreateViewport( int x, int y, int width, int height, void *windowHandle );
void apeDestroyViewport( ApeViewport *viewport );
ApeViewport *apeGetViewportBySlot( unsigned int slot );
void apeSetViewportCamera( ApeViewport *viewport, ApeCamera *camera );
void apeSetViewportSize( ApeViewport *viewport, int width, int height );
void apeGetViewportSize( const ApeViewport *viewport, int *width, int *height );
unsigned int apeGetViewportFramerate( const ApeViewport *viewport );

/**********************************************************/
// Textures
/**********************************************************/

/**
 * Attempts to load the desired texture.
 * If it's already cached, will return the existing instance.
 * Will return NULL on fail.
 */
ApeTexture *YnCore_Texture_Load( const char *path );

/**
 * Releases the reference for the given texture.
 */
void apeReleaseTexture( ApeTexture *texture );

/**********************************************************/
// Materials
/**********************************************************/

/** !!!SHADER API - PREFERABLY AVOID!!! *******************/

typedef enum ApeDefaultShaderProgram {
	APE_SHADER_DEFAULT,
	APE_SHADER_LIGHTING_PASS,
	APE_SHADER_DEFAULT_VERTEX,
	APE_SHADER_DEFAULT_ALPHA,
	APE_SHADER_DEFAULT_FONT,
	APE_SHADER_DEFAULT_SHADOW,

	APE_MAX_DEFAULT_SHADERS
} ApeDefaultShaderProgram;

PLGShaderProgram *apeGetDefaultShaderProgram( ApeDefaultShaderProgram defaultShaderProgram );

/**********************************************************/

/**
 * Returns the original path the material was loaded from.
 */
const char *ar_material_get_path( const ApeMaterial *material );

/**
 * Cache a new material into memory if not so already, otherwise
 * returns an existing material from the cache and adds a reference -
 * reference will need to be released once finished with.
 */
ApeMaterial *apeCacheMaterial( const char *path, ApeCacheGroup group, bool useFallback, bool preview );

/**
 * Releases a reference to the material, allowing it to clean up.
 */
void ar_material_release( ApeMaterial *material );

/**
 * Returns the surface type for the material.
 */
int8_t ar_material_get_surface_type( const ApeMaterial *material );

/**
 * Draws the given mesh with the given material. This also updates the peformance tracking,
 * so ideally you should always use this when drawing any mesh.
 */
void apeDrawMesh( ApeMaterial *material, PLGMesh *mesh, ApeLight **lights, unsigned int numLights );

/**
 * Returns the texture representing a material.
 * Can be used to see what a texture looks like without loading the whole
 * thing into memory if material is loaded with 'preview'.
 */
PLGTexture *ar_material_get_preview_texture( ApeMaterial *material );

/**********************************************************/
// Fonts
/**********************************************************/

/** !!!OLD BITMAP API - PREFERABLY AVOID!!! ***************/

typedef struct ApeBitmapFont ApeBitmapFont;

ApeBitmapFont *apeCacheBitmapFont( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end );
void apeReleaseBitmapFont( ApeBitmapFont *font );

ApeBitmapFont *apeGetDefaultBitmapFont( void );
ApeBitmapFont *apeGetDefaultSmallBitmapFont( void );

void apeAddBitmapCharacterToBatch( const ApeBitmapFont *font, float x, float y, float scale, PLColour colour, uint8_t character );
void apeAddBitmapStringToBatch( const ApeBitmapFont *font, float x, float y, float scale, PLColour colour, const char *msg, size_t length, bool shadow );

void apeDrawBitmapCharacter( ApeBitmapFont *font, float x, float y, float scale, PLColour colour, char character );
void apeDrawBitmapString( ApeBitmapFont *font, float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow );

void apeBeginBitmapFontDraw( ApeBitmapFont *font );
void apeDrawBitmapFont( ApeBitmapFont *font );

/**********************************************************/

PL_EXTERN_C_END
