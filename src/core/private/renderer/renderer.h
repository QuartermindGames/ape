// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plgraphics/plg.h>
#include <plgraphics/plg_texture.h>
#include <plgraphics/plg_mesh.h>

#include "ape/ape_public_renderer.h"

#include "camera/camera.h"

typedef struct PLHashTable PLHashTable;

typedef struct ComMathPlane ComMathPlane;

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
	unsigned int leftOffset;
	unsigned int topOffset;
	PLGTexture  *texture;
} ApeSpriteFrame;

typedef struct ApeRenderTarget ApeRenderTarget;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

static constexpr unsigned int APE_LIGHTMAP_SIZE = 64;

typedef struct __attribute__( ( packed ) ) ApeLightmapPixel
{
	unsigned char r, g, b;

	PLVector3 position;
	PLVector3 normal;
} ApeLightmapPixel;

#define APE_MAX_LIGHTS_PER_PASS 8// !! make sure this matches shared.inc.glsl !!
typedef struct ApeLight
{
	// This should always come first!
	ApeWorldNode base;

	ApeLightType type;

	PLColourF32 colour;
	float       radius;// per omni + spotlight
	float       angle; // per spotlight

	unsigned int flags;
	int          state;

	ApeLightmapPixel *lightmap;

	bool isCacheDirty;
} ApeLight;

typedef ApeLight *ApeLightPointerArray[ APE_MAX_LIGHTS_PER_PASS ];

typedef enum ApeCullMode
{
	APE_RENDERER_CULL_MODE_DEFAULT,
	APE_RENDERER_CULL_MODE_FRONT,
	APE_RENDERER_CULL_MODE_BACK,
	APE_RENDERER_CULL_MODE_NONE,
} ApeCullMode;

typedef enum ApeRendererPassFlag
{
	PL_BITFLAG( APE_RENDERER_PASS_FLAG_DEPTH_PREPASS, 0 ),
	PL_BITFLAG( APE_RENDERER_PASS_FLAG_OPAQUE, 1 ),
	PL_BITFLAG( APE_RENDERER_PASS_FLAG_TRANSLUCENT, 2 ),
	PL_BITFLAG( APE_RENDERER_PASS_FLAG_PORTAL, 3 ),
} ApeRendererPassFlag;

typedef struct ApeRendererPassState
{
	ApeCullMode cullMode;// override default cull mode
	//ApeRendererPassFlag passStage;

	PLColourF32 ambience;

	PLGBlend blendModeA, blendModeB;
	bool     overrideBlendMode;

	PLGCompareFunction depthMode;
	bool               overrideDepthMode;

	bool         mirror;
	unsigned int depth;

	ApeCamera *camera;
} ApeRendererPassState;
extern ApeRendererPassState ape_rendererState_;

void ape_initialize_renderer_( void );
void ape_shutdown_renderer_( void );

/**
 * Returns the camera currently being used to draw the scene.
 *
 * @return	Pointer to the currently active camera. Null if no camera active.
 */
ApeCamera *ape_renderer_get_current_camera_();

unsigned int ape_renderer_clip_polygon( const PLVector3 *vertices, unsigned int numVertices, const ComMathPlane *plane, PLVector3 *dstVertices, unsigned int dstSize );

bool ape_get_capture_state_( void );

void ape_setup_default_draw_state_( const ApeViewport *viewport );
void ape_draw_begin_( ApeViewport *viewport );
void ape_draw_end_( ApeViewport *viewport );
void ape_draw_menu_( ApeViewport *viewport );

void ape_set_2d_viewport_size_( int w, int h );
void ape_get_2d_viewport_size_( int *width, int *height );

void ape_draw_sprite_animation_frame( ApeSpriteFrame *frame, const PLVector3 *position, float spriteAngle );
void ape_draw_sprite_animation( ApeSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *ape_texture_load_direct_( const char *path, PLGTextureFilter filterMode );
PLGTexture *ape_texture_get_fallback( void );

void ape_add_flare_to_queue( const ApeCamera *camera, const PLVector3 *worldPos, const PLColourF32 *colour, float size, float intensity );
void ape_clear_flare_queue_( void );

/////////////////////////////////////////////////////////////////////////////////////
// Decal Manager
/////////////////////////////////////////////////////////////////////////////////////

ApeDecalManager *ape_decal_manager_create_();
void             ape_decal_manager_destroy_( ApeDecalManager *self );

void ape_decal_manager_deserialize_( ApeDecalManager *self, AcmBranch *root );
void ape_decal_manager_serialize_( ApeDecalManager *self, AcmBranch *root );

void ape_decal_manager_clear_( ApeDecalManager *self );
void ape_decal_manager_tick_( ApeDecalManager *self, double delta );

struct ComSharedPtr *ape_decal_manager_create_decal_( ApeDecalManager *self, ApeBrushFace *face, ApeMaterial *material, const PLVector3 *pos, float angle, float scale );
struct ComSharedPtr *ape_decal_manager_create_projected_decal_( ApeDecalManager *self, ApeRoom *room, ApeMaterial *material, const PLVector3 *pos, const PLVector3 *dir, float angle, float scale );

void ape_decal_manager_draw_( const ApeDecalManager *self );

/////////////////////////////////////////////////////////////////////////////////////
// Debug Draw
/////////////////////////////////////////////////////////////////////////////////////

void ape_draw_initialize_debug_mesh_();
void ape_draw_destroy_debug_mesh_();
void ape_draw_debug_clear_();
void ape_draw_debug_mesh_display_();

////////////////////////////////////////////////////////////////////
