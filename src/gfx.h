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

#define GFX_NUM_SPRITE_ANGLES 8

void Gfx_Initialize( void );
void Gfx_Shutdown( void );
void Gfx_Display( void );

const PLCamera *Gfx_GetCurrentCamera( void );

void Gfx_SetViewportSize( int width, int height );

void Gfx_EnableShaderProgram( GfxShaderType type );

void Gfx_DrawAxesPivot( PLVector3 position, PLVector3 rotation );
void Gfx_DrawAnimationFrame( GfxAnimationFrame *frame, const PLVector3 *position, float spriteAngle );
void Gfx_DrawAnimation( GfxAnimationFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle );

void Gfx_LoadAnimationFrames( const char **frameList, GfxAnimationFrame **destination, unsigned int numFrames );

PLTexture *Gfx_GetWallTexture( unsigned int index );
PLTexture *Gfx_GetFloorTexture( unsigned int index );
