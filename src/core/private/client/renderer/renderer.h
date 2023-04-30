// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plgraphics/plg.h>
#include <plgraphics/plg_texture.h>
#include <plgraphics/plg_mesh.h>
#include <plgraphics/plg_camera.h>

#include <yin/core_renderer.h>

#include "renderer_texture.h"

typedef struct YNCoreRendererStats
{
	PLVector3 cameraPos;
	unsigned int numBatches;
	unsigned int numTriangles;
	unsigned int numFacesDrawn;
	unsigned int numVisiblePortals;
} YNCoreRendererStats;
extern YNCoreRendererStats g_gfxPerfStats;

/* todo: introduce container around this */
typedef struct YNCoreSpriteFrame
{
	unsigned int leftOffset;
	unsigned int topOffset;
	PLGTexture *texture;
} YNCoreSpriteFrame;

typedef struct YNCoreCamera
{
	char tag[ 32 ];
	bool active;
	PLGCamera *internal; /* the camera used for this viewport */
	YNCoreCameraMode mode;
	YNCoreCameraDrawMode drawMode;
	struct Actor *parentActor;
	bool enablePostProcessing;
	PLLinkedListNode *node;
} YNCoreCamera;

////////////////////////////////////////////////////////////////////

#define YN_CORE_MAX_FPS_READINGS 64

typedef struct YNCoreViewport
{
	unsigned int index;
	int x, y;
	int width, height;

	YNCoreCamera *camera;

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
} YNCoreViewport;

////////////////////////////////////////////////////////////////////

typedef enum YNCoreLightType
{
	YN_CORE_LIGHT_TYPE_OMNI,
	YN_CORE_LIGHT_TYPE_SPOT,
	YN_CORE_LIGHT_TYPE_SUN,

	YN_CORE_MAX_LIGHT_TYPES
} YNCoreLightType;

#define YN_CORE_MAX_LIGHTS_PER_PASS 8
typedef struct YNCoreLight
{
	YNCoreLightType type;
	PLVector3 position;
	PLVector3 angles;
	PLColourF32 colour;
	float radius;
} YNCoreLight;
typedef YNCoreLight YNCoreLightArray[ YN_CORE_MAX_LIGHTS_PER_PASS ];

typedef struct YNCoreRendererPassState
{
	bool mirror;
	unsigned int depth;
} YNCoreRendererPassState;
extern YNCoreRendererPassState rendererState;

#define YR_NUM_SPRITE_ANGLES 8

#include "renderer_scenegraph.h"
#include "renderer_material.h"

void YnCore_InitializeRenderer( void );
void YnCore_ShutdownRenderer( void );

PLGCamera *YnCore_Rend_GetAuxCamera( void );

void YnCore_SetupRenderTarget( PLGFrameBuffer **buffer, PLGTexture **attachment, PLGTexture **depthAttachment, unsigned int w, unsigned int h );
PLGTexture *YnCore_GetPrimaryColourAttachment( void );
PLGTexture *YnCore_GetPrimaryDepthAttachment( void );

void YnCore_SetupDefaultRenderState( const YNCoreViewport *viewport );
void YnCore_BeginDraw( YNCoreViewport *viewport );
void YnCore_EndDraw( YNCoreViewport *viewport );

void YnCore_Set2DViewportSize( int w, int h );
void YnCore_Get2DViewportSize( int *width, int *height );
void YnCore_DrawMenu( const YNCoreViewport *viewport );

struct YNCoreShaderProgramIndex *YnCore_GetShaderProgramByName( const char *name );

void YnCore_DrawPerspective( YNCoreCamera *camera, const YNCoreViewport *viewport );

void YnCore_Draw2DQuad( YNCoreMaterial *material, int x, int y, int w, int h );
void YnCore_DrawAxesPivot( PLVector3 position, PLVector3 rotation );

void YnCore_Sprite_DrawAnimationFrame( YNCoreSpriteFrame *frame, const PLVector3 *position, float spriteAngle );
void YnCore_Sprite_DrawAnimation( YNCoreSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *YnCore_LoadTexture( const char *path, PLGTextureFilter filterMode );
PLGTexture *YnCore_GetFallbackTexture( void );

#if 0
typedef struct Texture Texture;
Texture               *Renderer_Texture_Load( const char *path );
void                   Renderer_Texture_Release( Texture *texture );
PLGTexture            *Renderer_Texture_GetInternal( Texture *texture );
#endif

////////////////////////////////////////////////////////////////////

typedef struct YNCoreRenderTarget YNCoreRenderTarget;

YNCoreRenderTarget *YnCore_RenderTarget_GetByKey( const char *key );
YNCoreRenderTarget *YnCore_RenderTarget_Create( const char *key, unsigned int width, unsigned int height, unsigned int flags );
void YnCore_RenderTarget_Release( YNCoreRenderTarget *renderTarget );
void YnCore_RenderTarget_SetSize( YNCoreRenderTarget *renderTarget, unsigned int width, unsigned int height );
PLGTexture *YnCore_RenderTarget_GetTextureAttachment( YNCoreRenderTarget *renderTarget );
