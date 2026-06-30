// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include <plgraphics/plg.h>
#include <plgraphics/plg_texture.h>
#include <plgraphics/plg_mesh.h>

#include "ape/ape_public_editor.h"
#include "ape/ape_public_renderer.h"

#include "camera/camera.h"

typedef struct PLHashTable PLHashTable;

typedef struct QmMathPlane QmMathPlane;

typedef struct AuxTexturePackerNode AuxTexturePackerNode;

typedef struct ApeRendererStats
{
	unsigned int numBatches;
	unsigned int numTriangles;
	unsigned int numFacesDrawn;
	unsigned int numVisiblePortals;
	unsigned int numRooms;
	unsigned int numDetailRooms;
	unsigned int numLights;
} ApeRendererStats;
extern ApeRendererStats ape_rendererPerformance_;

/* todo: introduce container around this */
typedef struct ApeSpriteFrame
{
	unsigned int  leftOffset;
	unsigned int  topOffset;
	QmGfxTexture *texture;
} ApeSpriteFrame;

typedef struct ApeTexture      ApeTexture;
typedef struct ApeRenderTarget ApeRenderTarget;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

static constexpr PLGCompareFunction APE_RENDERER_DEFAULT_DEPTH_FUNCTION = PLG_COMPARE_LEQUAL;
static constexpr PLGCullMode        APE_RENDERER_DEFAULT_CULL_FUNCTION  = PLG_CULL_POSITIVE;

/////////////////////////////////////////////////////////////////////////////////////
// Lighting
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeLightGrid     ApeLightGrid;
typedef struct ApeLightGridCell ApeLightGridCell;

/**
 * Create a new light grid.
 * Can be destroyed by calling free - includes destructor.
 */
ApeLightGrid *ape_light_grid_create_( QmMathVector3f mins, QmMathVector3f maxs, QmMathVector3i density );

void                    ape_light_grid_compute_( ApeLightGrid *self, ApeRoom *room, ApeLight **lights, unsigned int numLights );
const ApeLightGridCell *ape_light_grid_sample_cell_( const ApeLightGrid *self, QmMathVector3f position, QmMathColour3f16 *dstColour, QmMathVector3f *dstDir );

/**
 * Attempts to draw the given light grid.
 * Mind this method is *very* expensive, especially if you've got a high density
 * grid.
 */
void ape_light_grid_draw_( const ApeLightGrid *self );

typedef struct __attribute__( ( packed ) ) ApeLightmapPixel
{
	QmMathColour3f16 colour;
	//QmMathVector3f position;
	//QmMathVector3f normal;
} ApeLightmapPixel;

typedef struct ApeLightmap
{
	ApeLightmapPixel     *pixels;
	ApeTexture           *texture;
	AuxTexturePackerNode *packer;
} ApeLightmap;

ApeLightmap *ape_lightmap_create_( unsigned int edgeLength );
void         ape_lightmap_destroy_( ApeLightmap *self );
void         ape_lightmap_upload_( ApeLightmap *self, unsigned int edgeLength );
void         ape_lightmap_serialize_( const ApeLightmap *self, unsigned int edgeLength, AcmBranch *root );
ApeLightmap *ape_lightmap_deserialize_( unsigned int edgeLength, AcmBranch *root );

typedef struct ApeLight
{
	// This should always come first!
	ApeWorldNode base;

	ApeLightType type;

	ApeColour4fProperty colour;
	ApeFloatProperty    radius;// per omni + spotlight
	ApeFloatProperty    angle; // per spotlight

	ApeEnumProperty    flags;
	ApeIntegerProperty flareDeclType;

	int state;

	bool isCacheDirty;
} ApeLight;

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

typedef enum ApeCullMode
{
	APE_RENDERER_CULL_MODE_DEFAULT,
	APE_RENDERER_CULL_MODE_FRONT,
	APE_RENDERER_CULL_MODE_BACK,
	APE_RENDERER_CULL_MODE_NONE,
} ApeCullMode;

typedef enum ApeRendererPassFlag
{
	QM_OS_BIT_FLAG( APE_RENDERER_PASS_FLAG_DEPTH_PREPASS, 0 ),
	QM_OS_BIT_FLAG( APE_RENDERER_PASS_FLAG_OPAQUE, 1 ),
	QM_OS_BIT_FLAG( APE_RENDERER_PASS_FLAG_TRANSLUCENT, 2 ),
	QM_OS_BIT_FLAG( APE_RENDERER_PASS_FLAG_PORTAL, 3 ),
} ApeRendererPassFlag;

typedef struct ApeRendererPassState
{
	ApeCullMode cullMode;// override default cull mode
	//ApeRendererPassFlag passStage;

	PLGBlend blendModeA, blendModeB;
	bool     overrideBlendMode;

	bool depthMask;
	bool overrideDepthMask;

	PLGCompareFunction depthMode;
	bool               overrideDepthMode;

	QmMathColour4f fogColour;
	float          fogNear;
	float          fogFar;

	bool         mirror;
	unsigned int depth;

	ApeCamera *camera;

	struct
	{
		QmMathColour3f   ambience;
		QmMathColour3f16 colour;
		QmMathVector3f   dir;
	} lighting;

	QmGfxTexture *lightmapTexture;
	unsigned int  lightmapIndex;
} ApeRendererPassState;
extern ApeRendererPassState ape_rendererState_;

void ape_renderer_initialize_( void );
void ape_shutdown_renderer_( void );

/**
 * Returns the camera currently being used to draw the scene.
 *
 * @return	Pointer to the currently active camera. Null if no camera active.
 */
ApeCamera *ape_renderer_get_current_camera_();

unsigned int ape_renderer_clip_polygon( const QmMathVector3f *vertices, unsigned int numVertices, const QmMathPlane *plane, QmMathVector3f *dstVertices, unsigned int dstSize );

bool ape_get_capture_state_( void );

void ape_setup_default_draw_state_( const ApeViewport *viewport );

void ape_draw_rectangle_( PLGMesh *mesh, float x, float y, float w, float h, const QmMathColour4ub *colour );
void ape_draw_bevel_rectangle_( PLGMesh *mesh, float x, float y, float w, float h, float depth, const QmMathColour4ub *colour, bool inset );

void ape_setup_2d_viewport_( int w, int h );
void ape_get_2d_viewport_size_( int *width, int *height );

void ape_draw_sprite_animation_frame( ApeSpriteFrame *frame, const QmMathVector3f *position, float spriteAngle );
void ape_draw_sprite_animation( ApeSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const QmMathVector3f *position, float angle );

void ape_add_flare_to_queue( const ApeCamera *camera, const QmMathVector3f *worldPos, const QmMathColour4f *colour, float size, float intensity );
void ape_clear_flare_queue_( void );

/////////////////////////////////////////////////////////////////////////////////////
// Decal Manager
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeDecalManager ApeDecalManager;

ApeDecalManager *ape_decal_manager_create_();
void             ape_decal_manager_destroy_( ApeDecalManager *self );

void ape_decal_manager_deserialize_( ApeDecalManager *self, AcmBranch *root );
void ape_decal_manager_serialize_( ApeDecalManager *self, AcmBranch *root );

void ape_decal_manager_clear_( ApeDecalManager *self );
void ape_decal_manager_tick_( ApeDecalManager *self, double delta );

struct QmOsSharedPtr *ape_decal_manager_create_projected_decal_( ApeDecalManager *self, ApeRoom *room, ApeMaterial *material, const QmMathVector3f *pos, const QmMathVector3f *dir, float angle, float scale, bool isStatic );

void ape_decal_manager_draw_( const ApeDecalManager *self );

/////////////////////////////////////////////////////////////////////////////////////
// Batch Manager
/////////////////////////////////////////////////////////////////////////////////////

void ape_renderer_batch_display_();

/////////////////////////////////////////////////////////////////////////////////////
// Debug Draw
/////////////////////////////////////////////////////////////////////////////////////

void ape_draw_initialize_debug_mesh_();
void ape_draw_destroy_debug_mesh_();
void ape_draw_debug_clear_();
void ape_draw_debug_mesh_display_();

////////////////////////////////////////////////////////////////////
