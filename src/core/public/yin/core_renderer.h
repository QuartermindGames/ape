// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plgraphics/plg.h>
#include <plgraphics/plg_mesh.h>

// TODO: retire this...
typedef enum ApeCacheGroup
{
	APE_CACHE_GROUP_GLOBAL,// everything here is retained globally, and won't be unloaded
	APE_CACHE_GROUP_EDITOR,// these are cached when the editor is enabled, and free'd up when it's disabled
	APE_CACHE_GROUP_WORLD, // will be cached on world load and free'd up when world is unloaded

	APE_MAX_CACHE_GROUPS
} ApeCacheGroup;

PL_EXTERN_C

typedef struct ApeCamera ApeCamera;
typedef struct ApeViewport ApeViewport;
typedef struct ApeLight ApeLight;
typedef struct ApeRenderTarget ApeRenderTarget;
typedef struct ApeTexture ApeTexture;
typedef struct ApeMaterial ApeMaterial;
typedef struct ApeWorld ApeWorld;

/////////////////////////////////////////////////////////////////////////////////////
// Viewport API

#define APE_MAX_FPS_READINGS 128

typedef struct ApeViewport
{
	unsigned int index;

	int x, y;
	int width, height;

	float zoom;// used for the editor / 2D views

	ApeCamera *camera;
	ApeRenderTarget *renderTarget;

	struct
	{
		double frameTime, oldTime;
		unsigned int frameIndex;

		unsigned int lastFramerate;
		unsigned int lastFramerateUpdate;
		double frameReadings[ APE_MAX_FPS_READINGS ];

		unsigned int numBatches;
		unsigned int numTriangles;
		unsigned int numPolygons;
		unsigned int numPortals;
	} perf;

	void *windowHandle;
} ApeViewport;

ApeViewport *ape_get_viewport_by_slot( unsigned int slot );

ApeViewport *ape_viewport_create( int x, int y, int width, int height, void *windowHandle );
void ape_viewport_destroy( ApeViewport *self );

void ape_viewport_set_camera( ApeViewport *self, ApeCamera *camera );
ApeCamera *ape_viewport_get_camera( ApeViewport *viewport );

void ape_viewport_set_size( ApeViewport *self, int width, int height );
void ape_viewport_get_size( const ApeViewport *self, int *width, int *height );

unsigned int ape_viewport_get_framerate( ApeViewport *self );
ApeRenderTarget *ape_viewport_get_render_target( ApeViewport *self );

void ape_viewport_make_active( ApeViewport *self );
ApeViewport *ape_viewport_get_active( void );

/////////////////////////////////////////////////////////////////////////////////////
// Camera API

void ape_camera_draw_perspective( ApeCamera *camera, ApeViewport *viewport );

/////////////////////////////////////////////////////////////////////////////////////
// Render Target API

ApeRenderTarget *ape_render_target_create( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter );
void ape_render_target_release( ApeRenderTarget *renderTarget );
void ape_render_target_set_size( ApeRenderTarget *renderTarget, unsigned int width, unsigned int height );
void ape_render_target_get_size( const ApeRenderTarget *renderTarget, unsigned int *width, unsigned int *height );
PLGTexture *ape_render_target_get_texture( ApeRenderTarget *renderTarget );

/**
 * If the provided render target is null, this will clear whatever is currently set back to the default.
 */
void ape_render_target_bind( ApeRenderTarget *renderTarget, PLGFrameBufferObjectTarget target );

PLGFrameBuffer *ape_render_target_get_frame_buffer( ApeRenderTarget *renderTarget );

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

typedef enum SSArlDefaultMaterial
{
	SS_ARL_MATERIAL_DEFAULT_FALLBACK,
	SS_ARL_MATERIAL_DEFAULT_VERTEX,
	SS_ARL_MATERIAL_DEFAULT_SHADOW,
	SS_ARL_MATERIAL_DEFAULT_DEPTH,

	SS_ARL_MAX_DEFAULT_MATERIALS
} SSArlDefaultMaterial;

ApeMaterial *ss_arl_get_default_material( SSArlDefaultMaterial defaultMaterial );

/**
 * Returns the original path the material was loaded from.
 */
const char *ape_material_get_path( const ApeMaterial *material );

/**
 * Cache a new material into memory if not so already, otherwise
 * returns an existing material from the cache and adds a reference -
 * reference will need to be released once finished with.
 */
ApeMaterial *ape_material_cache( const char *path, ApeCacheGroup group, bool useFallback, bool preview );

/**
 * Releases a reference to the material, allowing it to clean up.
 */
void ape_material_release( ApeMaterial *material );

/**
 * Returns the surface type for the material.
 */
int8_t ss_arl_material_get_surface_type( const ApeMaterial *material );

/**
 * Draws the given mesh with the given material. This also updates the peformance tracking,
 * so ideally you should always use this when drawing any mesh.
 */
void ape_material_draw( ApeMaterial *material, PLGMesh *mesh, ApeLight **lights, unsigned int numLights );

/**
 * Returns the texture representing a material.
 * Can be used to see what a texture looks like without loading the whole
 * thing into memory if material is loaded with 'preview'.
 */
PLGTexture *ss_arl_material_get_preview_texture( ApeMaterial *material );

unsigned int ape_material_get_flags( const ApeMaterial *self );

/**********************************************************/
// Fonts
/**********************************************************/

/** !!!OLD BITMAP API - PREFERABLY AVOID!!! ***************/

typedef struct ApeBitmapFont ApeBitmapFont;

ApeBitmapFont *ss_arl_bitmap_font_cache( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end );
void ss_arl_bitmap_font_release( ApeBitmapFont *font );

ApeBitmapFont *ape_get_default_small_bitmap_font( void );

void ss_arl_bitmap_font_batch_character( const ApeBitmapFont *font, float x, float y, float scale, PLColour colour, uint8_t character );
void ape_bitmap_font_batch_string( const ApeBitmapFont *font, float x, float y, float scale, PLColour colour, const char *msg, size_t length, bool shadow );

void ape_bitmap_font_begin_draw( ApeBitmapFont *font );
void ape_bitmap_font_draw( ApeBitmapFont *font );

/**********************************************************/

/////////////////////////////////////////////////////////////////////////////////////
// Draw API

void ape_draw_sprite( ApeMaterial *material, const PLQuad *subRect, const PLColourF32 *colour, const PLVector3 *position, const PLVector3 *origin, const PLVector3 *angles, float scale );

/**
 * Draws a textured quad. *SLOW* so use sparingly.
 * If material is null, this will draw without passing through the material API.
 */
void ape_draw_textured_quad( ApeMaterial *material, float x, float y, float w, float h, const PLColour *colour );

void arl_draw_axis_pivot( PLVector3 position, PLVector3 rotation, float scale );
void ape_draw_graph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

PL_EXTERN_C_END
