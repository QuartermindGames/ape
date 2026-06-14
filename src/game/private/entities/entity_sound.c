// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Sound emitter entity, like ambient_generic in Source.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_shared_ptr.h"
#include "qmos/public/qm_os_string.h"

#include "game_private.h"

static constexpr char GAME_SOUND_ENTITY_CLASS_NAME[] = "sound";

typedef enum GameSoundEntityFlag
{
	QM_OS_BIT_FLAG( GAME_SOUND_ENTITY_FLAG_LOOP, 0 ),
	QM_OS_BIT_FLAG( GAME_SOUND_ENTITY_FLAG_GLOBAL, 1 ),
	QM_OS_BIT_FLAG( GAME_SOUND_ENTITY_FLAG_WAIT, 2 ),
} GameSoundEntityFlag;

typedef struct GameSoundEntity
{
	ApeStringProperty samplePath[ PL_SYSTEM_MAX_PATH ];

	ApeFloatProperty radius;
	ApeFloatProperty volume;
	ApeFloatProperty pitch;

	ApeIntegerProperty flags;

	ApeAudioSample *sample;
	ApeAudioSource *source;
} GameSoundEntity;

#define GAME_SOUND_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_SOUND_ENTITY_CLASS_NAME, GameSoundEntity )

static void *sound_entity_create( [[maybe_unused]] ApeEntity *self )
{
	ApeAudioSource *source = ape_audio_source_create( &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO, APE_AUDIO_SOURCE_GROUP_GENERIC );
	if ( source == nullptr )
	{
		return nullptr;
	}

	GameSoundEntity *soundEntity = QM_OS_MEMORY_NEW( GameSoundEntity );
	if ( soundEntity != nullptr )
	{
		soundEntity->source = source;
		soundEntity->volume = 100.0f;
		soundEntity->pitch  = 1.0f;
	}

	return soundEntity;
}

static void sound_entity_destroy( ApeEntity *self )
{
	GameSoundEntity *soundEntity = GAME_SOUND_ENTITY( self );
	assert( soundEntity != nullptr );

	ape_audio_source_destroy( soundEntity->source );

	qm_os_memory_free( soundEntity );
}

static void sound_entity_spawn( ApeEntity *self )
{
	GameSoundEntity *soundEntity = GAME_SOUND_ENTITY( self );
	assert( soundEntity != nullptr );

	soundEntity->sample = ape_audio_sample_cache( soundEntity->samplePath );
	if ( soundEntity->sample == nullptr )
	{
		ape_world_node_destroy( APE_WORLD_NODE( self ) );
		return;
	}

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	ape_audio_source_set_position( soundEntity->source, &pos );
	ape_audio_source_set_pitch( soundEntity->source, soundEntity->pitch );
	ape_audio_source_set_volume( soundEntity->source, soundEntity->volume );
	ape_audio_source_set_loop( soundEntity->source, soundEntity->flags & GAME_SOUND_ENTITY_FLAG_LOOP );

	if ( !( soundEntity->flags & GAME_SOUND_ENTITY_FLAG_WAIT ) )
	{
		ape_audio_source_emit( soundEntity->source, soundEntity->sample );
	}
}

static void sound_entity_tick( ApeEntity *self, double delta )
{
	GameSoundEntity *soundEntity = GAME_SOUND_ENTITY( self );
	assert( soundEntity != nullptr );

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	ape_audio_source_set_position( soundEntity->source, &pos );
}

static ApeProperty properties[] = {
        APE_PROPERTY_STRING( "Sample Path", "Sample to load for emitting.", GameSoundEntity, samplePath ),
        APE_PROPERTY_BASIC( "Radius", "Maximum radius of the sound.", GameSoundEntity, radius, FLOAT ),
        APE_PROPERTY_BASIC( "Volume", "Volume of the sound.", GameSoundEntity, volume, FLOAT ),
        APE_PROPERTY_BASIC( "Pitch", "Pitch of the sound.", GameSoundEntity, pitch, FLOAT ),

        APE_PROPERTY_BITFLAG( "Loop", "If true, sound will loop forever.", GameSoundEntity, flags, GAME_SOUND_ENTITY_FLAG_LOOP ),
        APE_PROPERTY_BITFLAG( "Global", "If true, game will sound everywhere.", GameSoundEntity, flags, GAME_SOUND_ENTITY_FLAG_GLOBAL ),
        APE_PROPERTY_BITFLAG( "Wait", "If true, the sound will not immediately play.", GameSoundEntity, flags, GAME_SOUND_ENTITY_FLAG_WAIT ),
};

const ApeEntityClassDefinition game_soundEntityClass_ = {
        .name        = GAME_SOUND_ENTITY_CLASS_NAME,
        .description = "Can play sound samples in the world or globally.",

        .createFunction  = sound_entity_create,
        .destroyFunction = sound_entity_destroy,
        .spawnFunction   = sound_entity_spawn,
        .tickFunction    = sound_entity_tick,

        .properties    = properties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( properties ),
};
