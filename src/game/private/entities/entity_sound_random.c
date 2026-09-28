// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: A little like the sound entity, only uh, random!
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_shared_ptr.h"
#include "qmos/public/qm_os_string.h"
#include "qmos/public/qm_os_random.h"

#include "game_private.h"
#include "game_server.h"

static constexpr char GAME_SOUND_RANDOM_ENTITY_CLASS_NAME[] = "sound_random";

static constexpr unsigned int MAX_SAMPLES = 8;

typedef struct GameRandomSoundEntity
{
	ApeStringProperty samplePaths[ MAX_SAMPLES ][ PL_SYSTEM_MAX_PATH ];

	ApeFloatProperty maxDistance;// maximum distance
	ApeFloatProperty minDistance;// minimum distance

	ApeFloatProperty maxHeight;// maximum height
	ApeFloatProperty minHeight;// minimum height

	ApeFloatProperty maxPitch;// maximum pitch variation
	ApeFloatProperty minPitch;// minimum pitch variation

	ApeFloatProperty maxVolume;// maximum volume variation
	ApeFloatProperty minVolume;// minimum volume variation

	ApeBooleanProperty isRelative;// whether or not the random positions are relative to the player

	ApeIntegerProperty odds;
	ApeIntegerProperty interval;
	uint64_t           lastTime;

	unsigned int    numSamples;
	ApeAudioSample *samples[ MAX_SAMPLES ];
} GameRandomSoundEntity;

#define GAME_SOUND_RANDOM_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_SOUND_RANDOM_ENTITY_CLASS_NAME, GameRandomSoundEntity )

static ApeProperty properties[] = {
        APE_PROPERTY_STRING( "Sample Path 1", "Sample to load for emitting.", GameRandomSoundEntity, samplePaths[ 0 ] ),
        APE_PROPERTY_STRING( "Sample Path 2", "Sample to load for emitting.", GameRandomSoundEntity, samplePaths[ 1 ] ),
        APE_PROPERTY_STRING( "Sample Path 3", "Sample to load for emitting.", GameRandomSoundEntity, samplePaths[ 2 ] ),
        APE_PROPERTY_STRING( "Sample Path 4", "Sample to load for emitting.", GameRandomSoundEntity, samplePaths[ 3 ] ),
        APE_PROPERTY_STRING( "Sample Path 5", "Sample to load for emitting.", GameRandomSoundEntity, samplePaths[ 4 ] ),
        APE_PROPERTY_STRING( "Sample Path 6", "Sample to load for emitting.", GameRandomSoundEntity, samplePaths[ 5 ] ),
        APE_PROPERTY_STRING( "Sample Path 7", "Sample to load for emitting.", GameRandomSoundEntity, samplePaths[ 6 ] ),
        APE_PROPERTY_STRING( "Sample Path 8", "Sample to load for emitting.", GameRandomSoundEntity, samplePaths[ 7 ] ),

        APE_PROPERTY_BASIC( "Max Distance",
                            "Maximum distance that a sound can use, at random.",
                            GameRandomSoundEntity, maxDistance, FLOAT ),
        APE_PROPERTY_BASIC( "Min Distance",
                            "Minimum distance that a sound can use, at random.",
                            GameRandomSoundEntity, minDistance, FLOAT ),

        APE_PROPERTY_BASIC( "Max Height",
                            "Maximum height that a sound can use, at random.",
                            GameRandomSoundEntity, maxHeight, FLOAT ),
        APE_PROPERTY_BASIC( "Min Height",
                            "Minimum height that a sound can use, at random.",
                            GameRandomSoundEntity, minHeight, FLOAT ),

        APE_PROPERTY_BASIC( "Max Pitch",
                            "Maximum pitch level that a sound can use, at random.",
                            GameRandomSoundEntity, maxPitch, FLOAT ),
        APE_PROPERTY_BASIC( "Min Pitch",
                            "Minimum pitch level that a sound can use, at random.",
                            GameRandomSoundEntity, minPitch, FLOAT ),

        APE_PROPERTY_BASIC( "Max Volume",
                            "Maximum volume level that a sound can use, at random.",
                            GameRandomSoundEntity, maxVolume, FLOAT ),
        APE_PROPERTY_BASIC( "Min Volume",
                            "Minimum volume level that a sound can use, at random.",
                            GameRandomSoundEntity, minVolume, FLOAT ),

        APE_PROPERTY_BASIC( "Is Relative",
                            "Whether or not the random positions are relative to the player, or the entity.",
                            GameRandomSoundEntity, isRelative, BOOLEAN ),

        APE_PROPERTY_BASIC( "Odds", "The odds that a sound is going to play.", GameRandomSoundEntity, odds, INTEGER ),
        APE_PROPERTY_BASIC( "Interval", "Maximum interval between sounds.", GameRandomSoundEntity, interval, INTEGER ),
};

static void *sound_random_entity_create( [[maybe_unused]] ApeEntity *self );
static void  sound_random_entity_destroy( ApeEntity *self );
static void  sound_random_entity_spawn( ApeEntity *self );
static void  sound_random_entity_tick( ApeEntity *self, double delta );
static void  sound_random_entity_draw_editor( ApeEntity *self, bool isSelected );

const ApeEntityClassDefinition game_soundRandomEntityClass_ = {
        .name        = GAME_SOUND_RANDOM_ENTITY_CLASS_NAME,
        .description = "Lets you play random sounds in your map.",

        .createFunction  = sound_random_entity_create,
        .destroyFunction = sound_random_entity_destroy,
        .spawnFunction   = sound_random_entity_spawn,
        .tickFunction    = sound_random_entity_tick,

        .onDrawEditor = sound_random_entity_draw_editor,

        .properties    = properties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( properties ),
};

static void *sound_random_entity_create( [[maybe_unused]] ApeEntity *self )
{
	GameRandomSoundEntity *soundEntity = QM_OS_MEMORY_NEW( GameRandomSoundEntity );

	// setup some *hopefully* reasonable defaults

	soundEntity->maxDistance = soundEntity->maxHeight = 128.0f;
	soundEntity->maxPitch = soundEntity->minPitch = 1.0f;
	soundEntity->maxVolume = soundEntity->minVolume = 100.0f;

	soundEntity->odds = 1000;

	return soundEntity;
}

static void sound_random_entity_destroy( ApeEntity *self )
{
	GameRandomSoundEntity *soundEntity = GAME_SOUND_RANDOM_ENTITY( self );
	assert( soundEntity != nullptr );

	for ( unsigned int i = 0; i < soundEntity->numSamples; ++i )
	{
		ape_audio_sample_release_reference( soundEntity->samples[ i ] );
	}

	qm_os_memory_free( soundEntity );
}

static void sound_random_entity_spawn( ApeEntity *self )
{
	GameRandomSoundEntity *soundEntity = GAME_SOUND_RANDOM_ENTITY( self );
	assert( soundEntity != nullptr );

	for ( unsigned int i = 0; i < MAX_SAMPLES; ++i )
	{
		if ( *soundEntity->samplePaths[ i ] == '\0' )
		{
			continue;
		}

		if ( ( soundEntity->samples[ soundEntity->numSamples ] = ape_audio_sample_cache( soundEntity->samplePaths[ i ] ) ) == nullptr )
		{
			continue;
		}

		soundEntity->numSamples++;
	}

	if ( soundEntity->numSamples == 0 )
	{
		game_warning_( "No samples for sound_random entity, destroying!\n" );
		ape_world_node_destroy( APE_WORLD_NODE( self ) );
	}
}

static void sound_random_entity_draw_editor( ApeEntity *self, bool isSelected )
{
	if ( !isSelected )
	{
		return;
	}

	const GameRandomSoundEntity *soundEntity = GAME_SOUND_RANDOM_ENTITY( self );
	assert( soundEntity != nullptr );

	const QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( self ) );

	ComCollisionCylinder cylinder = {};
	cylinder.height               = soundEntity->maxHeight - soundEntity->minHeight;
	cylinder.radius               = soundEntity->maxDistance;
	cylinder.origin               = pos;
	cylinder.origin.y += soundEntity->minHeight;
	ape_draw_debug_cylinder( &cylinder, &PL_COLOUR_BLUE, 8 );

	if ( soundEntity->maxDistance != soundEntity->minDistance )
	{
		cylinder.radius = soundEntity->minDistance;
		ape_draw_debug_cylinder( &cylinder, &PL_COLOUR_BLUE, 8 );
	}
}

static void sound_random_entity_tick( ApeEntity *self, [[maybe_unused]] double delta )
{
	GameRandomSoundEntity *soundEntity = GAME_SOUND_RANDOM_ENTITY( self );
	assert( soundEntity != nullptr );

#if 0
	static QmMathVector3f lastSound;
	ape_draw_debug_sphere( lastSound, PL_COLOUR_RED, 16.0f );

	sound_random_entity_draw_editor( self, true );
#endif

	const uint64_t now = ape_get_tick_ms();
	if ( now - soundEntity->lastTime < soundEntity->interval )
	{
		return;
	}

	unsigned int seed = qm_os_random_seed_initialize();
	if ( soundEntity->odds > 0 && qm_os_random_int( &seed ) % soundEntity->odds != 0 )
	{
		return;
	}

	ApeAudioSample *sample = soundEntity->samples[ qm_os_random_int( &seed ) % soundEntity->numSamples ];
	assert( sample != nullptr );

	QmMathVector3f pos = {};
	if ( soundEntity->isRelative )
	{
		const ApeEntity *entity = game_server_get_local_entity_();
		if ( entity != nullptr )
		{
			pos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );
		}
	}
	else
	{
		pos = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	}

	// radius
	const float r = ( soundEntity->maxDistance - soundEntity->minDistance ) * sqrtf( qm_os_random_float( &seed, 1.0f ) );
	const float t = qm_os_random_float( &seed, 1.0f ) * 2.0f * QM_MATH_PI;
	pos.x         = pos.x + ( soundEntity->minDistance + r ) * cosf( t );
	pos.z         = pos.z + ( soundEntity->minDistance + r ) * sinf( t );

	// height
	pos.y = pos.y + ( soundEntity->minHeight + qm_os_random_float( &seed, soundEntity->maxHeight - soundEntity->minHeight ) );

	const float pitch  = soundEntity->minPitch + qm_os_random_float( &seed, soundEntity->maxPitch - soundEntity->minPitch );
	const float volume = soundEntity->minVolume + qm_os_random_float( &seed, soundEntity->maxVolume - soundEntity->minVolume );

	ape_audio_sample_emit( sample, &pos, volume, pitch );

#if 0
	lastSound = pos;
#endif

	soundEntity->lastTime = now;
}
