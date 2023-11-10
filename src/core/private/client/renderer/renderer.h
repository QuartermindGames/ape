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
typedef struct ApeSpriteFrame
{
	unsigned int leftOffset;
	unsigned int topOffset;
	PLGTexture *texture;
} ApeSpriteFrame;

typedef struct ApeCamera
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
} ApeCamera;

typedef struct ArRenderTarget ArRenderTarget;

////////////////////////////////////////////////////////////////////

#define APE_MAX_FPS_READINGS 64

typedef struct ApeViewport
{
	unsigned int index;
	int x, y;
	int width, height;

	ApeCamera *camera;

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
} ApeViewport;

////////////////////////////////////////////////////////////////////

#define APE_LIGHT_GETTYPE( FLAG ) 	( ( FLAG ) & 0x30U ) >> 4 )
#define APE_LIGHT_GETSTATE( FLAG )	( ( FLAG ) & 0xF00U ) >> 8 )

typedef struct ArlLightCachedStencilVolume
{

} ArlLightCachedStencilVolume;

#define APE_MAX_LIGHTS_PER_PASS 8// !! make sure this matches shared.inc.glsl !!
typedef struct ApeLight
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
} ApeLight;

typedef ApeLight *ApeLightPointerArray[ APE_MAX_LIGHTS_PER_PASS ];

typedef enum ApeRendererCullMode
{
	APE_RENDERER_CULL_DEFAULT,
	APE_RENDERER_CULL_FRONT,
	APE_RENDERER_CULL_BACK,
	APE_RENDERER_CULL_NONE,
} ApeRendererCullMode;

typedef enum ApeRendererPassStage
{
	APE_RENDERER_PASS_DEFAULT,
	APE_RENDERER_PASS_DEPTH,
} ApeRendererPassStage;

typedef struct ArlRendererPassState
{
	ApeRendererCullMode cullMode;// override default cull mode
	ApeRendererPassStage passStage;

	PLGBlend blendModeA, blendModeB;
	bool overrideBlendMode;

	bool mirror;
	unsigned int depth;
} ArlRendererPassState;
extern ArlRendererPassState arl_rendererState_;

#define APE_NUM_SPRITE_ANGLES 8

#include "renderer_material.h"

void ar_initialize_( void );
void arl_shutdown_( void );

PLGCamera *apeGetAuxCamera( void );

//void apeSetupRenderTarget( PLGFrameBuffer **buffer, PLGTexture **attachment, PLGTexture **depthAttachment, unsigned int w, unsigned int h );
//PLGTexture *apeGetPrimaryColourAttachment( void );
//PLGTexture *apeGetPrimaryDepthAttachment( void );

void arl_setup_default_state( const ApeViewport *viewport );
void arl_draw_begin_( ApeViewport *viewport );
void arl_draw_end_( ApeViewport *viewport );
void arl_draw_menu_( const ApeViewport *viewport );

void apeSet2DViewportSize( int w, int h );
void apeGet2DViewportSize( int *width, int *height );

struct ApeShaderProgramIndex *arl_shader_get_by_name( const char *name );

void arl_camera_draw_perspective_( ApeCamera *camera, ApeViewport *viewport );

void arl_draw_quad( ApeMaterial *material, int x, int y, int w, int h, const PLColour *colour );
void arl_draw_axis_pivot( PLVector3 position, PLVector3 rotation, float scale );
void arl_draw_graph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max );

void arl_draw_sprite_animation_frame( ApeSpriteFrame *frame, const PLVector3 *position, float spriteAngle );
void arl_draw_sprite_animation( ApeSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *arl_texture_load_direct_( const char *path, PLGTextureFilter filterMode );
PLGTexture *arl_texture_get_fallback( void );

ArRenderTarget *arl_get_default_render_target( void );

////////////////////////////////////////////////////////////////////
