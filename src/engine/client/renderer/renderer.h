/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

/* todo: introduce container around this */
typedef struct SprAnimationFrame
{
	unsigned int leftOffset;
	unsigned int topOffset;
	PLGTexture * texture;
} SprAnimationFrame;

typedef enum ViewPerspective
{
	VIEW_PERSPECTIVE_EYE,
	VIEW_PERSPECTIVE_TOP,

	MAX_VIEW_PERSPECTIVES
} ViewPerspective;

typedef struct GfxCamera
{
	PLGCamera *              internalPtr; /* the camera used for this viewport */
	ViewPerspective          perspective;
	struct Actor *           parentActor;
	struct PLLinkedListNode *node; /* node representing this object in the linked list */
} GfxCamera;

typedef struct RendererStats
{
	PLVector3    cameraPos;
	unsigned int numBatches;
	unsigned int numFacesDrawn;
} RendererStats;
extern RendererStats g_gfxPerfStats;

#define RS_PROGRAM_NAME_LENGTH 64

enum
{
	RS_SHADER_DEFAULT,
	RS_SHADER_LIGHTING_PASS,
	RS_SHADER_DEFAULT_VERTEX,
	RS_SHADER_DEFAULT_ALPHA,
	RS_SHADER_POST_PROCESS,

	RS_MAX_DEFAULT_SHADERS
};
extern PLGShaderProgram *defaultShaderPrograms[ RS_MAX_DEFAULT_SHADERS ];

#define GFX_NUM_SPRITE_ANGLES 8

#include "scenegraph.h"
#include "material.h"

void R_Initialize( void );
void R_Shutdown( void );

void R_SetupDefaultState( void );
void R_DrawMenu( void );

GfxCamera *Gfx_CreateCamera( ViewPerspective perspective, PLVector3 position, PLVector3 angles );

PLGShaderProgram *RS_GetShaderProgram( const char *name );

void R_DrawPerspective( GfxCamera *camera );
void R_DrawAxesPivot( PLVector3 position, PLVector3 rotation );

void RSpr_DrawAnimationFrame( SprAnimationFrame *frame, const PLVector3 *position, float spriteAngle );
void RSpr_DrawAnimation( SprAnimationFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *R_LoadTexture( const char *path );
PLGTexture *R_GetFallbackTexture( void );
