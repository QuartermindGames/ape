// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: For handling general UI routines.
// Author:  Mark E. Sowden

#include "tox_ui.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

ApeMaterial *edHealthIcon = NULL;

typedef enum ToxUIElementType
{
	TOX_UI_ELEMENT_TYPE_HEALTH,
	TOX_UI_ELEMENT_TYPE_STAMINA,
	TOX_UI_ELEMENT_TYPE_ITEM,
	TOX_UI_ELEMENT_TYPE_ITEM_COUNT,

	TOX_UI_MAX_ELEMENT_TYPES
} ToxUIType;

static void draw_dial( int16_t value, float radius, float thickness, float centerX, float centerY, float precision, const PLColour *colour )
{
	PlgSetShaderProgram( ss_arl_shader_get_default( APE_SHADER_DEFAULT_VERTEX ) );
	PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );

	static const float RANDOM_VARIATION = 10.0f;

	srand( ( int ) precision );

	float endAngle = ( ( ( float ) value ) / 100.0f * 2.0f * PL_PI );
	for ( float angle = 0.0f; angle <= endAngle; angle += precision )
	{
		float x, y;

		// outer
		x = centerX + ( radius + PlGenerateRandomFloat( RANDOM_VARIATION ) ) * cosf( angle );
		y = centerY + ( radius + PlGenerateRandomFloat( RANDOM_VARIATION ) ) * sinf( angle );
		PlgImmPushVertex( x, y, 0.0f );
		PlgImmColour( colour->r, colour->g, colour->b, colour->a );

		// inner
		x = centerX + ( ( radius - thickness ) + PlGenerateRandomFloat( RANDOM_VARIATION ) ) * cosf( angle );
		y = centerY + ( ( radius - thickness ) + PlGenerateRandomFloat( RANDOM_VARIATION ) ) * sinf( angle );
		PlgImmPushVertex( x, y, 0.0f );
		PlgImmColour( colour->r / 2, colour->g / 2, colour->b / 2, colour->a );
	}

	PlgImmDraw();
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void tox_ui_initialize( void )
{
	edHealthIcon = ss_arl_material_cache( "materials/hud/hud_ed_head.mat.n", APE_CACHE_WORLD, true, false );
}

void tox_ui_tick( void ) {}

bool tox_ui_draw( ApeViewport *viewport )
{
	static const float HEALTH_RADIUS = 70.0f;
	static const float HEALTH_THICKNESS = 30.0f;

	PlPushMatrix();
	PlLoadIdentityMatrix();
	PlTranslateMatrix( PLVector3( HEALTH_RADIUS + 20.0f, viewport->height - ( HEALTH_RADIUS + 20.0f ), 0.0f ) );
	PlRotateMatrix( sinf( ape_get_num_ticks() / 20.0f ) / 40.0f, 0.0f, 0.0f, 1.0f );


	draw_dial( 100, HEALTH_RADIUS, HEALTH_THICKNESS, 10.0f, 10.0f, 1.0f, &PL_COLOURU8( 0, 0, 0, 255 ) );    // health
	draw_dial( 100, HEALTH_RADIUS / 2, HEALTH_THICKNESS, 10.0f, 10.0f, 1.0f, &PL_COLOURU8( 0, 0, 0, 255 ) );// stamina

	draw_dial( 100, HEALTH_RADIUS, HEALTH_THICKNESS, 0.0f, 0.0f, 1.0f, &PL_COLOURU8( 255, 0, 0, 255 ) );    // health
	draw_dial( 100, HEALTH_RADIUS / 2, HEALTH_THICKNESS, 0.0f, 0.0f, 1.0f, &PL_COLOURU8( 0, 255, 0, 255 ) );// stamina

	PlPopMatrix();

	return false;
}
