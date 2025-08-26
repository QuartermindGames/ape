// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

/**
 * Cached sample in memory.
 */
typedef struct ApeAudioSample ApeAudioSample;

/**
 * The point a sound emits from.
 */
typedef struct ApeAudioSource ApeAudioSource;

typedef enum ApeAudioSourceGroup
{
	APE_AUDIO_SOURCE_GROUP_GENERIC,
	APE_AUDIO_SOURCE_MAX_GROUPS
} ApeAudioSourceGroup;

/**
 * This list provides a number of
 * somewhat standard presets for
 * reverb.
 */
typedef enum ApeAudioReverbPreset
{
	APE_AUDIO_REVERB_PRESET_NONE,
	APE_AUDIO_REVERB_PRESET_FOREST,
	APE_AUDIO_REVERB_PRESET_DEFAULT,
	APE_AUDIO_REVERB_PRESET_GENERIC,
	APE_AUDIO_REVERB_PRESET_PADDEDCELL,
	APE_AUDIO_REVERB_PRESET_ROOM,
	APE_AUDIO_REVERB_PRESET_BATHROOM,
	APE_AUDIO_REVERB_PRESET_LIVINGROOM,
	APE_AUDIO_REVERB_PRESET_STONEROOM,
	APE_AUDIO_REVERB_PRESET_AUDITORIUM,
	APE_AUDIO_REVERB_PRESET_CONCERTHALL,
	APE_AUDIO_REVERB_PRESET_CAVE,
	APE_AUDIO_REVERB_PRESET_ARENA,
	APE_AUDIO_REVERB_PRESET_HANGAR,
	APE_AUDIO_REVERB_PRESET_CARPETEDHALLWAY,
	APE_AUDIO_REVERB_PRESET_HALLWAY,
	APE_AUDIO_REVERB_PRESET_STONECORRIDOR,
	APE_AUDIO_REVERB_PRESET_ALLEY,
	APE_AUDIO_REVERB_PRESET_CITY,
	APE_AUDIO_REVERB_PRESET_MOUNTAINS,
	APE_AUDIO_REVERB_PRESET_QUARRY,
	APE_AUDIO_REVERB_PRESET_PLAIN,
	APE_AUDIO_REVERB_PRESET_PARKINGLOT,
	APE_AUDIO_REVERB_PRESET_SEWERPIPE,
	APE_AUDIO_REVERB_PRESET_UNDERWATER,
	APE_AUDIO_REVERB_PRESET_SMALLROOM,
	APE_AUDIO_REVERB_PRESET_MEDIUMROOM,
	APE_AUDIO_REVERB_PRESET_LARGEROOM,
	APE_AUDIO_REVERB_PRESET_MEDIUMHALL,
	APE_AUDIO_REVERB_PRESET_LARGEHALL,
	APE_AUDIO_REVERB_PRESET_PLATE,

	APE_AUDIO_MAX_REVERB_PRESETS
} ApeAudioReverbPreset;

typedef struct ApeAudioEffectType
{
	const char          *name;
	ApeAudioReverbPreset effect;
} ApeAudioEffectType;
extern const ApeAudioEffectType APE_AUDIO_EFFECT_TYPES[];
extern const unsigned int       APE_NUM_AUDIO_EFFECT_TYPES;

/////////////////////////////////////////////////////////////////////////////////////
// Listener

/**
 * Update the main listener position.
 *
 * @param position 	Position of the listener.
 * @param angles 	Angles of the listener.
 * @param velocity 	Velocity of the listener.
 */
void ape_audio_update_listener( const QmMathVector3f *position, const QmMathVector3f *angles, const QmMathVector3f *velocity );

/**
 * Clear the properties of the current listener.
 */
void ape_audio_clear_listener( void );

/**
 * Fetch the current position of the listener.
 *
 * @return 	Vector position of the listener.
 */
QmMathVector3f ape_audio_get_listener_position( void );

/**
 * Fetch the current angles of the listener.
 *
 * @return 	Vector angles of the listener.
 */
QmMathVector3f ape_audio_get_listener_angles( void );

/**
 * Fetch the current velocity of the listener.
 *
 * @return 	Vector velocity of the listener.
 */
QmMathVector3f ape_audio_get_listener_velocity( void );

/////////////////////////////////////////////////////////////////////////////////////
// Sample

ApeAudioSample *ape_audio_sample_cache( const char *path );
void            ape_audio_sample_emit( ApeAudioSample *audioSample, const QmMathVector3f *position, float volume, float pitch );
void            ape_audio_sample_release( ApeAudioSample *audioSample );

/////////////////////////////////////////////////////////////////////////////////////
// Source

ApeAudioSource *ape_audio_source_create( const QmMathVector3f *position, const QmMathVector3f *velocity, ApeAudioSourceGroup group );
void            ape_audio_source_destroy( ApeAudioSource *audioSource );
void            ape_audio_source_set_position( ApeAudioSource *audioSource, const QmMathVector3f *position );
void            ape_audio_source_set_velocity( ApeAudioSource *audioSource, const QmMathVector3f *velocity );
void            ape_audio_source_emit( ApeAudioSource *audioSource, ApeAudioSample *audioSample );

PL_EXTERN_C_END
