// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: World simulation state.

#include "detox_game.h"
#include "detox_world.h"

#define DEFAULT_SUNPOSITION PLVector3( -2.0f, -2.0f, 0.0f )
#define DEFAULT_SUNCOLOUR   PL_COLOURF32( 1.0f, 1.0f, 1.0f, 0.75f )

static ToxWorldState worldState;
static unsigned int secondCountdown = 0;
static const unsigned int TICKS_UNTIL_SECOND = 30;

static ApeWorld *currentWorld = NULL;

static ApeLight *sunLight = NULL;
static float sunYaw = 0.0f;
static float sunPitch = 0.0f;

const ToxWorldState *tox_world_get_state( void ) { return &worldState; }

void tox_world_spawn( ApeWorld *world )
{
	currentWorld = world;

	PL_ZERO_( worldState );

	sunLight = ape_light_create( &DEFAULT_SUNPOSITION, &DEFAULT_SUNCOLOUR, 0.0f,
	                             APE_LIGHT_TYPE_SUN,
	                             APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_RUNTIME_SHADOWS );
	ape_world_attach_light( world, sunLight );
}

void tox_world_tick( void )
{
	if ( currentWorld == NULL )
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
	worldState.seconds += TOX_WORLD_SECONDS_TO_HOUR / 110;
#endif

	sunYaw = tox_world_get_seconds_in_day( &worldState ) / ( TOX_WORLD_SECONDS_TO_DAY / 360.0f );
	sunPitch = sinf( PL_DEG2RAD( sunYaw + 90.0f ) ) * 2.0f;

	// This is a very convoluted way to set the pitch and yaw...
	PLVector3 sunPosition = { 1.0f, sunPitch, 0.0f };
	PLMatrix4 sunMatrix = PlMatrix4Identity();
	sunMatrix = PlMultiplyMatrix4( PlTranslateMatrix4( sunPosition ), &sunMatrix );
	sunMatrix = PlMultiplyMatrix4( PlRotateMatrix4( PL_DEG2RAD( sunYaw ), &( PLVector3 ){ 0.0f, 1.0f, 0.0f } ), &sunMatrix );
	sunPosition.x = sunMatrix.m[ 0 ];
	sunPosition.z = sunMatrix.m[ 8 ];
	ape_light_set_position( sunLight, &sunPosition );

	float sunBrightness = PlClamp( 0.0f, ( -sunPitch ) / 1.0f, 0.75f );
	ape_light_set_colour( sunLight, &PL_COLOURF32( DEFAULT_SUNCOLOUR.r,
	                                               DEFAULT_SUNCOLOUR.g,
	                                               DEFAULT_SUNCOLOUR.b,
	                                               sunBrightness ) );

#if 0
	printf( "p%f y%f b%f d%d h%d m%d s%d\n", sunPitch, sunYaw, sunBrightness,
	        tox_world_get_total_days( &worldState ),
	        tox_world_get_current_hour( &worldState ),
	        tox_world_get_current_minute( &worldState ),
	        tox_world_get_seconds_in_day( &worldState ) );
	printf( "%s\n", tox_world_get_time_of_day_descriptor( tox_world_get_time_of_day( &worldState ) ) );
#endif
}
