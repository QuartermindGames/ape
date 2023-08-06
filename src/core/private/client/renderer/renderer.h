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

typedef enum ApeLightType
{
	APE_LIGHT_TYPE_OMNI,
	APE_LIGHT_TYPE_SPOT,
	APE_LIGHT_TYPE_SUN,

	APE_MAX_LIGHT_TYPES
} ApeLightType;

// GM flags, do not change!!
#define APE_LIGHT_FLAG_DYNAMIC         0x1U
#define APE_LIGHT_FLAG_FADE            0x2U
#define APE_LIGHT_FLAG_SHADOWS         0x4U
#define APE_LIGHT_FLAG_ENABLED         0x8U
#define APE_LIGHT_FLAG_RUNTIME_SHADOWS 0x2000U

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
} ApeLight;

typedef ApeLight ApeLightArray[ APE_MAX_LIGHTS_PER_PASS ];
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

void apeInitializeRenderer_( void );
void apeShutdownRenderer_( void );

PLGCamera *apeGetAuxCamera( void );

void apeSetupRenderTarget( PLGFrameBuffer **buffer, PLGTexture **attachment, PLGTexture **depthAttachment, unsigned int w, unsigned int h );
PLGTexture *apeGetPrimaryColourAttachment( void );
PLGTexture *apeGetPrimaryDepthAttachment( void );

void apeSetupDefaultRenderState( const ApeViewport *viewport );
void apeBeginDraw( ApeViewport *viewport );
void apeEndDraw( ApeViewport *viewport );

void apeSet2DViewportSize( int w, int h );
void apeGet2DViewportSize( int *width, int *height );
void apeDrawMenu( const ApeViewport *viewport );

struct ApeShaderProgramIndex *apeGetShaderProgramByName( const char *name );

void apeDrawPerspective_( ApeCamera *camera, ApeViewport *viewport );

void apeDraw2DQuad( ApeMaterial *material, int x, int y, int w, int h, const PLColour *colour );
void apeDrawAxesPivot( PLVector3 position, PLVector3 rotation, float scale );

void apeDrawSpriteAnimationFrame( ApeSpriteFrame *frame, const PLVector3 *position, float spriteAngle );
void apeDrawSpriteAnimation( ApeSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *apeLoadTexture( const char *path, PLGTextureFilter filterMode );
PLGTexture *apeGetFallbackTexture( void );

#if 0
typedef struct Texture Texture;
Texture               *Renderer_Texture_Load( const char *path );
void                   Renderer_Texture_Release( Texture *texture );
PLGTexture            *Renderer_Texture_GetInternal( Texture *texture );
#endif

////////////////////////////////////////////////////////////////////

typedef struct ApeRenderTarget ApeRenderTarget;

ApeRenderTarget *apeGetRenderTargetByKey( const char *key );
ApeRenderTarget *apeCreateRenderTarget( const char *key, unsigned int width, unsigned int height, unsigned int flags, unsigned int textureAttachmentComponent, PLGTextureFilter textureAttachmentFilter );
void apeReleaseRenderTarget( ApeRenderTarget *renderTarget );
void apeSetRenderTargetSize( ApeRenderTarget *renderTarget, unsigned int width, unsigned int height );
PLGTexture *apeGetRenderTargetTextureAttachment( ApeRenderTarget *renderTarget );
void apeBindRenderTarget( ApeRenderTarget *renderTarget, PLGFrameBufferObjectTarget target );
