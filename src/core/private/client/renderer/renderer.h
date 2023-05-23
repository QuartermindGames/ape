// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plgraphics/plg.h>
#include <plgraphics/plg_texture.h>
#include <plgraphics/plg_mesh.h>
#include <plgraphics/plg_camera.h>

#include <yin/core_renderer.h>

#include "renderer_texture.h"

typedef struct OgeRendererStats
{
	PLVector3 cameraPos;
	unsigned int numBatches;
	unsigned int numTriangles;
	unsigned int numFacesDrawn;
	unsigned int numVisiblePortals;
} OgeRendererStats;
extern OgeRendererStats oge_RendererPerformance_;

/* todo: introduce container around this */
typedef struct OgeSpriteFrame
{
	unsigned int leftOffset;
	unsigned int topOffset;
	PLGTexture *texture;
} OgeSpriteFrame;

typedef struct OgeCamera
{
	char tag[ 32 ];
	bool active;
	PLGCamera *internal; /* the camera used for this viewport */
	OgeCameraMode mode;
	OgeCameraDrawMode drawMode;
	struct Actor *parentActor;
	bool enablePostProcessing;
	PLLinkedListNode *node;
} OgeCamera;

////////////////////////////////////////////////////////////////////

#define YN_CORE_MAX_FPS_READINGS 64

typedef struct OgeViewport
{
	unsigned int index;
	int x, y;
	int width, height;

	OgeCamera *camera;

	struct
	{
		double frameTime, oldTime;
		double frameReadings[ YN_CORE_MAX_FPS_READINGS ];
		unsigned int frameIndex;

		unsigned int numBatches;
		unsigned int numTriangles;
		unsigned int numPolygons;
		unsigned int numPortals;
	} perf;

	void *windowHandle;
} OgeViewport;

////////////////////////////////////////////////////////////////////

typedef enum OgeLightType
{
	OGE_LIGHT_TYPE_OMNI,
	OGE_LIGHT_TYPE_SPOT,
	OGE_LIGHT_TYPE_SUN,

	OGE_MAX_LIGHT_TYPES
} OgeLightType;

#define OGE_MAX_LIGHTS_PER_PASS 8
typedef struct OgeLight
{
	OgeLightType type;
	PLVector3 position;
	PLVector3 angles;
	PLColourF32 colour;
	float radius;
} OgeLight;
typedef OgeLight OgeLightArray[ OGE_MAX_LIGHTS_PER_PASS ];

typedef struct OgeRendererPassState
{
	bool mirror;
	unsigned int depth;
} OgeRendererPassState;
extern OgeRendererPassState rendererState;

#define OGE_NUM_SPRITE_ANGLES 8

#include "renderer_scenegraph.h"
#include "renderer_material.h"

void ogeInitializeRenderer( void );
void ogeShutdownRenderer( void );

PLGCamera *ogeGetAuxCamera( void );

void ogeSetupRenderTarget( PLGFrameBuffer **buffer, PLGTexture **attachment, PLGTexture **depthAttachment, unsigned int w, unsigned int h );
PLGTexture *ogeGetPrimaryColourAttachment( void );
PLGTexture *ogeGetPrimaryDepthAttachment( void );

void YnCore_SetupDefaultRenderState( const OgeViewport *viewport );
void YnCore_BeginDraw( OgeViewport *viewport );
void YnCore_EndDraw( OgeViewport *viewport );

void YnCore_Set2DViewportSize( int w, int h );
void ogeGet2DViewportSize( int *width, int *height );
void YnCore_DrawMenu( const OgeViewport *viewport );

struct OgeShaderProgramIndex *ogeGetShaderProgramByName( const char *name );

void ogeDrawPerspective_( OgeCamera *camera, const OgeViewport *viewport );

void YnCore_Draw2DQuad( OgeMaterial *material, int x, int y, int w, int h );
void YnCore_DrawAxesPivot( PLVector3 position, PLVector3 rotation );

void YnCore_Sprite_DrawAnimationFrame( OgeSpriteFrame *frame, const PLVector3 *position, float spriteAngle );
void YnCore_Sprite_DrawAnimation( OgeSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *ogeLoadTexture( const char *path, PLGTextureFilter filterMode );
PLGTexture *ogeGetFallbackTexture( void );

#if 0
typedef struct Texture Texture;
Texture               *Renderer_Texture_Load( const char *path );
void                   Renderer_Texture_Release( Texture *texture );
PLGTexture            *Renderer_Texture_GetInternal( Texture *texture );
#endif

////////////////////////////////////////////////////////////////////

typedef struct OgeRenderTarget OgeRenderTarget;

OgeRenderTarget *ogeRenderTarget_GetByKey( const char *key );
OgeRenderTarget *ogeRenderTarget_Create( const char *key, unsigned int width, unsigned int height, unsigned int flags );
void ogeRenderTarget_Release( OgeRenderTarget *renderTarget );
void ogeRenderTarget_SetSize( OgeRenderTarget *renderTarget, unsigned int width, unsigned int height );
PLGTexture *ogeRenderTarget_GetTextureAttachment( OgeRenderTarget *renderTarget );
