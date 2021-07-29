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

typedef enum CameraMode
{
	CAMERA_MODE_EYE,
	CAMERA_MODE_TOPDOWN,

	MAX_CAMERA_MODES
} CameraMode;

typedef struct Camera
{
	PLGCamera *   internal; /* the camera used for this viewport */
	CameraMode    followMode;
	struct Actor *parentActor;
} Camera;

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

Camera *R_GetGlobalCamera( void );

Camera *R_CreateCamera( const PLVector3 *position, const PLVector3 *angles );
void R_DestroyCamera( Camera *camera );

PLGShaderProgram *RS_GetShaderProgram( const char *name );

void R_DrawPerspective( Camera *camera );
void R_DrawAxesPivot( PLVector3 position, PLVector3 rotation );

void RSpr_DrawAnimationFrame( SprAnimationFrame *frame, const PLVector3 *position, float spriteAngle );
void RSpr_DrawAnimation( SprAnimationFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLGTexture *RT_LoadTexture( const char *path );
PLGTexture *RT_GetFallbackTexture( void );
