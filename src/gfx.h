/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

typedef enum GfxShaderType {
	SHADER_GENERIC,
	SHADER_TEXTURE,
	SHADER_ALPHA_TEST,
	SHADER_LIT,

	MAX_SHADER_TYPES
} GfxShaderType;

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

#define GFX_NUM_SPRITE_ANGLES 8

void Gfx_Initialize( void );
void Gfx_Shutdown( void );
void Gfx_SetupDefaultState( void );
void Gfx_DisplayMenu( void );

GfxCamera *Gfx_CreateCamera( ViewPerspective perspective, PLVector3 position, PLVector3 angles, SysWindow *viewport );

void Gfx_EnableShaderProgram( GfxShaderType type );

void Gfx_DrawPerspective( GfxCamera *camera );
void Gfx_DrawAxesPivot( PLVector3 position, PLVector3 rotation );
void Gfx_DrawAnimationFrame( GfxAnimationFrame *frame, const PLVector3 *position, float spriteAngle );
void Gfx_DrawAnimation( GfxAnimationFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

void Gfx_LoadAnimationFrames( const char **frameList, GfxAnimationFrame **destination, unsigned int numFrames );

PLTexture *Gfx_GetWallTexture( unsigned int index );
PLTexture *Gfx_GetFloorTexture( unsigned int index );
PLTexture *Gfx_GetFallbackTexture( void );
