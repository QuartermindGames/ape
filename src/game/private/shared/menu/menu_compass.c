// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: A compass. What more can I say?
// Author:  Mark E. Sowden

#include "menu_compass.h"

static QmMathVector3f compassAngles;

static ApeGuiFont *compassFont;

void game_menu_compass_initialize_( ApeGuiFont *font )
{
	compassAngles = QM_MATH_VECTOR3F_ZERO;
	compassFont   = font;
}

void game_menu_compass_shutdown_()
{
}

void game_menu_compass_draw_( const ApeViewport *viewport )
{
	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	float x     = viewport->width / 2.0f;
	float y     = 128.0f;
	float w     = viewport->width / 4.0f;
	float h     = 128.0f;
	float scale = 1.0f;

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_LINES );

	for ( unsigned int i = 0; i < 360; i += 360 / ( 360 / 5 ) )
	{
		float xo = x + cosf( PL_DEG2RAD( ( float ) i + compassAngles.y ) ) * ( w / 2.0f * scale );
		float yo = y + sinf( PL_DEG2RAD( ( float ) i + compassAngles.y ) ) * ( h / 2.0f * scale );

		float height = i % 3 ? 2.0f : 16.0f;

		//float height = 8.0f;

		PlgImmPushVertex( xo, yo, 0.0f );
		PlgImmColour( 255, 255, 255, 255 );
		PlgImmPushVertex( xo, yo - height * scale, 0.0f );
		PlgImmColour( 255, 255, 255, 255 );
	}

	static constexpr char CHARS[ 4 ] = { 'N', 'E', 'S', 'W' };

	const char *c = CHARS;
	for ( unsigned int i = 0; i < 360; i += 360 / 4 )
	{
		float xo = x + cosf( PL_DEG2RAD( ( float ) i + compassAngles.y ) ) * ( w / 2.0f * scale );
		float yo = y + sinf( PL_DEG2RAD( ( float ) i + compassAngles.y ) ) * ( h / 2.0f * scale );

		float xc = fabsf( x - xo ) / w - w * 2.0f;
		float yc = fabsf( y - yo ) / h - h * 2.0f;
		float dc = scale * ( qm_math_vector2f_length( QM_MATH_VECTOR2F( 1.0f, 1.0f ) ) - qm_math_vector2f_length( QM_MATH_VECTOR2F( xc, yc ) ) );

		QmMathColour4ub colour = qm_math_colour4ub( 255, 255, 255, 255 );

		float cw, ch;
		ape_gui_font_get_character_pixel_size( compassFont, scale / 2.0f, *c, &cw, &ch );

		gui_font_draw_character( compassFont, xo - cw / 2.0f, yo - ch, scale / 2.0f, &colour, *c++ );

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
