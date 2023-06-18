// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plgraphics/plg_mesh.h>

typedef struct ApeCamera ApeCamera;
typedef struct ApeViewport ApeViewport;
typedef struct ApeLight ApeLight;
typedef struct ApeTexture ApeTexture;
typedef struct ApeMaterial ApeMaterial;

// TODO: retire this...
typedef enum YNCoreCacheGroup
{
	YN_CORE_CACHE_GROUP_EDITOR,
	YN_CORE_CACHE_GROUP_WORLD, /* everything that is cached during level load */

	YN_CORE_MAX_CACHE_GROUPS
} YNCoreCacheGroup;

PL_EXTERN_C

ApeViewport *YnCore_Viewport_Create( int x, int y, int width, int height, void *windowHandle );
void YnCore_Viewport_Destroy( ApeViewport *viewport );
ApeViewport *YnCore_Viewport_GetBySlot( unsigned int slot );
void YnCore_Viewport_SetCamera( ApeViewport *viewport, ApeCamera *camera );
void ogeViewport_SetSize( ApeViewport *viewport, int width, int height );
void YnCore_Viewport_GetSize( const ApeViewport *viewport, int *width, int *height );
unsigned int YnCore_Viewport_GetAverageFPS( const ApeViewport *viewport );

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

/**
 * Returns the original path the material was loaded from.
 */
const char *apeGetMaterialPath( const ApeMaterial *material );

/**
 * Returns the filename for the material.
 */
const char *YnCore_Material_GetName( const ApeMaterial *material );

/**
 * Cache a new material into memory if not so already, otherwise
 * returns an existing material from the cache and adds a reference -
 * reference will need to be released once finished with.
 */
ApeMaterial *apeCacheMaterial( const char *path, YNCoreCacheGroup group, bool useFallback, bool preview );

/**
 * Releases a reference to the material, allowing it to clean up.
 */
void apeReleaseMaterial( ApeMaterial *material );

/**
 * Draws the given mesh with the given material. This also updates the peformance tracking,
 * so ideally you should always use this when drawing any mesh.
 */
void apeDrawMesh( ApeMaterial *material, PLGMesh *mesh, ApeLight **lights, unsigned int numLights );

PL_EXTERN_C_END
