// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plgraphics/plg.h>
#include <plgraphics/plg_mesh.h>

// TODO: retire this...
typedef enum SS_Arl_CacheGroup
{
	APE_CACHE_EDITOR,
	APE_CACHE_WORLD, /* everything that is cached during level load */

	APE_MAX_CACHE_GROUPS
} SS_Arl_CacheGroup;

PL_EXTERN_C

typedef struct SSArlCamera SSArlCamera;
typedef struct SSArlViewport SSArlViewport;
typedef struct SSArlLight SSArlLight;
typedef struct SSArlRenderTarget SSArlRenderTarget;
typedef struct SSArlTexture SSArlTexture;
typedef struct ApeMaterial ApeMaterial;

SSArlViewport *ss_arl_get_viewport_by_slot( unsigned int slot );

SSArlViewport *ss_arl_viewport_create( int x, int y, int width, int height, void *windowHandle );
void ss_arl_viewport_destroy( SSArlViewport *viewport );
void ss_arl_viewport_set_camera( SSArlViewport *viewport, SSArlCamera *camera );
SSArlCamera *ss_arl_viewport_get_camera( SSArlViewport *viewport );
void ss_arl_viewport_set_size( SSArlViewport *viewport, int width, int height );
void ss_arl_viewport_get_size( const SSArlViewport *viewport, int *width, int *height );
unsigned int ss_arl_viewport_get_framerate( const SSArlViewport *viewport );
SSArlRenderTarget *ss_arl_viewport_get_render_target( SSArlViewport *viewport );
void ss_arl_viewport_make_active( SSArlViewport *viewport );

/////////////////////////////////////////////////////////////////////////////////////
// Render Target API

SSArlRenderTarget *ss_arl_render_target_create( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter );
void ss_arl_render_target_release( SSArlRenderTarget *renderTarget );
void ss_arl_render_target_set_size( SSArlRenderTarget *renderTarget, unsigned int width, unsigned int height );
void ss_arl_render_target_get_size( const SSArlRenderTarget *renderTarget, unsigned int *width, unsigned int *height );
PLGTexture *ss_arl_render_target_get_texture( SSArlRenderTarget *renderTarget );
void ss_arl_render_target_bind( SSArlRenderTarget *renderTarget, PLGFrameBufferObjectTarget target );
PLGFrameBuffer *ss_arl_render_target_get_frame_buffer( SSArlRenderTarget *renderTarget );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
// Materials
/**********************************************************/

/** !!!SHADER API - PREFERABLY AVOID!!! *******************/

typedef enum ApeDefaultShaderProgram
{
	APE_SHADER_DEFAULT,

	APE_SHADER_DEFAULT_VERTEX,
	APE_SHADER_DEFAULT_ALPHA,
	APE_SHADER_DEFAULT_FONT,
	APE_SHADER_DEFAULT_SHADOW,

	APE_MAX_DEFAULT_SHADERS
} ApeDefaultShaderProgram;

PLGShaderProgram *ss_arl_shader_get_default( ApeDefaultShaderProgram defaultShaderProgram );

/**********************************************************/

/**
 * Returns the original path the material was loaded from.
 */
const char *ss_arl_material_get_path( const ApeMaterial *material );

/**
 * Cache a new material into memory if not so already, otherwise
 * returns an existing material from the cache and adds a reference -
 * reference will need to be released once finished with.
 */
ApeMaterial *ss_arl_material_cache( const char *path, SS_Arl_CacheGroup group, bool useFallback, bool preview );

/**
 * Releases a reference to the material, allowing it to clean up.
 */
void ss_arl_material_release( ApeMaterial *material );

/**
 * Returns the surface type for the material.
 */
int8_t ss_arl_material_get_surface_type( const ApeMaterial *material );

/**
 * Draws the given mesh with the given material. This also updates the peformance tracking,
 * so ideally you should always use this when drawing any mesh.
 */
void ss_arl_material_draw( ApeMaterial *material, PLGMesh *mesh, SSArlLight **lights, unsigned int numLights );

/**
 * Returns the texture representing a material.
 * Can be used to see what a texture looks like without loading the whole
 * thing into memory if material is loaded with 'preview'.
 */
PLGTexture *ss_arl_material_get_preview_texture( ApeMaterial *material );

/**********************************************************/
// Fonts
/**********************************************************/

/** !!!OLD BITMAP API - PREFERABLY AVOID!!! ***************/

typedef struct SS_Arl_BitmapFont SS_Arl_BitmapFont;

SS_Arl_BitmapFont *ss_arl_bitmap_font_cache( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end );
void ss_arl_bitmap_font_release( SS_Arl_BitmapFont *font );

SS_Arl_BitmapFont *ss_arl_get_default_bitmap_font( void );
SS_Arl_BitmapFont *ss_arl_get_default_small_bitmap_font( void );

void ss_arl_bitmap_font_batch_character( const SS_Arl_BitmapFont *font, float x, float y, float scale, PLColour colour, uint8_t character );
void ss_arl_bitmap_font_batch_string( const SS_Arl_BitmapFont *font, float x, float y, float scale, PLColour colour, const char *msg, size_t length, bool shadow );

void ss_arl_bitmap_font_draw_character( SS_Arl_BitmapFont *font, float x, float y, float scale, PLColour colour, char character );
void ss_arl_bitmap_font_draw_string( SS_Arl_BitmapFont *font, float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow );

void ss_arl_bitmap_font_begin_draw( SS_Arl_BitmapFont *font );
void ss_arl_bitmap_font_draw( SS_Arl_BitmapFont *font );

/**********************************************************/

/////////////////////////////////////////////////////////////////////////////////////
// Draw API

void ss_arl_draw_sprite( ApeMaterial *material, const PLQuad *subRect, const PLVector3 *position, const PLVector3 *origin, const PLVector3 *angles, float scale );
void ss_arl_draw_quad( ApeMaterial *material, int x, int y, int w, int h, const PLColour *colour );
void arl_draw_axis_pivot( PLVector3 position, PLVector3 rotation, float scale );
void arl_draw_graph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

PL_EXTERN_C_END
