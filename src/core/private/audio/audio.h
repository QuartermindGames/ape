// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape_private.h"

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
	uint                 channels;
	uint                 sampleRate;
	void                *buffer;
	uint                 bufferSize;
	uint                 user;
	ApeMemoryReference   reference;
} ApeAudioSample;

typedef ApeAudioSample *( *ApeAudioSampleLoadCallback )( PLFile *file );

typedef struct ApeAudioSource
{
	PLVector3       position;
	PLVector3       velocity;
	ApeAudioSample *sample;
	uint            user;
} ApeAudioSource;

void ape_audio_initialize_( void );
void ape_audio_shutdown_( void );

void ape_audio_register_console_variables_( void );

void ape_audio_tick_( void );
void ape_audio_pause_( bool pause );

float ape_audio_get_global_volume_( void );

/////////////////////////////////////////////////////////////////////////////////////
// Sample

ApeAudioSample *ape_audio_sample_cache( const char *path );
void            ape_audio_sample_emit( ApeAudioSample *audioSample, const PLVector3 *position, float volume, float pitch );
void            ape_audio_sample_release( ApeAudioSample *audioSample );

/////////////////////////////////////////////////////////////////////////////////////
// Source

ApeAudioSource *ape_audio_source_create( const PLVector3 *position, const PLVector3 *velocity, ApeAudioSourceGroup group );
void            ape_audio_source_destroy( ApeAudioSource *audioSource );
void            ape_audio_source_emit( ApeAudioSource *audioSource, ApeAudioSample *audioSample );

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
