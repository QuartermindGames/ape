// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "audio.h"

#define AUDIO_SAMPLE_FREQ 48000
#define AUDIO_CHANNELS    2

// Provided as a list so we can hand this to the tools later...
// There are some odd ones in here, because the RFII developers
// seemingly removed EAX effects, only to bring them back and
// invent their own presets (with mixed-case names)...
const ApeAudioEffectType APE_AUDIO_EFFECT_TYPES[] = {
        { "none", APE_AUDIO_REVERB_PRESET_NONE },
        { "forest", APE_AUDIO_REVERB_PRESET_FOREST },
        { "default", APE_AUDIO_REVERB_PRESET_DEFAULT },
        { "generic", APE_AUDIO_REVERB_PRESET_GENERIC },
        { "paddedcell", APE_AUDIO_REVERB_PRESET_PADDEDCELL },
        { "room", APE_AUDIO_REVERB_PRESET_ROOM },
        { "bathroom", APE_AUDIO_REVERB_PRESET_BATHROOM },
        { "livingroom", APE_AUDIO_REVERB_PRESET_LIVINGROOM },
        { "stoneroom", APE_AUDIO_REVERB_PRESET_STONEROOM },
        { "auditorium", APE_AUDIO_REVERB_PRESET_AUDITORIUM },
        { "concerthall", APE_AUDIO_REVERB_PRESET_CONCERTHALL },
        { "cave", APE_AUDIO_REVERB_PRESET_CAVE },
        { "arena", APE_AUDIO_REVERB_PRESET_ARENA },
        { "hangar", APE_AUDIO_REVERB_PRESET_HANGAR },
        { "carpetedhallway", APE_AUDIO_REVERB_PRESET_CARPETEDHALLWAY },
        { "hallway", APE_AUDIO_REVERB_PRESET_HALLWAY },
        { "stonecorridor", APE_AUDIO_REVERB_PRESET_STONECORRIDOR },
        { "alley", APE_AUDIO_REVERB_PRESET_ALLEY },
        { "city", APE_AUDIO_REVERB_PRESET_CITY },
        { "mountains", APE_AUDIO_REVERB_PRESET_MOUNTAINS },
        { "quarry", APE_AUDIO_REVERB_PRESET_QUARRY },
        { "plain", APE_AUDIO_REVERB_PRESET_PLAIN },
        { "parkinglot", APE_AUDIO_REVERB_PRESET_PARKINGLOT },
        { "sewerpipe", APE_AUDIO_REVERB_PRESET_SEWERPIPE },
        { "underwater", APE_AUDIO_REVERB_PRESET_UNDERWATER },
        { "smallroom", APE_AUDIO_REVERB_PRESET_SMALLROOM },
        { "mediumroom", APE_AUDIO_REVERB_PRESET_MEDIUMROOM },
        { "largeroom", APE_AUDIO_REVERB_PRESET_LARGEROOM },
        { "mediumhall", APE_AUDIO_REVERB_PRESET_MEDIUMHALL },
        { "largehall", APE_AUDIO_REVERB_PRESET_LARGEHALL },
        { "plate", APE_AUDIO_REVERB_PRESET_PLATE },
        // types introduced in RFII
        { "hall", APE_AUDIO_REVERB_PRESET_HALLWAY },
        { "pipe", APE_AUDIO_REVERB_PRESET_SEWERPIPE },
};
const unsigned int APE_NUM_AUDIO_EFFECT_TYPES = PL_ARRAY_ELEMENTS( APE_AUDIO_EFFECT_TYPES );

static const ApeAudioDriverInterface *audioDriverInterface = nullptr;
#define CallAudioDriverFunction( FUNCTION, ... )                                                                                \
	{                                                                                                                           \
		if ( audioDriverInterface != nullptr && audioDriverInterface->FUNCTION ) audioDriverInterface->FUNCTION( __VA_ARGS__ ); \
	}

static ApeAudioSample *audioSamples = nullptr;
static uint32_t        numSamples   = 0;
static uint32_t        maxSamples   = 4096;

static bool audioInitialized = false;
static bool audioPaused      = false;

static float audioVolume = 1.0f;
float        ape_audio_get_global_volume_( void )
{
	return audioVolume;
}

static struct
{
	PLVector3 position;
	PLVector3 angles;
	PLVector3 velocity;
} audioListener;

static void test_audio_command( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	ApeAudioSample *sample = ape_audio_sample_cache_( "sounds/testing/ping.wav" );
	if ( sample == nullptr )
	{
		ape_warning_( "Failed to load test sample!\n" );
		return;
	}

	ape_audio_sample_emit( sample, 100 );

	ape_audio_sample_release_( sample );
}

static void play_audio_command( unsigned int argc, char **argv )
{
}

void ape_audio_initialize_( void )
{
	if ( audioInitialized )
	{
		return;
	}

	PRINT( "Initializing audio\n" );

	/* initialize the driver interface */
#if ( PL_SYSTEM_OS == PL_SYSTEM_OS_WINDOWS ) && defined( _MSC_VER )
	const ApeAudioDriverInterface *Audio_XAudio2_GetDriverInterface( void );
	audioDriverInterface = Audio_XAudio2_GetDriverInterface();
	if ( audioDriverInterface == nullptr || !audioDriverInterface->Initialize() )
	{
		ape_warning_( "Failed to initialize audio driver!\n" );
		return;
	}
#else
	const ApeAudioDriverInterface *ape_audio_get_driver_interface_( void );
	audioDriverInterface = ape_audio_get_driver_interface_();
	if ( audioDriverInterface == nullptr || !audioDriverInterface->initialize() )
	{
		ape_warning_( "Failed to initialize audio driver!\n" );
		return;
	}
#endif

	/* allocate storage for our samples */
	audioSamples = PlCAlloc( maxSamples, sizeof( ApeAudioSample ), true );

	PlRegisterConsoleCommand( "audio/test", "Test the audio system.", 0, test_audio_command );
	PlRegisterConsoleCommand( "audio/play", "Play a specific sound.", 1, play_audio_command );

	// reset listener
	ape_audio_clear_listener();

	audioInitialized = true;
}

void ape_audio_register_console_variables_( void )
{
	PlRegisterConsoleVariable( "audio/volume", "Set the global audio volume.", "1.0", PL_VAR_F32, &audioVolume, nullptr, true );
}

static void free_sample( uint32_t s )
{
	audioSamples[ s ].path[ 0 ] = '\0';

	CallAudioDriverFunction( freeSample, &audioSamples[ s ] );

	PL_DELETE( audioSamples[ s ].buffer );
	audioSamples[ s ].buffer = nullptr;

	/* mark it as unreserved, so we can utilise it again later */
	audioSamples[ s ].reserved = false;

	numSamples--;
}

void ape_audio_sample_release_( ApeAudioSample *audioSample )
{
	audioSample->numReferences--;
	assert( audioSample->numReferences > 0 );
	if ( audioSample->numReferences < 0 )
	{
		ape_warning_( "A sample was released too many times!\n" );
	}
}

void ape_audio_cleanup_samples_( bool force )
{
	/* if we're not forcing cleanup, allocate a
     * new sound list to fill with the ones we
     * will retain... */
	maxSamples = numSamples;
	ApeAudioSample *newAudioSounds;
	if ( !force )
		newAudioSounds = PlCAllocA( maxSamples, sizeof( ApeAudioSample ) );

	uint32_t j = 0;
	for ( uint32_t i = 0; i < numSamples; ++i )
	{
		if ( audioSamples[ i ].numReferences > 0 && !force )
		{
			newAudioSounds[ j++ ] = audioSamples[ i ];
			continue;
		}

		if ( force && audioSamples[ i ].numReferences > 0 )
		{
			ape_warning_( "Force cleaning dirty slot %d!\n", i );
		}

		free_sample( i );
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
		{
			continue;
		}

		if ( pl_strcasecmp( path, audioSamples[ i ].path ) != 0 )
		{
			continue;
		}

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
ApeAudioSample *ape_audio_sample_cache_( const char *path )
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
		{
			continue;
		}

		break;
	}

	if ( freeSlot >= maxSamples )
	{
		maxSamples += 256;
		audioSamples = PlReAllocA( audioSamples, maxSamples );
	}

	ApeAudioSample *newSound = &audioSamples[ freeSlot ];
	snprintf( newSound->path, sizeof( newSound->path ), "%s", path );

	/* attempt to load in the wav */
	uint32_t           bufferSize;
	ApeAudioWaveFormat format;
	uint8_t           *data = ape_audio_wav_load_( path, &format, &bufferSize );
	if ( data == nullptr )
	{
		ape_warning_( "Failed to load wav: %s\n", path );
		return nullptr;
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

void ape_audio_sample_emit( ApeAudioSample *audioSample, int8_t volume )
{
#if 0
	s->channel = Mix_PlayChannel( -1, s->sample, 0 );
	if ( s->channel == -1 )
	{
		ape_warning_( "Mix_PlayChannel: %s\n", Mix_GetError() );
		return;
	}

	Mix_Volume( s->channel, volume );
#endif

	CallAudioDriverFunction( emitSample, audioSample, volume );
}

void ape_audio_shutdown_( void )
{
	if ( !audioInitialized )
	{
		return;
	}

	CallAudioDriverFunction( shutdown );

	audioInitialized = false;
}

void ape_audio_tick_( void )
{
	if ( !audioInitialized )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	CallAudioDriverFunction( tick );

	COM_PROFILE_FUNCTION_END();
}

void ape_audio_pause_( bool pause )
{
	if ( !audioInitialized || pause == audioPaused )
	{
		return;
	}

	CallAudioDriverFunction( pause, pause );

	audioPaused = pause;
}

/****************************************
 * Sources
 ****************************************/

ApeAudioSource *ape_audio_source_create( const PLVector3 *position, const PLVector3 *velocity )
{
	ApeAudioSource *source = PL_NEW( ApeAudioSource );
	if ( position != nullptr )
		source->position = *position;
	if ( velocity != nullptr )
		source->velocity = *velocity;

	CallAudioDriverFunction( createSource, source );

	return source;
}

void ape_audio_source_destroy( ApeAudioSource *audioSource )
{
	if ( audioSource == nullptr )
	{
		return;
	}

	CallAudioDriverFunction( destroySource, audioSource );

	PL_DELETE( audioSource );
}

void ape_audio_source_emit( ApeAudioSource *audioSource, ApeAudioSample *audioSample )
{
	if ( audioSource == nullptr )
	{
		ape_warning_( "Passed an invalid audio source handle, ignoring!\n" );
		return;
	}
	if ( audioSample == nullptr )
	{
		ape_warning_( "Passed an invalid audio sample handle, ignoring!\n" );
		return;
	}
}

/****************************************
 * Listener
 ****************************************/

void ape_audio_update_listener( const PLVector3 *position, const PLVector3 *angles, const PLVector3 *velocity )
{
	if ( position != nullptr )
	{
		audioListener.position = *position;
	}
	if ( angles != nullptr )
	{
		audioListener.angles = *angles;
	}
	if ( velocity != nullptr )
	{
		audioListener.velocity = *velocity;
	}
}

/**
 * Zeros out the listener position, angles and velocity.
 */
void ape_audio_clear_listener( void )
{
	PL_ZERO_( audioListener );
}

PLVector3 ape_audio_get_listener_position( void )
{
	return audioListener.position;
}

PLVector3 ape_audio_get_listener_angles( void )
{
	return audioListener.angles;
}

PLVector3 ape_audio_get_listener_velocity( void )
{
	return audioListener.velocity;
}

/****************************************
 * Music Player
 ****************************************/

static ApeAudioSample *music = nullptr;

void ape_audio_destroy_music( void )
{
	if ( music == nullptr )
		return;

	free_sample( FetchCachedSoundSlotByPath( music->path ) );
	music = nullptr;
}

void Audio_CacheMusic( const char *path )
{
	/* free up anything we cached already */
	ape_audio_destroy_music();

	music = ape_audio_sample_cache_( path );
}
