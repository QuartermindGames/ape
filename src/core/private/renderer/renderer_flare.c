// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "ape_private.h"
#include "renderer.h"
#include "camera/camera.h"
#include "material/material.h"

// Prey '98 inspired flares!
//TODO: maybe this would be better placed under the game code?
// 		some projects might not want flares the way we do them...

typedef struct Flare
{
	QmMathColour4f colour;
	QmMathVector2f screenPos;
	float          size;
	float          intensity;
	float          distance;
	uint8_t        declType;
} Flare;

static constexpr unsigned int MAX_FLARES = 64;
static unsigned int           numFlares;
static Flare                  flares[ MAX_FLARES ];

static bool flareEnabled = true;

static constexpr char FLARE_SCRIPT_PATH[] = "scripts/flares.acm";

static constexpr float MAX_FLARE_DISTANCE = 256.0f;

static constexpr float FLARE_ELEMENT_WIDTH  = 128.0f;
static constexpr float FLARE_ELEMENT_HEIGHT = 128.0f;

typedef struct FlareElement
{
	float w;
	float h;
	float x;
	float y;
	float scale;
	float delta;
	bool  rotate;
} FlareElement;

static constexpr uint8_t MAX_FLARE_ELEMENTS = 8;
static constexpr uint8_t MAX_FLARE_DECLS    = 16;

typedef struct FlareDecl
{
	ApeMaterial *material;

	FlareElement elements[ MAX_FLARE_ELEMENTS ];
	uint8_t      numElements;
} FlareDecl;

static FlareDecl flareDecls[ MAX_FLARE_DECLS ];
static uint8_t   numFlareDecls;

void ape_initialize_flares_( void )
{
	AcmBranch *root = com_acm_load_file( FLARE_SCRIPT_PATH, "flares" );
	if ( root == nullptr )
	{
		return;
	}

	ACM_ITERATE_BRANCH( root, child )
	{
		if ( numFlareDecls >= MAX_FLARE_DECLS )
		{
			ape_console_warning_( "Hit maximum elements limit for flare (%u) (%u >= %u)!\n", numFlareDecls, numFlareDecls, MAX_FLARE_DECLS );
			break;
		}

		const char *materialPath = acm_get_string( child, "material", nullptr );
		if ( materialPath == nullptr )
		{
			ape_console_warning_( "Encountered a flare declaration without a material (%u)!\n", numFlareDecls );
			continue;
		}

		FlareDecl *flareDecl = &flareDecls[ numFlareDecls ];

		flareDecl->material = ape_material_cache( materialPath, APE_CACHE_GROUP_GLOBAL, true );

		AcmBranch *elements = acm_get_child_by_name( child, "elements" );
		ACM_ITERATE_BRANCH( elements, element )
		{
			if ( flareDecl->numElements >= MAX_FLARE_ELEMENTS )
			{
				ape_console_warning_( "Hit maximum elements limit for flare (%u) (%u >= %u)!\n", numFlareDecls, flareDecl->numElements, MAX_FLARE_ELEMENTS );
				break;
			}

			FlareElement *flareElement = &flareDecl->elements[ flareDecl->numElements ];

			flareElement->w = acm_get_f32( element, "w", 0.0f );
			flareElement->h = acm_get_f32( element, "h", 0.0f );
			flareElement->x = acm_get_f32( element, "x", 0.0f );
			flareElement->y = acm_get_f32( element, "y", 0.0f );

			flareElement->scale  = acm_get_f32( element, "scale", 1.0f );
			flareElement->delta  = acm_get_f32( element, "delta", 1.0f );
			flareElement->rotate = acm_get_bool( element, "rotate", false );

			flareDecl->numElements++;
		}

		numFlareDecls++;
	}
}

void ape_register_flare_console_variables_( void )
{
	ape_console_var_register( "renderer.flareEnabled", "Enable/disable rendering of lensflare effects.", "true", PL_VAR_BOOL, &flareEnabled, NULL, APE_CONSOLE_VAR_FLAG_ARCHIVE );
}

void ape_shutdown_flares_( void )
{
	for ( unsigned int i = 0; i < numFlareDecls; ++i )
	{
		ape_material_release_reference( flareDecls[ i ].material );
		flareDecls[ i ].material = nullptr;
	}
	numFlareDecls = 0;

	QM_OS_ZERO_( flares );
}

void ape_add_flare_to_queue( const ApeCamera *camera, const QmMathVector3f *worldPos, const QmMathColour4f *colour, float size, float intensity )
{
	ApeViewport *viewport = ape_viewport_get_active();
	if ( viewport == NULL )
	{
		return;
	}

	if ( numFlares >= MAX_FLARES )
	{
		ape_console_warning_( "Hit flare limit (%u >= %u)!\n", numFlares, MAX_FLARES );
		return;
	}

	float distance = qm_math_vector3f_length( qm_math_vector3f_sub( *worldPos, ape_camera_get_position( camera ) ) );
	if ( distance >= MAX_FLARE_DISTANCE )
	{
		return;
	}

	PLMatrix4      m              = PlMultiplyMatrix4( &camera->proj, &camera->view );
	int            viewportSize[] = { viewport->x, viewport->y, viewport->width, viewport->height };
	float          depth;
	QmMathVector2f screenPos = PlConvertWorldToScreen( worldPos, &m, viewportSize, &depth, true );
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

static void draw_flare( const Flare *flare, const float deltaX, const float deltaY, const float intensity )
{
	const FlareDecl *flareDecl = &flareDecls[ flare->declType ];
	if ( flareDecl->material == nullptr )
	{
		return;
	}

	for ( unsigned int i = 0; i < flareDecl->numElements; ++i )
	{
		const FlareElement *element = &flareDecl->elements[ i ];

		float x = flare->screenPos.x - deltaX * element->delta * ( float ) i;
		float y = flare->screenPos.y - deltaY * element->delta * ( float ) i;
		float s = flare->size * intensity;

		// sprite properties
		QmMathColour4f colour   = QM_MATH_COLOUR4F( flare->colour.r, flare->colour.g, flare->colour.b, intensity );
		QmMathVector3f position = QM_MATH_VECTOR3F( x, y, 0.0f );
		QmMathVector3f origin   = QM_MATH_VECTOR3F( -( element->w / 2.0f ), -( element->h / 2.0f ), 0.0f );
		QmMathVector3f angles   = QM_MATH_VECTOR3F( 0.0f, 0.0f, element->rotate ? ( deltaX + deltaY ) / QM_MATH_PI : 0.0f );

		// area of the texture we want to use
		PLQuad quad = { element->x, element->y, element->w, element->h };

		ape_draw_sprite( flareDecl->material, &quad, &colour, &position, &origin, &angles, element->scale * s );
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

		float deltaX = dx / ( float ) flareDecls[ flare->declType ].numElements * 4.0f;
		float deltaY = dy / ( float ) flareDecls[ flare->declType ].numElements * 4.0f;

		float maxDistance = qm_math_vector2f_length( QM_MATH_VECTOR2F( w, h ) ) / 4.0f;
		float intensity   = QM_MATH_CLAMP( 0.0f, ( 1.0f - ( qm_math_vector2f_length( QM_MATH_VECTOR2F( dx, dy ) ) / maxDistance ) ) - ( flare->distance / ( MAX_FLARE_DISTANCE ) ), 1.0f );
		sumFlareIntensity += intensity - flare->distance / ( MAX_FLARE_DISTANCE / 2.0f );

		draw_flare( &flares[ i ], deltaX, deltaY, intensity );
	}

#if 1// hogsy: todo!
	sumFlareIntensity = QM_MATH_CLAMP( 0.0f, sumFlareIntensity, 1.0f );
	if ( sumFlareIntensity > 0.0f )
	{
		ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

		PlgSetBlendMode( PLG_BLEND_ADDITIVE );
		PlgDrawRectangle( 0.0f, 0.0f, w, h, qm_math_colour4ub( 255, 255, 255, QM_MATH_FTOB( sumFlareIntensity ) ) );
		PlgSetBlendMode( PLG_BLEND_DISABLE );
	}
#endif

	PlPopMatrix();

	COM_PROFILE_FUNCTION_END();
}
