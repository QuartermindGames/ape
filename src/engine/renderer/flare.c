/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "flare.h"
#include "renderer.h"

/* Prey '98 inspired flares! */

static const char *flarePaths[] = {
        "materials/engine/effects/Flare1.gif",
        "materials/engine/effects/Flare2.gif",
        "materials/engine/effects/Flare3.gif",
        "materials/engine/effects/Flare4.gif",
        "materials/engine/effects/Flare5.gif",
        "materials/engine/effects/Flare6.gif",
};
#define MAX_FLARE_TEXTURES plArrayElements( flarePaths )
static PLGTexture *flareTextures[ MAX_FLARE_TEXTURES ];

static const char *shinePaths[] = {
        "materials/engine/effects/Shine0.gif",
        "materials/engine/effects/Shine1.gif",
        "materials/engine/effects/Shine2.gif",
        "materials/engine/effects/Shine3.gif",
        "materials/engine/effects/Shine4.gif",
        "materials/engine/effects/Shine5.gif",
        "materials/engine/effects/Shine6.gif",
        "materials/engine/effects/Shine7.gif",
        "materials/engine/effects/Shine8.gif",
        "materials/engine/effects/Shine9.gif",
};
#define MAX_SHINE_TEXTURES plArrayElements( shinePaths )
static PLGTexture *shineTextures[ MAX_SHINE_TEXTURES ];

void Flare_Initialize( void ) {
	for ( unsigned int i = 0; i < MAX_FLARE_TEXTURES; ++i ) {
		flareTextures[ i ] = Gfx_LoadTexture( flarePaths[ i ] );
	}
	for ( unsigned int i = 0; i < MAX_SHINE_TEXTURES; ++i ) {
		shineTextures[ i ] = Gfx_LoadTexture( shinePaths[ i ] );
	}
}

void Flare_Render( PLGTexture *texture, float diameter, float distance ) {
	PLGShaderProgram *program = PlgGetCurrentShaderProgram();
	if ( program == NULL ) {
		return;
	}

	PlgSetShaderUniformValue( program, "scale", &diameter, false );


}

void Flare_RenderFlares( const GfxCamera *camera ) {
	RS_GetShaderProgram( "flare" );

	PlgSetBlendMode( PLG_BLEND_ADDITIVE );



	PlgSetBlendMode( PLG_BLEND_DISABLE );
}
