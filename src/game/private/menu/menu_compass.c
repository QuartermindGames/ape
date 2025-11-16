// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: A compass. What more can I say?
// Author:  Mark E. Sowden

#include "menu_compass.h"

static QmMathVector3f compassAngles;

static ApeGuiFont  *compassFont;
static ApeMaterial *compassBackground;

void game_menu_compass_initialize_( ApeGuiFont *font )
{
	compassAngles = QM_MATH_VECTOR3F_ZERO;
	compassFont   = font;

	compassBackground = ape_material_cache( "materials/ui/ui_circle.mat.n", APE_CACHE_GROUP_GLOBAL, true );
}

void game_menu_compass_shutdown_()
{
	ape_material_release( compassBackground );
}

void game_menu_compass_draw_( const ApeViewport *viewport )
{
	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	float scale = 1.0f;

	const float w = 256.0f * scale;

	static constexpr float MAX_HEIGHT = 200.0f;
	static constexpr float MIN_HEIGHT = 30.0f;

	const float h = QM_MATH_CLAMP( MIN_HEIGHT, MAX_HEIGHT * ( -compassAngles.x / MAX_HEIGHT ) + MAX_HEIGHT / 2.0f * scale, MAX_HEIGHT );

	const float x = w / 2.0f + 32.0f;
	const float y = viewport->height - w / 2.0f + 32.0f;

	ape_draw_textured_quad( compassBackground, x - w / 2.0f, y - h / 2.0f, w * scale, h * scale, &QM_MATH_COLOUR4UB( 0, 0, 0, 128 ), 0.0f );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_LINES );

	for ( unsigned int i = 0; i < 360; i += 360 / ( 360 / 5 ) )
	{
		float xo = x + cosf( PL_DEG2RAD( ( float ) i + compassAngles.y ) ) * ( w / 2.0f * scale );
		float yo = y + sinf( PL_DEG2RAD( ( float ) i + compassAngles.y ) ) * ( h / 2.0f * scale );

		float vpos = ( yo - y ) / ( h / 2.0f );
		if ( vpos >= 0.0f )
		{
			continue;
		}

		static constexpr float MAX_PIN_HEIGHT = 16.0f;
		static constexpr float MIN_PIN_HEIGHT = 2.0f;

		float height = i % 3 ? MIN_PIN_HEIGHT : MAX_PIN_HEIGHT;
		height       = height - height * h / MAX_HEIGHT / 2.0f;

		PlgImmPushVertex( xo, yo, 0.0f );
		PlgImmColour( 255, 255, 255, 255 );
		PlgImmPushVertex( xo, yo - height * ( -vpos * scale ), 0.0f );
		PlgImmColour( 255, 255, 255, 255 );
	}

	static constexpr char CHARS[ 4 ] = { 'N', 'E', 'S', 'W' };

	const char *c = CHARS;
	for ( unsigned int i = 0; i < 360; i += 360 / 4, c++ )
	{
		float xo = x + cosf( PL_DEG2RAD( ( float ) i + compassAngles.y ) ) * ( w / 2.0f * scale );
		float yo = y + sinf( PL_DEG2RAD( ( float ) i + compassAngles.y ) ) * ( h / 2.0f * scale );

		float vpos = ( yo - y ) / ( h / 2.0f );
		if ( vpos >= 0.0f )
		{
			continue;
		}

		QmMathColour4ub colour = qm_math_colour4ub( 255, 255, 255, 255 );

		float fontScale = -vpos * scale / 4.0f;

		float cw, ch;
		ape_gui_font_get_character_pixel_size( compassFont, fontScale, *c, &cw, &ch );

		gui_font_draw_character( compassFont, xo - cw / 2.0f, yo - ch * 1.5f, fontScale, &colour, *c );

		PlgImmPushVertex( xo, yo, 0.0f );
		PlgImmColour( 255, 0, 0, 255 );
		PlgImmPushVertex( xo, yo - 16.0f * scale, 0.0f );
		PlgImmColour( 255, 0, 0, 255 );
	}

	ape_material_draw( material, mesh, nullptr );

	gui_font_display( compassFont );
}

void game_menu_compass_tick_( const ApeCamera *camera, const double delta )
{
	QmMathVector3f angles = ape_camera_get_angles( camera );
	com_math_interpolate_angles( &compassAngles, &angles, 10.0f * delta, &compassAngles );
}
