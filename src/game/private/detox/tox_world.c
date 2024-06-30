// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: World simulation state.

#include "tox_game.h"
#include "tox_world.h"

static ToxWorldState worldState;
static unsigned int secondCountdown = 0;
static const unsigned int TICKS_UNTIL_SECOND = 30;

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

ToxWorldState *tox_world_get_state( void ) { return &worldState; }

typedef enum ToxSkyLayerType
{
	TOX_SKY_LAYER_TYPE_CLOUD,
	TOX_SKY_LAYER_TYPE_CLOUD_B,
	TOX_SKY_LAYER_TYPE_STARS,

	TOX_MAX_SKY_LAYER_TYPES
} ToxSkyLayerType;

typedef struct ToxSkyLayer
{
	unsigned int id;
	const char *material;

	float baseScale;
	float baseY;
	float baseAlpha;

	float parallaxDiff;
} ToxSkyLayer;
static ToxSkyLayer skyLayers[ TOX_MAX_SKY_LAYER_TYPES ] = {
        {0, "materials/sky/cloudlayer00.mat.n",      0.85f, 12.0f, 0.5f, 100.0f},
        {0, "materials/sky/cloudlayer00.mat.n",      0.25f, 14.0f, 0.5f, 500.0f},
        {0, "materials/clouds/cloud_layer_01.mat.n", 0.1f,  16.0f, 1.0f, 700.0f},
};

void tox_world_spawn( ApeWorld *world )
{
	PL_ZERO_( worldState );

	// kick off during daylight hours
	//TODO: make this configurable via editor?
	worldState.seconds = 40000;

	ape_sky_clear_layers();
	for ( unsigned int i = 0; i < TOX_MAX_SKY_LAYER_TYPES; ++i )
	{
		skyLayers[ i ].id = ape_sky_add_layer( skyLayers[ i ].material,
		                                       skyLayers[ i ].baseScale,
		                                       skyLayers[ i ].baseY,
		                                       skyLayers[ i ].baseAlpha );
	}

	ape_world_set_clear_colour( world, &DEFAULT_CLEAR_COLOUR );

	ApeWorldNode *worldNode = &world->base;
	sunLight = ape_create_light( worldNode, &DEFAULT_SUN_POSITION, &DEFAULT_SUN_COLOUR, 0.0f,
	                             APE_LIGHT_TYPE_SUN,
	                             APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC | APE_LIGHT_FLAG_RUNTIME_SHADOWS );
	moonLight = ape_create_light( worldNode, &DEFAULT_SUN_POSITION, &DEFAULT_MOON_COLOUR, 0.0f,
	                              APE_LIGHT_TYPE_SUN,
	                              APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC | APE_LIGHT_FLAG_RUNTIME_SHADOWS );

	//TODO: scrap this - let's just pass the world into the draw call... it's safer
	ApeCamera *camera = tox_get_player_camera();
	assert( camera != nullptr );
	ape_world_node_attach( ( ApeWorldNode * ) camera, worldNode );

	// do one tick, just to let things settle...
	tox_world_tick( world );
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

void tox_world_tick( ApeWorld *world )
{
	if ( world == nullptr )
	{
		return;
	}

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
	worldState.seconds += TOX_WORLD_SECONDS_TO_HOUR / tox_globalVars.timeSpeed;
#endif

	sunYaw = tox_world_get_seconds_in_day( &worldState ) / ( TOX_WORLD_SECONDS_TO_DAY / 360.0f );
	sunPitch = sinf( PL_DEG2RAD( sunYaw + 90.0f ) ) * 2.0f;
	sunBrightness = PlClamp( 0.0f, ( -sunPitch ) / 1.0f, 1.25f );

	if ( sunLight != nullptr )
	{
		PLVector3 sunPosition = pitch_yaw_to_position( sunPitch, sunYaw );
		ape_light_set_position( sunLight, &sunPosition );
		ape_light_set_colour( sunLight, &PL_COLOURF32( DEFAULT_SUN_COLOUR.r,
		                                               DEFAULT_SUN_COLOUR.g,
		                                               DEFAULT_SUN_COLOUR.b,
		                                               sunBrightness ) );
	}

	PLVector3 moonPosition = pitch_yaw_to_position( -sunPitch, -sunYaw );
	moonBrightness = PlClamp( 0.0f, ( sunPitch ) / 1.0f, 0.25f );

	if ( moonLight != nullptr )
	{
		ape_light_set_position( moonLight, &moonPosition );
		ape_light_set_colour( moonLight, &PL_COLOURF32( DEFAULT_MOON_COLOUR.r,
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

	ape_world_set_ambience( world, &PL_COLOURF32( PlClamp( 0.05f, DEFAULT_SUN_COLOUR.r * ( sunBrightness / 0.5f ), 0.45f ),
	                                              PlClamp( 0.05f, DEFAULT_SUN_COLOUR.g * ( sunBrightness / 0.5f ), 0.45f ),
	                                              PlClamp( 0.05f, DEFAULT_SUN_COLOUR.b * ( sunBrightness / 0.5f ), 0.45f ),
	                                              1.0f ) );

	// Fog and clear should remain the same as each other, for a good little fade-out
	PLColourF32 fallbackColour = PL_COLOURF32( PlClamp( 0.0f, DEFAULT_CLEAR_COLOUR.r * ( sunBrightness / 0.5f ), 1.0f ),
	                                           PlClamp( 0.0f, DEFAULT_CLEAR_COLOUR.g * ( sunBrightness / 0.5f ), 1.0f ),
	                                           PlClamp( 0.0f, DEFAULT_CLEAR_COLOUR.b * ( sunBrightness / 0.5f ), 1.0f ),
	                                           1.0f );
	ape_world_set_clear_colour( world, &fallbackColour );
	ape_world_set_fog_colour( world, &fallbackColour );
}

float tox_world_get_sun_brightness( void ) { return sunBrightness; }
float tox_world_get_moon_brightness( void ) { return moonBrightness; }
