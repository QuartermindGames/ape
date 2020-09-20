/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

/* todo: introduce container around this */
typedef struct GfxAnimationFrame {
	unsigned int leftOffset;
	unsigned int topOffset;
	PLTexture    *texture;
} GfxAnimationFrame;

typedef enum ViewPerspective {
	VIEW_PERSPECTIVE_EYE,

	/* editor modes */
	VIEW_PERSPECTIVE_TOP,
	VIEW_PERSPECTIVE_SIDE,
	VIEW_PERSPECTIVE_FRONT,

	MAX_VIEW_PERSPECTIVES
} ViewPerspective;

typedef struct GfxCamera {
	SysWindow               *viewportPtr;
	PLCamera			    *cameraPtr;			/* the camera used for this viewport */
	ViewPerspective		    perspective;
	struct Actor		    *parentActor;
	struct PLLinkedListNode *node;				/* node representing this object in the linked list */
} GfxCamera;

struct {
	PLVector3 cameraPos;
	unsigned int numFacesDrawn;
} g_gfxPerfStats;

#define GfxPerfStart( A )

#define GFX_PROGRAM_NAME_LENGTH 64

enum {
	GFX_SHADER_DEFAULT,
	GFX_SHADER_LIGHTING_PASS,
	GFX_SHADER_DEFAULT_VERTEX,
	GFX_SHADER_DEFAULT_ALPHA,

	GFX_MAX_DEFAULT_SHADERS
};
extern PLShaderProgram *gfxDefaultShaderPrograms[ GFX_MAX_DEFAULT_SHADERS ];

#define GFX_NUM_SPRITE_ANGLES 8

#include "material.h"

void Gfx_Initialize( void );
void Gfx_Shutdown( void );
void Gfx_SetupDefaultState( void );
void Gfx_DrawMenu( void );

GfxCamera *Gfx_CreateCamera( ViewPerspective perspective, PLVector3 position, PLVector3 angles, SysWindow *viewport );

PLShaderProgram *Gfx_GetShaderProgram( const char *name );

void Gfx_DrawPerspective( GfxCamera *camera );
void Gfx_DrawAxesPivot( PLVector3 position, PLVector3 rotation );
void Gfx_DrawAnimationFrame( GfxAnimationFrame *frame, const PLVector3 *position, float spriteAngle );
void Gfx_DrawAnimation( GfxAnimationFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

PLTexture *Gfx_LoadTexture( const char *path );

PLTexture *Gfx_GetFallbackTexture( void );

const char *Gfx_GetPerspectiveDescription( ViewPerspective perspective );

void Gfx_DrawCharacter( PLTexture *baseTexture, char character, float x, float y, float scale );
void Gfx_DrawString( PLTexture *baseTexture, const char *string, float x, float y, float scale );
