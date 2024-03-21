// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plgraphics/plg.h>
#include <plgraphics/plg_texture.h>
#include <plgraphics/plg_mesh.h>
#include <plgraphics/plg_camera.h>

#include <yin/core_renderer.h>

#include "renderer_texture.h"

typedef struct ApeRendererStats
{
	PLVector3 cameraPos;

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
	PLGTexture *texture;
} ApeSpriteFrame;

typedef struct ApeCamera
{
	// This should always come first!
	ApeWorldNodeHeader header;

	char tag[ 32 ];

	bool active;

	PLGCamera *internal; /* the camera used for this viewport */

	ApeCameraViewMode mode;
	ApeCameraDrawMode drawMode;

	ApeWorldNode *worldNode;
	ApeWorld *world;
	ApeWorldRoom *room;

	// For visibility
	bool dirty;
	PLVector3 oldPosition, oldAngles;
	PLVectorArray *visibleLights;
	PLVectorArray *visibleRooms;

	PLVector3 forward;// calculated on call to SetCameraAngle
	PLLinkedListNode *node;
} ApeCamera;

typedef struct ApeRenderTarget ApeRenderTarget;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#define SS_ARL_LIGHT_GETTYPE( FLAG ) 	( ( FLAG ) & 0x30U ) >> 4 )
#define SS_ARL_LIGHT_GETSTATE( FLAG )	( ( FLAG ) & 0xF00U ) >> 8 )

typedef struct ArlLightCachedStencilVolume
{

} ArlLightCachedStencilVolume;

#define SS_ARL_MAX_LIGHTS_PER_PASS 8// !! make sure this matches shared.inc.glsl !!
typedef struct ApeLight
{
	// This should always come first!
	ApeWorldNodeHeader header;

	ApeLightType type;

	PLVector3 position;
	PLVector3 angles;
	PLColourF32 colour;
	float radius;

	bool isHidden;

	unsigned int flags;
	int state;

	bool isCacheDirty;

	ApeWorld *world;
} ApeLight;

typedef ApeLight *ApeLightPointerArray[ SS_ARL_MAX_LIGHTS_PER_PASS ];

typedef enum ApeCullMode
{
	SS_ARL_CULL_MODE_DEFAULT,
	SS_ARL_CULL_MODE_FRONT,
	SS_ARL_CULL_MODE_BACK,
	SS_ARL_CULL_MODE_NONE,
} ApeCullMode;

typedef enum ApeRendererPassStage
{
	SS_ARL_RENDERER_PASS_DEFAULT,
	SS_ARL_RENDERER_PASS_DEPTH,
} ApeRendererPassStage;

typedef struct ApeRendererPassState
{
	ApeCullMode cullMode;// override default cull mode
	ApeRendererPassStage passStage;

	PLGBlend blendModeA, blendModeB;
	bool overrideBlendMode;

	bool mirror;
	unsigned int depth;

	ApeCamera *camera;
} ApeRendererPassState;
extern ApeRendererPassState ape_rendererState_;

#include "renderer_material.h"

void ape_initialize_renderer_( void );
void ape_shutdown_renderer_( void );

bool ape_get_capture_state_( void );

void ape_setup_default_draw_state_( const ApeViewport *viewport );
void ape_draw_begin_( ApeViewport *viewport );
void ape_draw_end_( ApeViewport *viewport );
void ape_draw_menu_( ApeViewport *viewport );

void ape_set_2d_viewport_size_( int w, int h );
void ss_arl_get_2d_viewport_size_( int *width, int *height );

struct SS_Arl_ShaderProgramIndex *ape_shader_get_by_name( const char *name );

void ss_arl_draw_sprite_animation_frame( ApeSpriteFrame *frame, const PLVector3 *position, float spriteAngle );
void ss_arl_draw_sprite_animation( ApeSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *ape_texture_load_direct_( const char *path, PLGTextureFilter filterMode );
PLGTexture *ss_arl_texture_get_fallback( void );

////////////////////////////////////////////////////////////////////
