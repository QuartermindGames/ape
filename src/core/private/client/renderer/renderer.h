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

#define APE_MAX_LIGHTS_PER_PASS 8// !! make sure this matches shared.inc.glsl !!
typedef struct ApeLight
{
	ApeLightType type;

	PLVector3 position;
	PLVector3 angles;
	PLColourF32 colour;
	float radius;

	bool isHidden;

	uint32_t flags;

	int32_t state;

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
	APE_RENDERER_PASS_STENCIL,
	APE_RENDERER_PASS_LIGHTING,
} ApeRendererPassStage;

typedef struct ApeRendererPassState
{
	ApeRendererCullMode cullMode;// override default cull mode
	ApeRendererPassStage passStage;

	PLGBlend blendModeA, blendModeB;
	bool overrideBlendMode;

	bool mirror;
	unsigned int depth;
} ApeRendererPassState;
extern ApeRendererPassState rendererState;

#define APE_NUM_SPRITE_ANGLES 8

#include "renderer_material.h"

void ar_initialize_( void );
void ar_shutdown_( void );

PLGCamera *apeGetAuxCamera( void );

//void apeSetupRenderTarget( PLGFrameBuffer **buffer, PLGTexture **attachment, PLGTexture **depthAttachment, unsigned int w, unsigned int h );
//PLGTexture *apeGetPrimaryColourAttachment( void );
//PLGTexture *apeGetPrimaryDepthAttachment( void );

void ar_setup_default_state( const ApeViewport *viewport );
void ar_draw_begin( ApeViewport *viewport );
void ar_draw_end( ApeViewport *viewport );
void ar_draw_menu( const ApeViewport *viewport );

void apeSet2DViewportSize( int w, int h );
void apeGet2DViewportSize( int *width, int *height );

struct ApeShaderProgramIndex *apeGetShaderProgramByName( const char *name );

void apeDrawPerspective_( ApeCamera *camera, ApeViewport *viewport );

void ar_draw_quad( ApeMaterial *material, int x, int y, int w, int h, const PLColour *colour );
void ar_draw_axis_pivot( PLVector3 position, PLVector3 rotation, float scale );
void ar_draw_graph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max );

void apeDrawSpriteAnimationFrame( ApeSpriteFrame *frame, const PLVector3 *position, float spriteAngle );
void apeDrawSpriteAnimation( ApeSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *apeLoadTexture( const char *path, PLGTextureFilter filterMode );
PLGTexture *apeGetFallbackTexture( void );

ArRenderTarget *ar_get_default_render_target( void );

////////////////////////////////////////////////////////////////////
