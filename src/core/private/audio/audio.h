// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../ape_private.h"

#include "ape/ape_public_audio.h"

PL_EXTERN_C

typedef enum ApeAudioSampleFormat
{
	APE_AUDIO_SAMPLE_FORMAT_INVALID,
	APE_AUDIO_SAMPLE_FORMAT_MONO8,
	APE_AUDIO_SAMPLE_FORMAT_STEREO8,
	APE_AUDIO_SAMPLE_FORMAT_MONO16,
	APE_AUDIO_SAMPLE_FORMAT_STEREO16,
} ApeAudioSampleFormat;

typedef struct ApeAudioSample
{
	ApeAudioSampleFormat type;
	unsigned int         channels;
	unsigned int         sampleRate;
	void                *buffer;
	unsigned int         bufferSize;
	intptr_t             user;
	ApeMemoryReference   reference;
} ApeAudioSample;

typedef ApeAudioSample *( *ApeAudioSampleLoadCallback )( PLFile *file );

typedef struct ApeAudioSource
{
	PLVector3       position;
	PLVector3       velocity;
	ApeAudioSample *sample;
	intptr_t        user;
} ApeAudioSource;

void ape_audio_initialize_( void );
void ape_audio_shutdown_( void );

void ape_audio_register_console_variables_( void );

void ape_audio_tick_( void );
void ape_audio_pause_( bool pause );

float ape_audio_get_global_volume_( void );

/////////////////////////////////////////////////////////////////////////////////////
// Driver Interface

typedef struct ApeAudioDriverInterface
{
	bool ( *initialize )( void );
	void ( *shutdown )( void );
	void ( *tick )( void );

	void ( *pause )( bool pause );

	bool ( *cacheSample )( ApeAudioSample *audioSample );
	void ( *freeSample )( ApeAudioSample *audioSample );
	void ( *emitSample )( ApeAudioSample *audioSample, const PLVector3 *position, float volume, float pitch );

	bool ( *createSource )( ApeAudioSource *audioSource );
	void ( *destroySource )( ApeAudioSource *audioSource );
} ApeAudioDriverInterface;

PL_EXTERN_C_END
