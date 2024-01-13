// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

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
typedef struct SSArlSpriteFrame
{
	unsigned int leftOffset;
	unsigned int topOffset;
	PLGTexture *texture;
} SSArlSpriteFrame;

typedef struct SSArlCamera
{
	char tag[ 32 ];

	bool active;

	PLGCamera *internal; /* the camera used for this viewport */

	SSArlCameraMode mode;
	ApeCameraDrawMode drawMode;

	ApeWorld *world;
	SSAclWorldRoom *room;

	// For visibility
	PLVectorArray *visibleLights;
	PLVectorArray *visibleRooms;

	PLVector3 forward;// calculated on call to SetCameraAngle
	PLLinkedListNode *node;
} SSArlCamera;

typedef struct SSArlRenderTarget SSArlRenderTarget;

////////////////////////////////////////////////////////////////////

#define APE_MAX_FPS_READINGS 128

typedef struct SSArlViewport
{
	unsigned int index;
	int x, y;
	int width, height;

	SSArlCamera *camera;
	SSArlRenderTarget *renderTarget;

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
} SSArlViewport;

////////////////////////////////////////////////////////////////////

#define SS_ARL_LIGHT_GETTYPE( FLAG ) 	( ( FLAG ) & 0x30U ) >> 4 )
#define SS_ARL_LIGHT_GETSTATE( FLAG )	( ( FLAG ) & 0xF00U ) >> 8 )

typedef struct ArlLightCachedStencilVolume
{

} ArlLightCachedStencilVolume;

#define SS_ARL_MAX_LIGHTS_PER_PASS 8// !! make sure this matches shared.inc.glsl !!
typedef struct SSArlLight
{
	SSArlLightType type;

	PLVector3 position;
	PLVector3 angles;
	PLColourF32 colour;
	float radius;

	bool isHidden;

	unsigned int flags;
	int state;

	bool isCacheDirty;

	ApeWorld *world;
} SSArlLight;

typedef SSArlLight *SSArlLightPointerArray[ SS_ARL_MAX_LIGHTS_PER_PASS ];

typedef enum SSArlCullMode
{
	SS_ARL_CULL_MODE_DEFAULT,
	SS_ARL_CULL_MODE_FRONT,
	SS_ARL_CULL_MODE_BACK,
	SS_ARL_CULL_MODE_NONE,
} SSArlCullMode;

typedef enum SSArlRendererPassStage
{
	SS_ARL_RENDERER_PASS_DEFAULT,
	SS_ARL_RENDERER_PASS_DEPTH,
} SSArlRendererPassStage;

typedef struct SSArlRendererPassState
{
	SSArlCullMode cullMode;// override default cull mode
	SSArlRendererPassStage passStage;

	PLGBlend blendModeA, blendModeB;
	bool overrideBlendMode;

	bool mirror;
	unsigned int depth;
} SSArlRendererPassState;
extern SSArlRendererPassState arl_rendererState_;

#include "renderer_material.h"

void ss_arl_initialize_( void );
void ss_arl_shutdown_( void );

PLGCamera *ss_arl_get_aux_camera_( void );

bool ss_arl_get_capture_state_( void );

void ss_arl_setup_default_state( const SSArlViewport *viewport );
void ss_arl_draw_begin_( SSArlViewport *viewport );
void ss_arl_draw_end_( SSArlViewport *viewport );
void ss_arl_draw_menu_( SSArlViewport *viewport );

void ss_arl_set_2d_viewport_size_( int w, int h );
void ss_arl_get_2d_viewport_size_( int *width, int *height );

struct SS_Arl_ShaderProgramIndex *arl_shader_get_by_name( const char *name );

void ss_arl_draw_sprite_animation_frame( SSArlSpriteFrame *frame, const PLVector3 *position, float spriteAngle );
void ss_arl_draw_sprite_animation( SSArlSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *ss_arl_texture_load_direct_( const char *path, PLGTextureFilter filterMode );
PLGTexture *ss_arl_texture_get_fallback( void );

SSArlRenderTarget *ss_arl_get_default_render_target( void );

////////////////////////////////////////////////////////////////////
