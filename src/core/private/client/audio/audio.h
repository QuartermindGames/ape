// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "core_private.h"

PL_EXTERN_C

typedef struct YNCoreAudioXWBRecord YNCoreAudioXWBRecord;
typedef struct YNCoreAudioXWB YNCoreAudioXWB;

YNCoreAudioXWB *YnCore_Audio_XWB_Load( const char *path );
void YnCore_Audio_XWB_Destroy( YNCoreAudioXWB *xwb );

/**
 * This list provides a number of
 * somewhat standard presets for
 * reverb.
 */
typedef enum YNCoreAudioReverbPreset
{
	AUDIO_REVERB_PRESET_FOREST,
	AUDIO_REVERB_PRESET_DEFAULT,
	AUDIO_REVERB_PRESET_GENERIC,
	AUDIO_REVERB_PRESET_PADDEDCELL,
	AUDIO_REVERB_PRESET_ROOM,
	AUDIO_REVERB_PRESET_BATHROOM,
	AUDIO_REVERB_PRESET_LIVINGROOM,
	AUDIO_REVERB_PRESET_STONEROOM,
	AUDIO_REVERB_PRESET_AUDITORIUM,
	AUDIO_REVERB_PRESET_CONCERTHALL,
	AUDIO_REVERB_PRESET_CAVE,
	AUDIO_REVERB_PRESET_ARENA,
	AUDIO_REVERB_PRESET_HANGAR,
	AUDIO_REVERB_PRESET_CARPETEDHALLWAY,
	AUDIO_REVERB_PRESET_HALLWAY,
	AUDIO_REVERB_PRESET_STONECORRIDOR,
	AUDIO_REVERB_PRESET_ALLEY,
	AUDIO_REVERB_PRESET_CITY,
	AUDIO_REVERB_PRESET_MOUNTAINS,
	AUDIO_REVERB_PRESET_QUARRY,
	AUDIO_REVERB_PRESET_PLAIN,
	AUDIO_REVERB_PRESET_PARKINGLOT,
	AUDIO_REVERB_PRESET_SEWERPIPE,
	AUDIO_REVERB_PRESET_UNDERWATER,
	AUDIO_REVERB_PRESET_SMALLROOM,
	AUDIO_REVERB_PRESET_MEDIUMROOM,
	AUDIO_REVERB_PRESET_LARGEROOM,
	AUDIO_REVERB_PRESET_MEDIUMHALL,
	AUDIO_REVERB_PRESET_LARGEHALL,
	AUDIO_REVERB_PRESET_PLATE,

	AUDIO_MAX_REVERB_PRESETS
} YNCoreAudioReverbPreset;

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

typedef struct YNCoreAudioSample
{
	char path[ PL_SYSTEM_MAX_PATH ];
	bool reserved;
	int numReferences;
	int channel;
	YNCoreAudioWaveFormat format;
	uint8_t *buffer;
	unsigned int bufferSize;
	void *user;
} YNCoreAudioSample;

typedef struct YNCoreAudioSource
{
	PLVector3 position;
	PLVector3 velocity;
	void *user;
} YNCoreAudioSource;

void YnCore_InitializeAudio( void );
void YnCore_ShutdownAudio( void );

void YnCore_TickAudio( void );
void Audio_Pause( bool pause );

void Audio_UpdateListener( const PLVector3 *position, const PLVector3 *angles, const PLVector3 *velocity );
void Audio_ClearListener( void );
PLVector3 Audio_GetListenerPosition( void );
PLVector3 Audio_GetListenerAngles( void );
PLVector3 Audio_GetListenerVelocity( void );

float Audio_GetGlobalVolume( void );

void Audio_CleanupSamples( bool force );
YNCoreAudioSample *Audio_CacheSample( const char *path );
void YnCore_AudioSample_Emit( YNCoreAudioSample *audioSample, int8_t volume );
void YnCore_AudioSample_Release( YNCoreAudioSample *audioSample );

YNCoreAudioSource *YnCore_AudioSource_Create( const PLVector3 *position, const PLVector3 *velocity );
void YnCore_AudioSource_Destroy( YNCoreAudioSource *audioSource );
void YnCore_AudioSource_Emit( YNCoreAudioSource *audioSource, YNCoreAudioSample *audioSample );

void *YnCore_Audio_Wav_Load( const char *path, YNCoreAudioWaveFormat *waveFormatEx, unsigned int *bufferSize );

typedef struct YNCoreAudioDriverInterface
{
	bool ( *Initialize )( void );
	void ( *Shutdown )( void );
	void ( *Tick )( void );

	void ( *Pause )( bool pause );

	bool ( *CacheSample )( YNCoreAudioSample *audioSample );
	void ( *FreeSample )( YNCoreAudioSample *audioSample );
	void ( *EmitSample )( YNCoreAudioSample *audioSample, int8_t volume );

	bool ( *CreateSource )( YNCoreAudioSource *audioSource );
	void ( *DestroySource )( YNCoreAudioSource *audioSource );
} YNCoreAudioDriverInterface;

PL_EXTERN_C_END
