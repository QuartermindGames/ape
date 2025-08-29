// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape_public_gui.h"

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

typedef struct ComCollisionCylinder ComCollisionCylinder;

typedef struct ApeCamera       ApeCamera;
typedef struct ApeViewport     ApeViewport;
typedef struct ApeLight        ApeLight;
typedef struct ApeRenderTarget ApeRenderTarget;
typedef struct ApeTexture      ApeTexture;
typedef struct ApeMaterial     ApeMaterial;
typedef struct ApeRoom         ApeRoom;
typedef struct ApeWorld        ApeWorld;

/////////////////////////////////////////////////////////////////////////////////////
// Viewport API

#define APE_MAX_FPS_READINGS 128

typedef struct ApeViewport
{
	unsigned int index;

	int x, y;
	int width, height;

	float zoom;// used for the editor / 2D views

	ApeCamera       *camera;
	ApeRenderTarget *renderTarget;
	QmMathColour4ub  clearColour;

	struct
	{
		double       frameTime, oldTime;
		unsigned int frameIndex;

		unsigned int lastFramerate;
		unsigned int lastFramerateUpdate;
		double       frameReadings[ APE_MAX_FPS_READINGS ];

		unsigned int numBatches;
		unsigned int numTriangles;
		unsigned int numPolygons;
		unsigned int numPortals;
	} perf;

	void *windowHandle;
} ApeViewport;

ApeViewport *ape_get_viewport_by_slot( unsigned int slot );

ApeViewport *ape_viewport_create( int x, int y, int width, int height, void *windowHandle, bool msaa );
void         ape_viewport_destroy( ApeViewport *self );

void       ape_viewport_set_camera( ApeViewport *self, ApeCamera *camera );
ApeCamera *ape_viewport_get_camera( ApeViewport *viewport );

void ape_viewport_set_size( ApeViewport *self, int width, int height );
void ape_viewport_get_size( const ApeViewport *self, int *width, int *height );

unsigned int     ape_viewport_get_framerate( ApeViewport *self );
ApeRenderTarget *ape_viewport_get_render_target( ApeViewport *self );

void         ape_viewport_make_active( ApeViewport *self );
ApeViewport *ape_viewport_get_active( void );

void ape_viewport_set_clip( const ApeViewport *self );

void ape_viewport_set_clear_colour( ApeViewport *self, const QmMathColour4ub *clearColour );

/**
 * Convert the given position in screen space, into a position in world space.
 *
 * @param self			Viewport instance.
 * @param pos			Screen position.
 * @param viewMatrix	View matrix.
 * @param projMatrix	Projection matrix.
 * @return				World-space position.
 */
QmMathVector3f ape_viewport_convert_screen_to_world( const ApeViewport *self, const int pos[ 2 ], const PLMatrix4 *viewMatrix, const PLMatrix4 *projMatrix );

/////////////////////////////////////////////////////////////////////////////////////
// Render Target API

ApeRenderTarget *ape_render_target_create( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter, bool useMsaa );
void             ape_render_target_release( ApeRenderTarget *renderTarget );
void             ape_render_target_set_size( ApeRenderTarget *renderTarget, unsigned int width, unsigned int height );
void             ape_render_target_get_size( const ApeRenderTarget *renderTarget, unsigned int *width, unsigned int *height );
PLGTexture      *ape_render_target_get_texture( ApeRenderTarget *renderTarget );

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

typedef struct ApeShaderProgram ApeShaderProgram;

typedef enum ApeDefaultShaderProgram
{
	APE_SHADER_DEFAULT,

	//TODO: replace the below with materials, and pipe them through the material system instead
	APE_SHADER_DEFAULT_VERTEX,
	APE_SHADER_DEFAULT_ALPHA,
	APE_SHADER_DEFAULT_FONT,
	APE_SHADER_DEFAULT_SHADOW,
	APE_SHADER_DEFAULT_GRID,

	APE_MAX_DEFAULT_SHADERS,
	APE_SHADER_DEFAULT_NULL = APE_MAX_DEFAULT_SHADERS,
} ApeDefaultShaderProgram;

ApeShaderProgram *ape_get_default_shader( ApeDefaultShaderProgram defaultShaderProgram );

/**********************************************************/

typedef enum ApeDefaultMaterial
{
	APE_MATERIAL_DEFAULT_FALLBACK,    // fallback for missing materials
	APE_MATERIAL_DEFAULT_VERTEX,      // basic vertex, no texture
	APE_MATERIAL_DEFAULT_VERTEX_ALPHA,// basic vertex, no texture, blended
	APE_MATERIAL_DEFAULT_SHADOW,      // material used per shadow volumes
	APE_MATERIAL_DEFAULT_HIDDEN,      // material to display when a surface is marked hidden

	APE_MATERIAL_DEFAULT_EDITOR,          // default material to use per new brushes
	APE_MATERIAL_DEFAULT_EDITOR_SELECTION,// used to highlight faces/objects that are selected

	APE_MATERIAL_DEFAULT_DEBUG_NORMALS,

	APE_MAX_DEFAULT_MATERIALS
} ApeDefaultMaterial;

ApeMaterial *ape_material_get_default( ApeDefaultMaterial defaultMaterial );

/**
 * Returns the original path the material was loaded from.
 */
const char *ape_material_get_path( const ApeMaterial *material );

/**
 * Cache a new material into memory if not so already, otherwise
 * returns an existing material from the cache and adds a reference -
 * reference will need to be released once finished with.
 */
ApeMaterial *ape_material_cache( const char *path, ApeCacheGroup group, bool useFallback );

/**
 * Releases a reference to the material, allowing it to clean up.
 */
void ape_material_release( ApeMaterial *material );

/**
 * Returns the surface type for the material.
 */
int8_t ape_material_get_surface_type( const ApeMaterial *material );

/**
 * Draws the given mesh with the given material. This also updates the peformance tracking,
 * so ideally you should always use this when drawing any mesh.
 */
void ape_material_draw( ApeMaterial *material, PLGMesh *mesh, ApeLight **lights );

unsigned int ape_material_get_flags( const ApeMaterial *self );

/////////////////////////////////////////////////////////////////////////////////////
// Decal Manager
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeDecal        ApeDecal;
typedef struct ApeDecalManager ApeDecalManager;

////////////////////////////////////////////////////////////////////

/**********************************************************/
// Fonts
/**********************************************************/

/** !!!OLD BITMAP API - PREFERABLY AVOID!!! ***************/

typedef struct ApeBitmapFont ApeBitmapFont;

ApeBitmapFont *ss_arl_bitmap_font_cache( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end );
void           ss_arl_bitmap_font_release( ApeBitmapFont *font );

ApeBitmapFont *ape_get_default_small_bitmap_font( void );

void ss_arl_bitmap_font_batch_character( const ApeBitmapFont *font, float x, float y, float scale, QmMathColour4ub colour, uint8_t character );
void ape_bitmap_font_batch_string( const ApeBitmapFont *font, float x, float y, float scale, QmMathColour4ub colour, const char *msg, size_t length, bool shadow );

void ape_bitmap_font_begin_draw( ApeBitmapFont *font );
void ape_bitmap_font_draw( ApeBitmapFont *font );

/**********************************************************/

/////////////////////////////////////////////////////////////////////////////////////
// Draw API

void ape_draw_sprite( ApeMaterial *material, const PLQuad *subRect, const QmMathColour4f *colour, const QmMathVector3f *position, const QmMathVector3f *origin, const QmMathVector3f *angles, float scale );

/**
 * Draws a textured quad. *SLOW* so use sparingly.
 * If material is null, this will draw without passing through the material API.
 */
void ape_draw_textured_quad( ApeMaterial *material, float x, float y, float w, float h, const QmMathColour4ub *colour );

void ape_draw_axis_pivot( QmMathVector3f position, QmMathVector3f rotation, float scale );
void ape_draw_graph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max, ApeGuiFont *font );

// The following are safe to be called during tick, the above are *not*!

/**
 * Draws a basic wireframe line from start to end.
 *
 * @param start 	Point the line starts.
 * @param end 		Point the line ends.
 * @param colour 	Colour of the line.
 */
void ape_draw_debug_line( QmMathVector3f start, QmMathVector3f end, QmMathColour4ub colour );

/**
 * Draw a wireframe arrow from start to end.
 *
 * @param start		Point the line starts.
 * @param end 		Point the line ends. Arrow will point in this direction.
 * @param colour	Colour of the line.
 */
void ape_draw_debug_arrow( QmMathVector3f start, QmMathVector3f end, QmMathColour4ub colour, float scale );

/**
 * Draw a wireframe sphere at the given location.
 *
 * @param origin 	Position of the sphere.
 * @param colour 	Colour of the sphere.
 * @param scale 	Scale of the sphere.
 */
void ape_draw_debug_sphere( QmMathVector3f origin, QmMathColour4ub colour, float scale );

/**
 * Draw an axis at the given position using Euler angles.
 *
 * @param origin 	Position of the axis.
 * @param angles 	Angles of the axis.
 * @param scale		Scale of the axis.
 */
void ape_draw_debug_axis( QmMathVector3f origin, QmMathVector3f angles, float scale );

/**
 * Draw the specified AABB volume at its origin.
 *
 * @param aabb		Pointer to the AABB to draw.
 * @param colour 	Colour of the volume.
 */
void ape_draw_debug_aabb( const PLCollisionAABB *aabb, QmMathColour4ub colour );

/**
 * Draw the specified cylinder at its origin.
 *
 * @param cylinder		Pointer to the cylinder to draw.
 * @param colour		Colour of the cylinder.
 * @param resolution	Resolution of the cylinder.
 */
void ape_draw_debug_cylinder( const ComCollisionCylinder *cylinder, const QmMathColour4ub *colour, unsigned int resolution );

/**
 * Draw a wireframe cone.
 *
 * @param origin Origin of the cone.
 * @param angles Angles of the cone.
 * @param colour Colour of the cone.
 * @param range Range or distance, of the cone.
 * @param radius Radius at the end of the cone.
 * @param resolution Resolution of the cone.
 */
void ape_draw_debug_cone( QmMathVector3f origin, QmMathVector3f angles, const QmMathColour4ub *colour, float range, float radius, unsigned int resolution );

/**
 * Draw the specified plane.
 *
 * @param plane		Plane to draw, will draw from origin.
 * @param colour	Colour of the plane.
 * @param scale
 */
void ape_draw_debug_plane( const PLCollisionPlane *plane, QmMathColour4ub colour, float scale );

/**
 * Draw a polygon derived from the given vertices.
 *
 * @param vertices		An array of vertices.
 * @param numVertices	The number of vertices in the array.
 * @param colour		Colour of the polygon.
 */
void ape_draw_debug_polygon( const QmMathVector3f *vertices, unsigned int numVertices, QmMathColour4ub colour );

/**
 * Queue up a string to display on the screen.
 * Mind this is not for displaying a string in 3D space!
 *
 * @param x X position of the string.
 * @param y Y position of the string.
 * @param z Z position of the string.
 * @param colour Colour to display for the string.
 * @param string String to display.
 * @param ... Variable arguments.
 */
void ape_draw_debug_string( float x, float y, float z, const QmMathColour4ub *colour, const char *string, ... );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

PL_EXTERN_C_END
