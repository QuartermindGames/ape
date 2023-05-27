// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plgraphics/plg_mesh.h>

typedef struct OgeCamera OgeCamera;
typedef struct OgeViewport OgeViewport;
typedef struct OgeLight OgeLight;
typedef struct OgeTexture OgeTexture;
typedef struct OgeMaterial OgeMaterial;

// TODO: retire this...
typedef enum YNCoreCacheGroup
{
	YN_CORE_CACHE_GROUP_EDITOR,
	YN_CORE_CACHE_GROUP_WORLD, /* everything that is cached during level load */

	YN_CORE_MAX_CACHE_GROUPS
} YNCoreCacheGroup;

PL_EXTERN_C

OgeViewport *YnCore_Viewport_Create( int x, int y, int width, int height, void *windowHandle );
void YnCore_Viewport_Destroy( OgeViewport *viewport );
OgeViewport *YnCore_Viewport_GetBySlot( unsigned int slot );
void YnCore_Viewport_SetCamera( OgeViewport *viewport, OgeCamera *camera );
void ogeViewport_SetSize( OgeViewport *viewport, int width, int height );
void YnCore_Viewport_GetSize( const OgeViewport *viewport, int *width, int *height );
unsigned int YnCore_Viewport_GetAverageFPS( const OgeViewport *viewport );

/**********************************************************/
// Textures
/**********************************************************/

/**
 * Attempts to load the desired texture.
 * If it's already cached, will return the existing instance.
 * Will return NULL on fail.
 */
OgeTexture *YnCore_Texture_Load( const char *path );

/**
 * Releases the reference for the given texture.
 */
void YnCore_Texture_Release( OgeTexture *texture );

/**********************************************************/
// Materials
/**********************************************************/

/**
 * Returns the original path the material was loaded from.
 */
const char *ogeMaterial_GetPath( const OgeMaterial *material );

/**
 * Returns the filename for the material.
 */
const char *YnCore_Material_GetName( const OgeMaterial *material );

/**
 * Cache a new material into memory if not so already, otherwise
 * returns an existing material from the cache and adds a reference -
 * reference will need to be released once finished with.
 */
OgeMaterial *ogeCacheMaterial( const char *path, YNCoreCacheGroup group, bool useFallback, bool preview );

/**
 * Releases a reference to the material, allowing it to clean up.
 */
void ogeMaterial_Release( OgeMaterial *material );

/**
 * Draws the given mesh with the given material. This also updates the peformance tracking,
 * so ideally you should always use this when drawing any mesh.
 */
void ogeMaterial_DrawMesh( OgeMaterial *material, PLGMesh *mesh, OgeLight *lights, unsigned int numLights );

PL_EXTERN_C_END
