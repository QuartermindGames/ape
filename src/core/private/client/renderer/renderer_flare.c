// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "renderer.h"

// Prey '98 inspired flares!

typedef struct Flare
{
	PLColourF32 colour;
	PLVector2 screenPos;
	float size;
	float intensity;
	float distance;
} Flare;

#define MAX_FLARES 64
static unsigned int numFlares;
static Flare flares[ MAX_FLARES ];

static const char *flareMaterialPath = "materials/effects/effect_flare_sheet.mat.n";
static ApeMaterial *flareMaterial;

static bool flareEnabled = true;

static const float MAX_FLARE_DISTANCE = 256.0f;

static const float FLARE_ELEMENT_WIDTH = 128.0f;
static const float FLARE_ELEMENT_HEIGHT = 128.0f;
#define NUM_FLARE_ELEMENTS 4
static const PLQuad flareElements[ NUM_FLARE_ELEMENTS ] = {
        {.w = FLARE_ELEMENT_WIDTH,  .h = FLARE_ELEMENT_HEIGHT, .x = 0.0f,                    .y = 0.0f},
        { .w = FLARE_ELEMENT_WIDTH, .h = FLARE_ELEMENT_HEIGHT, .x = FLARE_ELEMENT_WIDTH,     .y = 0.0f},
        { .w = FLARE_ELEMENT_WIDTH, .h = FLARE_ELEMENT_HEIGHT, .x = FLARE_ELEMENT_WIDTH * 2, .y = 0.0f},
        { .w = FLARE_ELEMENT_WIDTH, .h = FLARE_ELEMENT_HEIGHT, .x = FLARE_ELEMENT_WIDTH * 3, .y = 0.0f},
};

void ape_initialize_flares_( void )
{
	flareMaterial = ape_material_cache( flareMaterialPath, APE_CACHE_GROUP_GLOBAL, true, false );
}

void ape_register_flare_console_variables_( void )
{
	PlRegisterConsoleVariable( "renderer.flareEnabled", "Enable/disable rendering of lensflare effects.", "true", PL_VAR_BOOL, &flareEnabled, NULL, true );
}

void ape_shutdown_flares_( void )
{
	ape_material_release( flareMaterial );
	flareMaterial = NULL;

	PL_ZERO_( flares );
}

void ape_add_flare_to_queue( const ApeCamera *camera, const PLVector3 *worldPos, const PLColourF32 *colour, float size, float intensity )
{
	ApeViewport *viewport = ape_viewport_get_active();
	if ( viewport == NULL )
	{
		return;
	}

	if ( numFlares >= MAX_FLARES )
	{
		ape_warning_( "Hit flare limit (%u >= %u)!\n", numFlares, MAX_FLARES );
		return;
	}

	float distance = PlVector3Length( PlSubtractVector3( *worldPos, camera->internal->position ) );
	if ( distance >= MAX_FLARE_DISTANCE )
	{
		return;
	}

	float w = ( float ) viewport->width;
	float h = ( float ) viewport->height;

	PLMatrix4 m = PlMultiplyMatrix4( camera->internal->internal.proj, &camera->internal->internal.view );
	PLVector2 screenPos = PlConvertWorldToScreen( worldPos, &m, viewport->width, viewport->height, viewport->x, viewport->y, true );
	if ( screenPos.x > w || screenPos.y > h || screenPos.x < 0.f || screenPos.y < 0.f )
	{
		return;
	}

	Flare *flare = &flares[ numFlares++ ];
	flare->colour = *colour;
	flare->size = size;
	flare->intensity = intensity;
	flare->screenPos = screenPos;
	flare->distance = distance;
}

void ape_clear_flare_queue_( void )
{
	numFlares = 0;
}

static void draw_flare( const Flare *flare, float deltaX, float deltaY, float intensity )
{
	for ( unsigned int i = 0; i < NUM_FLARE_ELEMENTS; ++i )
	{
		float x = flare->screenPos.x - ( deltaX * ( float ) i );
		float y = flare->screenPos.y - ( deltaY * ( float ) i );

		// TODO: these are flipped relative to screen-space, urgh...
#if 0
		ape_draw_textured_quad( ss_arl_get_default_material( SS_ARL_MATERIAL_DEFAULT_VERTEX ), x - FLARE_ELEMENT_WIDTH / 2.0f, y - FLARE_ELEMENT_HEIGHT / 2.0f, FLARE_ELEMENT_WIDTH, FLARE_ELEMENT_HEIGHT, &PL_COLOURU8( 255, 255, 255, 255 ) );
#else
		ape_draw_sprite( flareMaterial, &flareElements[ i ],
		                 &PL_COLOURF32( 1.0f, 1.0f, 1.0f, intensity ),
		                 &PL_VECTOR3( x - FLARE_ELEMENT_WIDTH / 2.0f, y - FLARE_ELEMENT_HEIGHT / 2.0f, 0.0f ),
		                 &pl_vecOrigin3,
		                 &pl_vecOrigin3, /*1.0f - ( intensity / ( ( i + 1 ) * 100.0f ) ) / 1.0f*/ 1.0f );
#endif
	}
}

void ape_flare_draw_( const ApeViewport *viewport )
{
	if ( !flareEnabled )
	{
		return;
	}

	PlPushMatrix();
	PlLoadIdentityMatrix();

	float w = ( float ) viewport->width;
	float h = ( float ) viewport->height;

	float sumFlareIntensity = 0.0f;

	for ( unsigned int i = 0; i < numFlares; ++i )
	{
		const Flare *flare = &flares[ i ];

		float cx = w / 2.0f;
		float cy = h / 2.0f;

		float dx = flare->screenPos.x - cx;
		float dy = flare->screenPos.y - cy;

		float deltaX = dx / ( float ) NUM_FLARE_ELEMENTS;
		float deltaY = dy / ( float ) NUM_FLARE_ELEMENTS;

		float maxDistance = PlGetVector2Length( &PL_VECTOR2( w, h ) ) / 4.0f;
		float intensity = PlClamp( 0.0f, ( 1.0f - ( PlGetVector2Length( &PL_VECTOR2( dx, dy ) ) / maxDistance ) ), 1.0f );
		sumFlareIntensity += intensity * ( 1.0f - ( flare->distance / ( MAX_FLARE_DISTANCE / 10.0f ) ) );

		draw_flare( &flares[ i ], deltaX, deltaY, intensity );
	}

#if 0// hogsy: todo!
	ape_print_( "sum flare = %f\n", sumFlareIntensity );
	if ( sumFlareIntensity > 0.0f )
	{
		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
		PlgSetBlendMode( PLG_BLEND_ADDITIVE );
		PlgDrawRectangle( 0.0f, 0.0f, w, h, PLColour( 255, 255, 255, PlFloatToByte( sumFlareIntensity ) ) );
		PlgSetBlendMode( PLG_BLEND_DISABLE );
	}
#endif

	PlPopMatrix();
}
