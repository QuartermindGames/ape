// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "audio.h"

#define AUDIO_SAMPLE_FREQ 48000
#define AUDIO_CHANNELS    2

static const YNCoreAudioDriverInterface *audioDriverInterface = NULL;
#define CallAudioDriverFunction( FUNCTION, ... )                                                                             \
	{                                                                                                                        \
		if ( audioDriverInterface != NULL && audioDriverInterface->FUNCTION ) audioDriverInterface->FUNCTION( __VA_ARGS__ ); \
	}

static YNCoreAudioSample *audioSamples = NULL;
static uint32_t     numSamples   = 0;
static uint32_t     maxSamples   = 4096;

static bool audioInitialized = false;
static bool audioPaused      = false;

static float audioVolume = 1.0f;
float        Audio_GetGlobalVolume( void )
{
	return audioVolume;
}

static struct
{
	PLVector3 position;
	PLVector3 angles;
	PLVector3 velocity;
} audioListener;

static void TestAudioCommand( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	YNCoreAudioSample *sample = Audio_CacheSample( "sounds/testing/ping.wav" );
	if ( sample == NULL )
	{
		PRINT_WARNING( "Failed to load test sample!\n" );
		return;
	}

	//Audio_EmitSample( sample, 255 );

	YnCore_AudioSample_Release( sample );
}

static void PlayAudioCommand( unsigned int argc, char **argv )
{

}

void YnCore_InitializeAudio( void )
{
	if ( audioInitialized )
		return;

	PRINT( "Initializing audio\n" );

	/* initialize the driver interface */
#if ( PL_SYSTEM_OS == PL_SYSTEM_OS_WINDOWS ) && defined( _MSC_VER )
	const YNCoreAudioDriverInterface *Audio_XAudio2_GetDriverInterface( void );
	audioDriverInterface = Audio_XAudio2_GetDriverInterface();
	if ( audioDriverInterface == NULL || !audioDriverInterface->Initialize() )
	{
		PRINT_WARNING( "Failed to initialize audio driver!\n" );
		return;
	}
#else
	const YNCoreAudioDriverInterface *YnCore_Audio_OpenAL_GetDriverInterface( void );
	audioDriverInterface = YnCore_Audio_OpenAL_GetDriverInterface();
	if ( audioDriverInterface == NULL || !audioDriverInterface->Initialize() )
	{
		PRINT_WARNING( "Failed to initialize audio driver!\n" );
		return;
	}
#endif

	/* allocate storage for our samples */
	audioSamples = PlCAlloc( maxSamples, sizeof( YNCoreAudioSample ), true );

	PlRegisterConsoleCommand( "audio.test", "Test the audio system.", 0, TestAudioCommand );
	PlRegisterConsoleCommand( "audio.play", "Play a specific sound.", 1, PlayAudioCommand );

	// reset listener
	Audio_ClearListener();

	audioInitialized = true;
}

void ogeRegisterAudioConsoleVariables_( void )
{
	PlRegisterConsoleVariable( "audio.volume", "Set the global audio volume.", "1.0", PL_VAR_F32, &audioVolume, NULL, true );
}

static void Audio_FreeSample( uint32_t s )
{
	audioSamples[ s ].path[ 0 ] = '\0';

	CallAudioDriverFunction( FreeSample, &audioSamples[ s ] );

	PL_DELETE( audioSamples[ s ].buffer );
	audioSamples[ s ].buffer = NULL;

	/* mark it as unreserved, so we can utilise it again later */
	audioSamples[ s ].reserved = false;

	numSamples--;
}

void YnCore_AudioSample_Release( YNCoreAudioSample *audioSample )
{
	audioSample->numReferences--;
	assert( audioSample->numReferences > 0 );
	if ( audioSample->numReferences < 0 )
		PRINT_WARNING( "A sample was released too many times!\n" );
}

void Audio_CleanupSamples( bool force )
{
	/* if we're not forcing cleanup, allocate a
     * new sound list to fill with the ones we
     * will retain... */
	maxSamples = numSamples;
	YNCoreAudioSample *newAudioSounds;
	if ( !force )
		newAudioSounds = PlCAllocA( maxSamples, sizeof( YNCoreAudioSample ) );

	uint32_t j = 0;
	for ( uint32_t i = 0; i < numSamples; ++i )
	{
		if ( audioSamples[ i ].numReferences > 0 && !force )
		{
			newAudioSounds[ j++ ] = audioSamples[ i ];
			continue;
		}

		if ( force && audioSamples[ i ].numReferences > 0 )
			PRINT_WARNING( "Force cleaning dirty slot %d!\n", i );

		Audio_FreeSample( i );
	}

	numSamples = j;
	if ( !force )
	{
		PL_DELETE( audioSamples );
		audioSamples = newAudioSounds;
	}
}

static int FetchCachedSoundSlotByPath( const char *path )
{
	for ( uint32_t i = 0; i < numSamples; ++i )
	{
		if ( !audioSamples[ i ].reserved )
			continue;

		if ( pl_strcasecmp( path, audioSamples[ i ].path ) != 0 )
			continue;

		return ( int ) i;
	}

	return -1;
}

/**
 * Fetches and adds the specified sound to memory,
 * if it's not in there already.
 *
 * Be sure to release the sound once you're done with
 * it!
 */
YNCoreAudioSample *Audio_CacheSample( const char *path )
{
	/* check if it's cached already */
	int s = FetchCachedSoundSlotByPath( path );
	if ( s != -1 )
	{
		audioSamples[ s ].numReferences++;
		return &audioSamples[ s ];
	}

	/* setup our new sound slot */
	uint32_t freeSlot = 0;
	for ( ; freeSlot < maxSamples; ++freeSlot )
	{
		if ( audioSamples[ freeSlot ].reserved )
			continue;

		break;
	}

	if ( freeSlot >= maxSamples )
	{
		maxSamples += 256;
		audioSamples = PlReAllocA( audioSamples, maxSamples );
	}

	YNCoreAudioSample *newSound = &audioSamples[ freeSlot ];
	snprintf( newSound->path, sizeof( newSound->path ), "%s", path );

	/* attempt to load in the wav */
	uint32_t        bufferSize;
	YNCoreAudioWaveFormat format;
	uint8_t        *data = YnCore_Audio_Wav_Load( path, &format, &bufferSize );
	if ( data == NULL )
	{
		PRINT_WARNING( "Failed to load wav: %s\n", path );
		return NULL;
	}

	/* setup our sound structure */
	newSound->reserved   = true;
	newSound->buffer     = data;
	newSound->bufferSize = bufferSize;
	newSound->format     = format;

	numSamples++;

	PRINT_DEBUG( "Cached sound, \"%s\"\n", path );

	return newSound;
}

void YnCore_AudioSample_Emit( YNCoreAudioSample *audioSample, int8_t volume )
{
#if 0
	s->channel = Mix_PlayChannel( -1, s->sample, 0 );
	if ( s->channel == -1 )
	{
		PRINT_WARNING( "Mix_PlayChannel: %s\n", Mix_GetError() );
		return;
	}

	Mix_Volume( s->channel, volume );
#endif

	CallAudioDriverFunction( EmitSample, audioSample, volume );
}

void YnCore_ShutdownAudio( void )
{
	if ( !audioInitialized )
		return;

	CallAudioDriverFunction( Shutdown );

	audioInitialized = false;
}

void YnCore_TickAudio( void )
{
	if ( !audioInitialized )
		return;

	CallAudioDriverFunction( Tick );
}

void Audio_Pause( bool pause )
{
	if ( !audioInitialized || pause == audioPaused )
		return;

	CallAudioDriverFunction( Pause, pause );

	audioPaused = pause;
}

/****************************************
 * Sources
 ****************************************/

YNCoreAudioSource *YnCore_AudioSource_Create( const PLVector3 *position, const PLVector3 *velocity )
{
	YNCoreAudioSource *source = PL_NEW( YNCoreAudioSource );
	if ( position != NULL )
		source->position = *position;
	if ( velocity != NULL )
		source->velocity = *velocity;

	CallAudioDriverFunction( CreateSource, source );

	return source;
}

void YnCore_AudioSource_Destroy( YNCoreAudioSource *audioSource )
{
	if ( audioSource == NULL )
		return;

	CallAudioDriverFunction( DestroySource, audioSource );

	PL_DELETE( audioSource );
}

void YnCore_AudioSource_Emit( YNCoreAudioSource *audioSource, YNCoreAudioSample *audioSample )
{
	assert( audioSource != NULL );
	if ( audioSource == NULL )
	{
		PRINT_WARNING( "Passed an invalid audio source handle, ignoring!\n" );
		return;
	}
	assert( audioSample != NULL );
	if ( audioSample == NULL )
	{
		PRINT_WARNING( "Passed an invalid audio sample handle, ignoring!\n" );
		return;
	}
}

/****************************************
 * Listener
 ****************************************/

void Audio_UpdateListener( const PLVector3 *position, const PLVector3 *angles, const PLVector3 *velocity )
{
	if ( position != NULL )
		audioListener.position = *position;
	if ( angles != NULL )
		audioListener.angles = *angles;
	if ( velocity != NULL )
		audioListener.velocity = *velocity;
}

/**
 * Zeros out the listener position, angles and velocity.
 */
void Audio_ClearListener( void )
{
	PL_ZERO_( audioListener );
}

PLVector3 Audio_GetListenerPosition( void )
{
	return audioListener.position;
}

PLVector3 Audio_GetListenerAngles( void )
{
	return audioListener.angles;
}

PLVector3 Audio_GetListenerVelocity( void )
{
	return audioListener.velocity;
}

/****************************************
 * Music Player
 ****************************************/

static YNCoreAudioSample *music = NULL;

void Audio_DestroyMusic( void )
{
	if ( music == NULL )
		return;

	Audio_FreeSample( FetchCachedSoundSlotByPath( music->path ) );
	music = NULL;
}

void Audio_CacheMusic( const char *path )
{
	/* free up anything we cached already */
	Audio_DestroyMusic();

	music = Audio_CacheSample( path );
}
