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
typedef struct SS_Arl_SpriteFrame
{
	unsigned int leftOffset;
	unsigned int topOffset;
	PLGTexture *texture;
} SS_Arl_SpriteFrame;

typedef struct SS_Arl_Camera
{
	char tag[ 32 ];
	bool active;
	PLGCamera *internal; /* the camera used for this viewport */
	ApeCameraMode mode;
	ApeCameraDrawMode drawMode;
	ApeWorldRoom *room;
	struct Actor *parentActor;
	bool enablePostProcessing;
	PLVector3 forward;// calculated on call to SetCameraAngle
	PLLinkedListNode *node;
} SS_Arl_Camera;

typedef struct ArRenderTarget ArRenderTarget;

////////////////////////////////////////////////////////////////////

#define APE_MAX_FPS_READINGS 64

typedef struct SS_Arl_Viewport
{
	unsigned int index;
	int x, y;
	int width, height;

	SS_Arl_Camera *camera;

	struct
	{
		double frameTime, oldTime;
		double frameReadings[ APE_MAX_FPS_READINGS ];
		unsigned int frameIndex;

		unsigned int numBatches;
		unsigned int numTriangles;
		unsigned int numPolygons;
		unsigned int numPortals;
	} perf;

	void *windowHandle;
} SS_Arl_Viewport;

////////////////////////////////////////////////////////////////////

#define APE_LIGHT_GETTYPE( FLAG ) 	( ( FLAG ) & 0x30U ) >> 4 )
#define APE_LIGHT_GETSTATE( FLAG )	( ( FLAG ) & 0xF00U ) >> 8 )

typedef struct ArlLightCachedStencilVolume
{

} ArlLightCachedStencilVolume;

#define APE_MAX_LIGHTS_PER_PASS 8// !! make sure this matches shared.inc.glsl !!
typedef struct SS_Arl_Light
{
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
} SS_Arl_Light;

typedef SS_Arl_Light *SS_Arl_LightPointerArray[ APE_MAX_LIGHTS_PER_PASS ];

typedef enum SS_Arl_CullMode
{
	SS_ARL_CULL_MODE_DEFAULT,
	SS_ARL_CULL_MODE_FRONT,
	SS_ARL_CULL_MODE_BACK,
	SS_ARL_CULL_MODE_NONE,
} SS_Arl_CullMode;

typedef enum ApeRendererPassStage
{
	APE_RENDERER_PASS_DEFAULT,
	APE_RENDERER_PASS_DEPTH,
} ApeRendererPassStage;

typedef struct SS_Arl_RendererPassState
{
	SS_Arl_CullMode cullMode;// override default cull mode
	ApeRendererPassStage passStage;

	PLGBlend blendModeA, blendModeB;
	bool overrideBlendMode;

	bool mirror;
	unsigned int depth;
} SS_Arl_RendererPassState;
extern SS_Arl_RendererPassState arl_rendererState_;

#define APE_NUM_SPRITE_ANGLES 8

#include "renderer_material.h"

void ss_arl_initialize_( void );
void ss_arl_shutdown_( void );

PLGCamera *ss_arl_get_aux_camera_( void );

//void apeSetupRenderTarget( PLGFrameBuffer **buffer, PLGTexture **attachment, PLGTexture **depthAttachment, unsigned int w, unsigned int h );
//PLGTexture *apeGetPrimaryColourAttachment( void );
//PLGTexture *apeGetPrimaryDepthAttachment( void );

bool ss_arl_get_capture_state_( void );

void ss_arl_setup_default_state( const SS_Arl_Viewport *viewport );
void ss_arl_draw_begin_( SS_Arl_Viewport *viewport );
void ss_arl_draw_end_( SS_Arl_Viewport *viewport );
void ss_arl_draw_menu_( const SS_Arl_Viewport *viewport );

void apeSet2DViewportSize( int w, int h );
void apeGet2DViewportSize( int *width, int *height );

struct SS_Arl_ShaderProgramIndex *arl_shader_get_by_name( const char *name );

void arl_camera_draw_perspective_( SS_Arl_Camera *camera, SS_Arl_Viewport *viewport );

void arl_draw_quad( ApeMaterial *material, int x, int y, int w, int h, const PLColour *colour );
void arl_draw_axis_pivot( PLVector3 position, PLVector3 rotation, float scale );
void arl_draw_graph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max );

void arl_draw_sprite_animation_frame( SS_Arl_SpriteFrame *frame, const PLVector3 *position, float spriteAngle );
void arl_draw_sprite_animation( SS_Arl_SpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *arl_texture_load_direct_( const char *path, PLGTextureFilter filterMode );
PLGTexture *arl_texture_get_fallback( void );

ArRenderTarget *arl_get_default_render_target( void );

////////////////////////////////////////////////////////////////////
