// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "renderer.h"

// Prey '98 inspired flares!
//TODO: maybe this would be better placed under the game code?
// 		some projects might not want flares the way we do them...

typedef struct Flare
{
	PLColourF32 colour;
	PLVector2   screenPos;
	float       size;
	float       intensity;
	float       distance;
} Flare;

static constexpr unsigned int MAX_FLARES = 64;
static unsigned int           numFlares;
static Flare                  flares[ MAX_FLARES ];

static const char  *flareMaterialPath = "materials/effects/effect_flare_sheet.mat.n";
static ApeMaterial *flareMaterial;

static bool flareEnabled = true;

static constexpr float MAX_FLARE_DISTANCE = 256.0f;

static constexpr float FLARE_ELEMENT_WIDTH  = 128.0f;
static constexpr float FLARE_ELEMENT_HEIGHT = 128.0f;

typedef struct FlareElement
{
	float w;
	float h;
	float x;
	float y;
	bool  rotate;
} FlareElement;

static constexpr unsigned int NUM_FLARE_ELEMENTS                  = 4;
static const FlareElement     flareElements[ NUM_FLARE_ELEMENTS ] = {
        { .w = FLARE_ELEMENT_WIDTH, .h = FLARE_ELEMENT_HEIGHT, .x = 0.0f, .y = 0.0f, .rotate = true },
        { .w = FLARE_ELEMENT_WIDTH, .h = FLARE_ELEMENT_HEIGHT, .x = FLARE_ELEMENT_WIDTH, .y = 0.0f },
        { .w = FLARE_ELEMENT_WIDTH, .h = FLARE_ELEMENT_HEIGHT, .x = FLARE_ELEMENT_WIDTH * 2, .y = 0.0f },
        { .w = FLARE_ELEMENT_WIDTH, .h = FLARE_ELEMENT_HEIGHT, .x = FLARE_ELEMENT_WIDTH * 3, .y = 0.0f },
};

void ape_initialize_flares_( void )
{
	flareMaterial = ape_material_cache( flareMaterialPath, APE_CACHE_GROUP_GLOBAL, true );
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

	float distance = PlVector3Length( PlSubtractVector3( *worldPos, ape_camera_get_position( camera ) ) );
	if ( distance >= MAX_FLARE_DISTANCE )
	{
		return;
	}

	PLMatrix4 m              = PlMultiplyMatrix4( &camera->internal->internal.proj, &camera->internal->internal.view );
	int       viewportSize[] = { viewport->x, viewport->y, viewport->width, viewport->height };
	float     depth;
	PLVector2 screenPos = PlConvertWorldToScreen( worldPos, &m, viewportSize, &depth, true );
	if ( depth <= 0.0f )
	{
		return;
	}

	Flare *flare     = &flares[ numFlares++ ];
	flare->colour    = *colour;
	flare->size      = size;
	flare->intensity = intensity;
	flare->screenPos = screenPos;
	flare->distance  = distance;
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
		float s = 2.0f;

		// sprite properties
		const FlareElement *element  = &flareElements[ i ];
		PLColourF32         colour   = { 1.0f, 1.0f, 1.0f, intensity };
		PLVector3           position = { x, y, 0.0f };
		PLVector3           origin   = { -( element->w / 2.0f ), -( element->h / 2.0f ), 0.0f };
		PLVector3           angles   = { 0.0f, 0.0f, element->rotate ? ( deltaX + deltaY ) / PL_PI : 0.0f };

		// area of the texture we want to use
		PLQuad quad = { element->x, element->y, element->w, element->h };

		ape_draw_sprite( flareMaterial, &quad, &colour, &position, &origin, &angles, /*1.0f - ( intensity / ( ( i + 1 ) * 100.0f ) ) / 1.0f*/ s );
	}
}

void ape_flare_draw_( const ApeViewport *viewport )
{
	if ( !flareEnabled )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

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

		float deltaX = ( dx / ( float ) NUM_FLARE_ELEMENTS ) * 2.0f;
		float deltaY = ( dy / ( float ) NUM_FLARE_ELEMENTS ) * 2.0f;

		float maxDistance = PlGetVector2Length( &PL_VECTOR2( w, h ) ) / 4.0f;
		float intensity   = PlClamp( 0.0f, ( 1.0f - ( PlGetVector2Length( &PL_VECTOR2( dx, dy ) ) / maxDistance ) ) - ( flare->distance / ( MAX_FLARE_DISTANCE ) ), 1.0f );
		sumFlareIntensity += ( intensity - ( flare->distance / ( MAX_FLARE_DISTANCE / 2.0f ) ) );

		draw_flare( &flares[ i ], deltaX, deltaY, intensity );
	}

#if 1// hogsy: todo!
	sumFlareIntensity = PlClamp( 0.0f, sumFlareIntensity, 1.0f );
	if ( sumFlareIntensity > 0.0f )
	{
		ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

		PlgSetBlendMode( PLG_BLEND_ADDITIVE );
		PlgDrawRectangle( 0.0f, 0.0f, w, h, PLColour( 255, 255, 255, PlFloatToByte( sumFlareIntensity ) ) );
		PlgSetBlendMode( PLG_BLEND_DISABLE );
	}
#endif

	PlPopMatrix();

	COM_PROFILE_FUNCTION_END();
}
