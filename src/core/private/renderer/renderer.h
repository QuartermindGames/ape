// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plgraphics/plg.h>
#include <plgraphics/plg_texture.h>
#include <plgraphics/plg_mesh.h>
#include <plgraphics/plg_camera.h>

#include <yin/core_renderer.h>

#include "renderer_texture.h"

typedef struct PLHashTable PLHashTable;

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

#define APE_CAMERA_MAX_ROOM_VISITS    4  // Maximum number of times we can visit the same room. TODO: hook up to var
#define APE_CAMERA_MAX_VISIBLE_ROOMS  256// we'll go through 256 portals maximum (maybe hook this to a var)
#define APE_CAMERA_MAX_VISIBLE_LIGHTS 512//TODO: hook up to var

typedef struct ApeCameraVisibleSet
{
	bool      dirty;
	PLVector3 oldPosition, oldAngles;

	ApeLight    *lights[ APE_CAMERA_MAX_VISIBLE_LIGHTS ];
	unsigned int numLights;

	PLVectorArray *nodes;         //ApeWorldNode
	PLVectorArray *visibleFaces;  //ApeBrushFace
	PLVectorArray *visiblePortals;//ApeBrushFace

	struct
	{
		PLMatrix4    transform;
		unsigned int numVisits;
		ApeRoom     *room;
	} rooms[ APE_CAMERA_MAX_VISIBLE_ROOMS ];
	unsigned int numRooms;
	PLHashTable *visitedRooms;
} ApeCameraVisibleSet;

typedef struct ApeCamera
{
	// This should always come first!
	ApeWorldNode base;

	char tag[ 32 ];

	bool active;

	PLGCamera *internal; /* the camera used for this viewport */

	ApeCameraViewMode mode;
	ApeCameraDrawMode drawMode;

	ApeCameraVisibleSet pvs;

	/////////////////////////////////////////////////////////////////////////////////////

	PLLinkedListNode *node;
} ApeCamera;

typedef struct ApeRenderTarget ApeRenderTarget;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#define APE_MAX_LIGHTS_PER_PASS 8// !! make sure this matches shared.inc.glsl !!
typedef struct ApeLight
{
	// This should always come first!
	ApeWorldNode base;

	ApeLightType type;

	PLColourF32 colour;
	float       radius;

	bool isHidden;

	unsigned int flags;
	int          state;

	bool isCacheDirty;

	ApeWorld *world;
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

	bool         mirror;
	unsigned int depth;

	ApeCamera *camera;
} ApeRendererPassState;
extern ApeRendererPassState ape_rendererState_;

#include "material/material.h"

void ape_initialize_renderer_( void );
void ape_shutdown_renderer_( void );

/**
 * Returns the camera currently being used to draw the scene.
 *
 * @return	Pointer to the currently active camera. Null if no camera active.
 */
ApeCamera *ape_renderer_get_current_camera_();

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
// Debug Draw
/////////////////////////////////////////////////////////////////////////////////////

void ape_draw_initialize_debug_mesh_();
void ape_draw_destroy_debug_mesh_();
void ape_draw_debug_clear_();
void ape_draw_debug_mesh_display_();

////////////////////////////////////////////////////////////////////
