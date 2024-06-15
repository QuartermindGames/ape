// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: For handling general UI routines.
// Author:  Mark E. Sowden

#include "tox_ui.h"

#include "../tox_world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

ApeMaterial *edHealthIcon = NULL;

static float updateAggro = 0.0f;
static int16_t health = 100;

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
	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );

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

	ape_material_draw( ss_arl_get_default_material( SS_ARL_MATERIAL_DEFAULT_VERTEX ), mesh, NULL, 0 );
}

static void draw_debug_overlay( void )
{
#if !defined( NDEBUG )

	const ToxWorldState *worldState = tox_world_get_state();
	assert( worldState != NULL );

	GuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_TINY );
	assert( font != NULL );

	float ny = 10.0f;
	float nx = 10.0f;

	char tmp[ 256 ];
	snprintf( tmp, sizeof( tmp ), "%s\n", tox_world_get_time_of_day_descriptor( tox_world_get_time_of_day( worldState ) ) );
	gui_font_draw_string( font, nx, ny, &nx, &ny, 1.0f, &PL_COLOUR_PURPLE, tmp, strlen( tmp ), false );
	snprintf( tmp, sizeof( tmp ), "s: %u ", tox_world_get_current_second( worldState ) );
	gui_font_draw_string( font, nx, ny, &nx, &ny, 1.0f, &PL_COLOUR_PURPLE, tmp, strlen( tmp ), false );
	snprintf( tmp, sizeof( tmp ), "m: %u ", tox_world_get_current_minute( worldState ) );
	gui_font_draw_string( font, nx, ny, &nx, &ny, 1.0f, &PL_COLOUR_PURPLE, tmp, strlen( tmp ), false );
	snprintf( tmp, sizeof( tmp ), "h: %u ", tox_world_get_current_hour( worldState ) );
	gui_font_draw_string( font, nx, ny, &nx, &ny, 1.0f, &PL_COLOUR_PURPLE, tmp, strlen( tmp ), false );

	gui_font_display( font );

#endif
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void tox_ui_initialize( void )
{
	edHealthIcon = ape_material_cache( "materials/hud/hud_ed_head.mat.n", APE_CACHE_GROUP_WORLD, true, false );
}

void tox_ui_shutdown( void )
{
	ape_material_release( edHealthIcon );
	edHealthIcon = NULL;
}

void tox_ui_tick( void )
{
	if ( updateAggro > 0.f )
	{
		updateAggro -= 0.5f;
	}
}

void tox_ui_handle_damage_event( int16_t damage )
{
	updateAggro += ( float ) ( damage * 10 );
	if ( updateAggro > 100.0f )
	{
		updateAggro = 100.0f;
	}

	health -= damage;
}

bool tox_ui_draw( ApeViewport *viewport )
{
	draw_debug_overlay();

	static const float HEALTH_RADIUS = 70.0f;
	static const float HEALTH_THICKNESS = 30.0f;

	PlPushMatrix();
	PlLoadIdentityMatrix();
	PlTranslateMatrix( PLVector3( HEALTH_RADIUS + 20.0f, viewport->height - ( HEALTH_RADIUS + 20.0f ), 0.0f ) );
	PlRotateMatrix( sinf( ape_get_num_ticks() / 20.0f ) / 40.0f * ( updateAggro + 1.0f ), 0.0f, 0.0f, 1.0f );

	draw_dial( health, HEALTH_RADIUS, HEALTH_THICKNESS, 10.0f, 10.0f, 1.0f, &PL_COLOURU8( 0, 0, 0, 255 ) ); // health
	draw_dial( 100, HEALTH_RADIUS / 2, HEALTH_THICKNESS, 10.0f, 10.0f, 1.0f, &PL_COLOURU8( 0, 0, 0, 255 ) );// stamina

	draw_dial( health, HEALTH_RADIUS, HEALTH_THICKNESS, 0.0f, 0.0f, 1.0f, &PL_COLOURU8( 255, 0, 0, 255 ) ); // health
	draw_dial( 100, HEALTH_RADIUS / 2, HEALTH_THICKNESS, 0.0f, 0.0f, 1.0f, &PL_COLOURU8( 0, 255, 0, 255 ) );// stamina

	PlPopMatrix();

	return false;
}
