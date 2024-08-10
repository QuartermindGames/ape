// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape_private.h"

PL_EXTERN_C

typedef struct YNCoreAudioXWBRecord YNCoreAudioXWBRecord;
typedef struct AclAudioXwb          AclAudioXwb;

AclAudioXwb *acl_audio_xwb_load_file( const char *path );
void         acl_audio_xwb_destroy( AclAudioXwb *xwb );

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

/**
 * WARNING: DO NOT CHANGE THIS!!
 * This should match with the 'fmt ' structure
 * within a WAV file.
 */
typedef struct ApeAudioWaveFormat
{
	uint16_t formatTag;
	uint16_t channels;
	uint32_t samplesPerSec;
	uint32_t avgBytesPerSec;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	uint16_t size;
} ApeAudioWaveFormat;

typedef struct ApeAudioSample
{
	char               path[ PL_SYSTEM_MAX_PATH ];
	bool               reserved;
	int                numReferences;
	int                channel;
	ApeAudioWaveFormat format;
	uint8_t           *buffer;
	unsigned int       bufferSize;
	void              *user;
} ApeAudioSample;

typedef struct ApeAudioSource
{
	PLVector3 position;
	PLVector3 velocity;
	void     *user;
} ApeAudioSource;

void ape_audio_initialize_( void );
void ape_audio_shutdown_( void );

void ape_audio_register_console_variables_( void );

void ape_audio_tick_( void );
void ape_audio_pause_( bool pause );

void      ape_audio_update_listener( const PLVector3 *position, const PLVector3 *angles, const PLVector3 *velocity );
void      ape_audio_clear_listener( void );
PLVector3 ape_audio_get_listener_position( void );
PLVector3 ape_audio_get_listener_angles( void );
PLVector3 ape_audio_get_listener_velocity( void );

float ape_audio_get_global_volume_( void );

void ape_audio_cleanup_samples_( bool force );

ApeAudioSample *ape_audio_sample_cache_( const char *path );
void            ape_audio_sample_emit( ApeAudioSample *audioSample, int8_t volume );
void            ape_audio_sample_release_( ApeAudioSample *audioSample );

ApeAudioSource *ape_audio_source_create( const PLVector3 *position, const PLVector3 *velocity );
void            ape_audio_source_destroy( ApeAudioSource *audioSource );
void            ape_audio_source_emit( ApeAudioSource *audioSource, ApeAudioSample *audioSample );

void *ape_audio_wav_load_( const char *path, ApeAudioWaveFormat *waveFormatEx, unsigned int *bufferSize );

typedef struct ApeAudioDriverInterface
{
	bool ( *initialize )( void );
	void ( *shutdown )( void );
	void ( *tick )( void );

	void ( *pause )( bool pause );

	bool ( *cacheSample )( ApeAudioSample *audioSample );
	void ( *freeSample )( ApeAudioSample *audioSample );
	void ( *emitSample )( ApeAudioSample *audioSample, int8_t volume );

	bool ( *createSource )( ApeAudioSource *audioSource );
	void ( *destroySource )( ApeAudioSource *audioSource );
} ApeAudioDriverInterface;

PL_EXTERN_C_END
