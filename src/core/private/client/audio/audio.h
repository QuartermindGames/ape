// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "ape_private.h"

PL_EXTERN_C

typedef struct YNCoreAudioXWBRecord YNCoreAudioXWBRecord;
typedef struct AclAudioXwb AclAudioXwb;

AclAudioXwb *acl_audio_xwb_load_file( const char *path );
void acl_audio_xwb_destroy( AclAudioXwb *xwb );

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

typedef struct AclAudioEffectType
{
	const char *name;
	ApeAudioReverbPreset effect;
} AclAudioEffectType;
extern const AclAudioEffectType APE_AUDIO_EFFECT_TYPES[];
extern const unsigned int APE_NUM_AUDIO_EFFECT_TYPES;

/**
 * WARNING: DO NOT CHANGE THIS!!
 * This should match with the 'fmt ' structure
 * within a WAV file.
 */
typedef struct YNCoreAudioWaveFormat
{
	uint16_t formatTag;
	uint16_t channels;
	uint32_t samplesPerSec;
	uint32_t avgBytesPerSec;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	uint16_t size;
} YNCoreAudioWaveFormat;

typedef struct ApeAudioSample
{
	char path[ PL_SYSTEM_MAX_PATH ];
	bool reserved;
	int numReferences;
	int channel;
	YNCoreAudioWaveFormat format;
	uint8_t *buffer;
	unsigned int bufferSize;
	void *user;
} ApeAudioSample;

typedef struct ApeAudioSource
{
	PLVector3 position;
	PLVector3 velocity;
	void *user;
} ApeAudioSource;

void apeInitializeAudio_( void );
void apeShutdownAudio_( void );

void ss_acl_audio_register_console_variables_( void );

void apeTickAudio_( void );
void Audio_Pause( bool pause );

void Audio_UpdateListener( const PLVector3 *position, const PLVector3 *angles, const PLVector3 *velocity );
void apeClearAudioListener( void );
PLVector3 Audio_GetListenerPosition( void );
PLVector3 Audio_GetListenerAngles( void );
PLVector3 Audio_GetListenerVelocity( void );

float Audio_GetGlobalVolume( void );

void Audio_CleanupSamples( bool force );
ApeAudioSample *Audio_CacheSample( const char *path );
void YnCore_AudioSample_Emit( ApeAudioSample *audioSample, int8_t volume );
void apeReleaseAudioSample( ApeAudioSample *audioSample );

ApeAudioSource *YnCore_AudioSource_Create( const PLVector3 *position, const PLVector3 *velocity );
void YnCore_AudioSource_Destroy( ApeAudioSource *audioSource );
void YnCore_AudioSource_Emit( ApeAudioSource *audioSource, ApeAudioSample *audioSample );

void *apeLoadWav( const char *path, YNCoreAudioWaveFormat *waveFormatEx, unsigned int *bufferSize );

typedef struct ApeAudioDriverInterface
{
	bool ( *Initialize )( void );
	void ( *Shutdown )( void );
	void ( *Tick )( void );

	void ( *Pause )( bool pause );

	bool ( *CacheSample )( ApeAudioSample *audioSample );
	void ( *FreeSample )( ApeAudioSample *audioSample );
	void ( *EmitSample )( ApeAudioSample *audioSample, int8_t volume );

	bool ( *CreateSource )( ApeAudioSource *audioSource );
	void ( *DestroySource )( ApeAudioSource *audioSource );
} ApeAudioDriverInterface;

PL_EXTERN_C_END
