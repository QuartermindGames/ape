// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: World simulation state.

#include "tox_game.h"
#include "tox_world.h"

static ToxWorldState worldState;
static unsigned int secondCountdown = 0;
static const unsigned int TICKS_UNTIL_SECOND = 30;

static ApeLight *sunLight = NULL;
static float sunYaw = 0.0f;
static float sunPitch = 0.0f;
static float sunBrightness = 0.0f;


static ApeLight *moonLight = NULL;
static float moonBrightness = 0.0f;

/**
 * This is a very convoluted way to set the pitch and yaw, but
 * unfortunately *this* idiot decided to make the sun a position
 */
static QmMathVector3f pitch_yaw_to_position( float pitch, float yaw )
{
	QmMathVector3f position = { 1.0f, pitch, 0.0f };
	PLMatrix4 matrix = PlMatrix4Identity();
	matrix = PlMultiplyMatrix4( PlTranslateMatrix4( position ), &matrix );
	matrix = PlMultiplyMatrix4( PlRotateMatrix4( PL_DEG2RAD( yaw ), &( QmMathVector3f ){ 0.0f, 1.0f, 0.0f } ), &matrix );
	position.x = matrix.m[ 0 ];
	position.z = matrix.m[ 8 ];
	return position;
}

	sunYaw = tox_world_get_seconds_in_day( &worldState ) / ( TOX_WORLD_SECONDS_TO_DAY / 360.0f );
	sunPitch = sinf( PL_DEG2RAD( sunYaw + 90.0f ) ) * 2.0f;
	sunBrightness = PlClamp( 0.0f, ( -sunPitch ) / 1.0f, 1.25f );

	if ( sunLight != nullptr )
	{
		QmMathVector3f sunPosition = pitch_yaw_to_position( sunPitch, sunYaw );
		ape_light_set_position( sunLight, &sunPosition );
		ape_light_set_colour( sunLight, &QM_MATH_COLOUR4F( DEFAULT_SUN_COLOUR.r,
		                                               DEFAULT_SUN_COLOUR.g,
		                                               DEFAULT_SUN_COLOUR.b,
		                                               sunBrightness ) );
	}

	QmMathVector3f moonPosition = pitch_yaw_to_position( -sunPitch, -sunYaw );
	moonBrightness = PlClamp( 0.0f, ( sunPitch ) / 1.0f, 0.25f );

	if ( moonLight != nullptr )
	{
		ape_light_set_position( moonLight, &moonPosition );
		ape_light_set_colour( moonLight, &QM_MATH_COLOUR4F( DEFAULT_MOON_COLOUR.r,
		                                                DEFAULT_MOON_COLOUR.g,
		                                                DEFAULT_MOON_COLOUR.b,
		                                                moonBrightness ) );
	}

	for ( unsigned int i = 0; i < TOX_MAX_SKY_LAYER_TYPES; ++i )
	{
		//TODO: in the future, this should probably be based on wind dir or something...
		float parallax = tox_world_get_seconds_in_day( &worldState ) / ( TOX_WORLD_SECONDS_TO_DAY / skyLayers[ i ].parallaxDiff );
		ape_sky_set_layer_offset( skyLayers[ i ].id, parallax, parallax );
	}

	ape_world_set_ambience( world, &QM_MATH_COLOUR4F( PlClamp( 0.05f, DEFAULT_SUN_COLOUR.r * ( sunBrightness / 0.5f ), 0.45f ),
	                                              PlClamp( 0.05f, DEFAULT_SUN_COLOUR.g * ( sunBrightness / 0.5f ), 0.45f ),
	                                              PlClamp( 0.05f, DEFAULT_SUN_COLOUR.b * ( sunBrightness / 0.5f ), 0.45f ),
	                                              1.0f ) );

	// Fog and clear should remain the same as each other, for a good little fade-out
	QmMathColour4f fallbackColour = QM_MATH_COLOUR4F( PlClamp( 0.0f, DEFAULT_CLEAR_COLOUR.r * ( sunBrightness / 0.5f ), 1.0f ),
	                                           PlClamp( 0.0f, DEFAULT_CLEAR_COLOUR.g * ( sunBrightness / 0.5f ), 1.0f ),
	                                           PlClamp( 0.0f, DEFAULT_CLEAR_COLOUR.b * ( sunBrightness / 0.5f ), 1.0f ),
	                                           1.0f );
	ape_world_set_clear_colour( world, &fallbackColour );
	ape_world_set_fog_colour( world, &fallbackColour );
}

float tox_world_get_sun_brightness( void ) { return sunBrightness; }
float tox_world_get_moon_brightness( void ) { return moonBrightness; }
