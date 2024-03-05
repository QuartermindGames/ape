// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "renderer.h"

void ss_arl_draw_sprite_animation_frame( ApeSpriteFrame *frame, const PLVector3 *position, float spriteAngle )
{
#if 0
    PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlRotateMatrix( PL_DEG2RAD( 0.0f ), 1.0f, 0.0f, 0.0f );
	PlRotateMatrix( PL_DEG2RAD( spriteAngle ), 0.0f, 1.0f, 0.0f );
	PlRotateMatrix( PL_DEG2RAD( 180.0f ), 0.0f, 0.0f, 1.0f );

	PlPopMatrix();

	PlgSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT_LIT ] );

#	if 0
	int w = frame->texture->w; //* 1.7;
	int h = frame->texture->h; //* 1.7;
	int x = -frame->leftOffset;
	int y = -frame->topOffset;
#	else /* for the sake of time, let's botch it! */
	float w = frame->texture->w * 1.7f;
	float h = frame->texture->h * 1.7f;
	float x = -( w / 2.0f );
	float y = -h;
#	endif

	plDrawTexturedRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), x, y, w, h, frame->texture );
#endif
}

void ss_arl_draw_sprite_animation( ApeSpriteFrame **animation, unsigned int numFrames, unsigned int curFrame, const PLVector3 *position, float angle )
{
#if 0
    const GfxCamera *camera = Gfx_GetCurrentCamera();
	if( camera == NULL ) {
		return;
	}

	/* here we go, dumb maths written by dumb me... */
	PLVector2 a = PLVector2( position->x, position->z );
	PLVector2 b = PLVector2( camera->cameraPtr->position.x, camera->cameraPtr->position.z );
	PLVector2 normal = plComputeLineNormal( &a, &b );

	float spriteAngle = atan2f( normal.y, normal.x ) * PL_180_DIV_PI;

	/* should really account for the intended angle, but hey ho...
	 * there are some further improvements to make here - again, running out of time! */
	unsigned int frameColumn = 1;
	float frameAngle = spriteAngle < 0 ? spriteAngle + 360 : spriteAngle;
	if( frameAngle > 315.0f ) {
		frameColumn = 2;
	} else if( frameAngle > 270.0f ) {
		frameColumn = 3;
	} else if( frameAngle > 225.0f ) {
		frameColumn = 4;
	} else if( frameAngle > 180.0f ) {
		frameColumn = 5;
	} else if( frameAngle > 135.0f ) {
		frameColumn = 6;
	} else if( frameAngle > 90.0f ) {
		frameColumn = 7;
	} else if( frameAngle > 45.0f ) {
		frameColumn = 8;
	}

	curFrame *= YR_NUM_SPRITE_ANGLES;
	unsigned int actualFrame = curFrame + ( frameColumn - 1 );
	if( actualFrame > numFrames ) {
		PRINT_WARNING( "Out of scope frame, %d/%d!\n", actualFrame, numFrames );
		actualFrame = numFrames;
	}

	Gfx_DrawAnimationFrame( animation[ actualFrame ], position, spriteAngle );
#endif
}
