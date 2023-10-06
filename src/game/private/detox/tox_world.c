// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: World simulation state.

#include "tox_game.h"
#include "tox_world.h"

static ToxWorldState worldState;
static unsigned int secondCountdown = 0;
static const unsigned int TICKS_UNTIL_SECOND = 30;

static ApeWorld *currentWorld = NULL;

#define DEFAULT_SUN_POSITION PLVector3( -2.0f, -2.0f, 0.0f )
#define DEFAULT_SUN_COLOUR   PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.85f )
#define DEFAULT_CLEAR_COLOUR PL_COLOURF32( 0.1f, 0.5f, 1.0f, 1.0f )

static ApeLight *sunLight = NULL;
static float sunYaw = 0.0f;
static float sunPitch = 0.0f;
static float sunBrightness = 0.0f;

#define DEFAULT_MOON_COLOUR PL_COLOURF32( 0.2f, 0.2f, 0.5f, 0.0f )

static ApeLight *moonLight = NULL;
static float moonBrightness = 0.0f;

#define TEST_NIGHT_LIGHTS

#ifdef TEST_NIGHT_LIGHTS
#	define NUM_TEST_NIGHT_LIGHTS 16
static ApeLight *testLights[ NUM_TEST_NIGHT_LIGHTS ];
#endif

const ToxWorldState *tox_world_get_state( void ) { return &worldState; }

void tox_world_spawn( ApeWorld *world )
{
	currentWorld = world;

	PL_ZERO_( worldState );

	acl_world_set_clear_colour( world, &DEFAULT_CLEAR_COLOUR );

	sunLight = ape_light_create( &DEFAULT_SUN_POSITION, &DEFAULT_SUN_COLOUR, 0.0f,
	                             APE_LIGHT_TYPE_SUN,
	                             APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC | APE_LIGHT_FLAG_RUNTIME_SHADOWS );
	ape_world_attach_light( world, sunLight );

	moonLight = ape_light_create( &DEFAULT_SUN_POSITION, &DEFAULT_MOON_COLOUR, 0.0f,
	                              APE_LIGHT_TYPE_SUN,
	                              APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC | APE_LIGHT_FLAG_RUNTIME_SHADOWS );
	ape_world_attach_light( world, moonLight );

#ifdef TEST_NIGHT_LIGHTS
#	define TORCH_COLOUR ( PLColourF32 ){ 0.85f, 0.83f, 0.45f, 0.0f }
	ape_world_attach_light( world,
	                        ( testLights[ 0 ] = ape_light_create(
	                                  &( PLVector3 ){ -12.f, 1.5f, -13.2f },
	                                  &TORCH_COLOUR,
	                                  1.5f,
	                                  APE_LIGHT_TYPE_OMNI,
	                                  APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC ) ) );
	ape_world_attach_light( world,
	                        ( testLights[ 1 ] = ape_light_create(
	                                  &( PLVector3 ){ -4.8f, 1.5f, -4.2f },
	                                  &TORCH_COLOUR,
	                                  1.5f,
	                                  APE_LIGHT_TYPE_OMNI,
	                                  APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC ) ) );
#endif
}

/**
 * This is a very convoluted way to set the pitch and yaw, but
 * unfortunately *this* idiot decided to make the sun a position
 */
static PLVector3 pitch_yaw_to_position( float pitch, float yaw )
{
	PLVector3 position = { 1.0f, pitch, 0.0f };
	PLMatrix4 matrix = PlMatrix4Identity();
	matrix = PlMultiplyMatrix4( PlTranslateMatrix4( position ), &matrix );
	matrix = PlMultiplyMatrix4( PlRotateMatrix4( PL_DEG2RAD( yaw ), &( PLVector3 ){ 0.0f, 1.0f, 0.0f } ), &matrix );
	position.x = matrix.m[ 0 ];
	position.z = matrix.m[ 8 ];
	return position;
}

void tox_world_tick( void )
{
	ApeWorld *world = acl_world_get_current();
	if ( world == NULL )
		return;

#if 0
	// Don't increment a second for every tick,
	// otherwise in-game time will go superfast
	if ( secondCountdown == 0 )
	{
		worldState.seconds += TOX_WORLD_SECONDS_TO_HOUR / TOX_WORLD_MINUTES_TO_HOUR;
		secondCountdown = TICKS_UNTIL_SECOND;
	}
	else
		secondCountdown--;
#else
	worldState.seconds += TOX_WORLD_SECONDS_TO_HOUR / 200;
#endif

	sunYaw = tox_world_get_seconds_in_day( &worldState ) / ( TOX_WORLD_SECONDS_TO_DAY / 360.0f );
	sunPitch = sinf( PL_DEG2RAD( sunYaw + 90.0f ) ) * 2.0f;

	PLVector3 sunPosition = pitch_yaw_to_position( sunPitch, sunYaw );
	ape_light_set_position( sunLight, &sunPosition );

	sunBrightness = PlClamp( 0.0f, ( -sunPitch ) / 1.0f, 1.25f );
	ape_light_set_colour( sunLight, &PL_COLOURF32( DEFAULT_SUN_COLOUR.r,
	                                               DEFAULT_SUN_COLOUR.g,
	                                               DEFAULT_SUN_COLOUR.b,
	                                               sunBrightness ) );

	PLVector3 moonPosition = pitch_yaw_to_position( -sunPitch, -sunYaw );
	ape_light_set_position( moonLight, &moonPosition );

	moonBrightness = PlClamp( 0.0f, ( sunPitch ) / 1.0f, 0.25f );
	ape_light_set_colour( moonLight, &PL_COLOURF32( DEFAULT_MOON_COLOUR.r,
	                                                DEFAULT_MOON_COLOUR.g,
	                                                DEFAULT_MOON_COLOUR.b,
	                                                moonBrightness ) );

	acl_world_set_ambience( world, &PL_COLOURF32( PlClamp( 0.15f, DEFAULT_SUN_COLOUR.r * ( sunBrightness / 0.5f ), 0.45f ),
	                                              PlClamp( 0.15f, DEFAULT_SUN_COLOUR.g * ( sunBrightness / 0.5f ), 0.45f ),
	                                              PlClamp( 0.15f, DEFAULT_SUN_COLOUR.b * ( sunBrightness / 0.5f ), 0.45f ),
	                                              1.0f ) );
	acl_world_set_clear_colour( world, &PL_COLOURF32( PlClamp( 0.15f, DEFAULT_CLEAR_COLOUR.r * ( sunBrightness / 0.5f ), 0.45f ),
	                                                  PlClamp( 0.15f, DEFAULT_CLEAR_COLOUR.g * ( sunBrightness / 0.5f ), 0.45f ),
	                                                  PlClamp( 0.15f, DEFAULT_CLEAR_COLOUR.b * ( sunBrightness / 0.5f ), 0.45f ),
	                                                  1.0f ) );

#ifdef TEST_NIGHT_LIGHTS
	for ( unsigned int i = 0; i < NUM_TEST_NIGHT_LIGHTS; ++i )
	{
		if ( testLights[ i ] == NULL )
			break;

		PLColourF32 colour = ape_light_get_colour( testLights[ i ] );
		colour.a = ( moonBrightness * 2.0f ) * ( ( ( rand() % 2 ) + 1 ) / 2.0f );
		ape_light_set_colour( testLights[ i ], &colour );
	}
#endif

#if 0
	printf( "p%f y%f b%f d%d h%d m%d s%d\n", sunPitch, sunYaw, sunBrightness,
	        tox_world_get_total_days( &worldState ),
	        tox_world_get_current_hour( &worldState ),
	        tox_world_get_current_minute( &worldState ),
	        tox_world_get_seconds_in_day( &worldState ) );
	printf( "%s\n", tox_world_get_time_of_day_descriptor( tox_world_get_time_of_day( &worldState ) ) );
#endif
}

float tox_world_get_sun_brightness( void ) { return sunBrightness; }
float tox_world_get_moon_brightness( void ) { return moonBrightness; }
