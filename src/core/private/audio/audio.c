// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"

#include "audio.h"

// Provided as a list so we can hand this to the tools later...
const ApeAudioEffectType APE_AUDIO_EFFECT_TYPES[] = {
        {"none",            APE_AUDIO_REVERB_PRESET_NONE           },
        {"forest",          APE_AUDIO_REVERB_PRESET_FOREST         },
        {"default",         APE_AUDIO_REVERB_PRESET_DEFAULT        },
        {"generic",         APE_AUDIO_REVERB_PRESET_GENERIC        },
        {"paddedcell",      APE_AUDIO_REVERB_PRESET_PADDEDCELL     },
        {"room",            APE_AUDIO_REVERB_PRESET_ROOM           },
        {"bathroom",        APE_AUDIO_REVERB_PRESET_BATHROOM       },
        {"livingroom",      APE_AUDIO_REVERB_PRESET_LIVINGROOM     },
        {"stoneroom",       APE_AUDIO_REVERB_PRESET_STONEROOM      },
        {"auditorium",      APE_AUDIO_REVERB_PRESET_AUDITORIUM     },
        {"concerthall",     APE_AUDIO_REVERB_PRESET_CONCERTHALL    },
        {"cave",            APE_AUDIO_REVERB_PRESET_CAVE           },
        {"arena",           APE_AUDIO_REVERB_PRESET_ARENA          },
        {"hangar",          APE_AUDIO_REVERB_PRESET_HANGAR         },
        {"carpetedhallway", APE_AUDIO_REVERB_PRESET_CARPETEDHALLWAY},
        {"hallway",         APE_AUDIO_REVERB_PRESET_HALLWAY        },
        {"stonecorridor",   APE_AUDIO_REVERB_PRESET_STONECORRIDOR  },
        {"alley",           APE_AUDIO_REVERB_PRESET_ALLEY          },
        {"city",            APE_AUDIO_REVERB_PRESET_CITY           },
        {"mountains",       APE_AUDIO_REVERB_PRESET_MOUNTAINS      },
        {"quarry",          APE_AUDIO_REVERB_PRESET_QUARRY         },
        {"plain",           APE_AUDIO_REVERB_PRESET_PLAIN          },
        {"parkinglot",      APE_AUDIO_REVERB_PRESET_PARKINGLOT     },
        {"sewerpipe",       APE_AUDIO_REVERB_PRESET_SEWERPIPE      },
        {"underwater",      APE_AUDIO_REVERB_PRESET_UNDERWATER     },
        {"smallroom",       APE_AUDIO_REVERB_PRESET_SMALLROOM      },
        {"mediumroom",      APE_AUDIO_REVERB_PRESET_MEDIUMROOM     },
        {"largeroom",       APE_AUDIO_REVERB_PRESET_LARGEROOM      },
        {"mediumhall",      APE_AUDIO_REVERB_PRESET_MEDIUMHALL     },
        {"largehall",       APE_AUDIO_REVERB_PRESET_LARGEHALL      },
        {"plate",           APE_AUDIO_REVERB_PRESET_PLATE          },
};
const unsigned int APE_NUM_AUDIO_EFFECT_TYPES = PL_ARRAY_ELEMENTS( APE_AUDIO_EFFECT_TYPES );

static const ApeAudioDriverInterface *audioDriverInterface = nullptr;
#define DRIVER_CALLBACK( FUNCTION, ... )                                                                                        \
	{                                                                                                                           \
		if ( audioDriverInterface != nullptr && audioDriverInterface->FUNCTION ) audioDriverInterface->FUNCTION( __VA_ARGS__ ); \
	}

static bool  audioInitialized = false;
static bool  audioPaused      = false;
static float audioVolume      = 1.0f;

float ape_audio_get_global_volume_( void )
{
	return audioVolume;
}

static struct
{
	PLVector3 position;
	PLVector3 angles;
	PLVector3 velocity;
} audioListener;

static void play_audio_command( unsigned int argc, char **argv )
{
	const char     *path   = ( argc > 1 ) ? argv[ 1 ] : "sounds/testing/ping.wav";
	ApeAudioSample *sample = ape_audio_sample_cache( path );
	if ( sample == nullptr )
	{
		return;
	}

	ape_audio_sample_emit( sample, nullptr, 100, 1.0 );
	ape_audio_sample_release( sample );
}

static void pause_audio_command( unsigned int argc, char **argv )
{
	ape_audio_pause_( !audioPaused );
}

void ape_audio_initialize_( void )
{
	return;

	if ( audioInitialized )
	{
		return;
	}

	ape_print_( "Initializing audio\n" );

	//todo: make these selectable
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

	PlRegisterConsoleCommand( "audio_play", "Play a specific sound. If no sound is specified, plays a test sound.", -1, play_audio_command );
	PlRegisterConsoleCommand( "audio_pause", "Pause all audio.", 0, pause_audio_command );

	// reset listener
	ape_audio_clear_listener();

	audioInitialized = true;
}

void ape_audio_register_console_variables_( void )
{
	PlRegisterConsoleVariable( "audio.volume", "Set the global audio volume.", "1.0", PL_VAR_F32, &audioVolume, nullptr, true );
}

static void destroy_sample( void *user )
{
	ApeAudioSample *sample = ( ApeAudioSample * ) user;
	assert( sample != nullptr );

	DRIVER_CALLBACK( freeSample, sample );

	PL_DELETE( sample->buffer );
	PL_DELETE( sample );
}

void ape_audio_sample_release( ApeAudioSample *audioSample )
{
	ape_memory_release( &audioSample->reference );
}

ApeAudioSample *ape_audio_format_vorbis_load_( PLFile *file );
ApeAudioSample *ape_audio_format_wav_load_( PLFile *file );
ApeAudioSample *ape_audio_sample_cache( const char *path )
{
	ApeAudioSample *sample = ape_memory_get_from_pool_( path, APE_CACHE_POOL_SAMPLES );
	if ( sample != nullptr )
	{
		ape_memory_add_reference( &sample->reference );
		return sample;
	}

	const char *extension = PlGetFileExtension( path );
	if ( extension == nullptr )
	{
		ape_warning_( "Failed to get audio file extension (%s)!\n", path );
		return nullptr;
	}

	PLFile *file = PlOpenFile( path, false );
	if ( file == nullptr )
	{
		ape_warning_( "Failed to open audio file (%s): %s\n", path, PlGetError() );
		return nullptr;
	}

	if ( pl_strcasecmp( extension, "wav" ) == 0 )
	{
		sample = ape_audio_format_wav_load_( file );
	}
	else if ( pl_strcasecmp( extension, "ogg" ) == 0 )
	{
		sample = ape_audio_format_vorbis_load_( file );
	}

	PlCloseFile( file );

	if ( sample == nullptr )
	{
		// this comes over a little shit given we're just checking the extension, but meh...
		ape_warning_( "Unknown audio format (%s)!\n", path );
		return nullptr;
	}

	if ( audioDriverInterface != nullptr && audioDriverInterface->cacheSample != nullptr )
	{
		if ( !audioDriverInterface->cacheSample( sample ) )
		{
			ape_warning_( "Driver upload for audio sample failed!\n" );
			destroy_sample( sample );
			return nullptr;
		}
	}

	//todo: there's an issue with this at the moment...
	//ape_memory_manager_add_to_pool_( path, APE_CACHE_POOL_SAMPLES, sample );

	ape_memory_setup_reference( path, APE_CACHE_POOL_SAMPLES, &sample->reference, destroy_sample, sample );
	ape_memory_add_reference( &sample->reference );

	PRINT_DEBUG( "Cached sound, \"%s\"\n", path );

	return sample;
}

void ape_audio_sample_emit( ApeAudioSample *audioSample, const PLVector3 *position, float volume, float pitch )
{
	DRIVER_CALLBACK( emitSample, audioSample, position, volume, pitch );
}

void ape_audio_shutdown_( void )
{
	if ( !audioInitialized )
	{
		return;
	}

	DRIVER_CALLBACK( shutdown );

	audioInitialized = false;
}

void ape_audio_tick_( void )
{
	if ( !audioInitialized )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	DRIVER_CALLBACK( tick );

	COM_PROFILE_FUNCTION_END();
}

void ape_audio_pause_( bool pause )
{
	if ( !audioInitialized || pause == audioPaused )
	{
		return;
	}

	DRIVER_CALLBACK( pause, pause );

	audioPaused = pause;

	PRINT_DEBUG( "Audio %s\n", audioPaused ? "paused" : "unpaused" );
}

/****************************************
 * Sources
 ****************************************/

ApeAudioSource *ape_audio_source_create( const PLVector3 *position, const PLVector3 *velocity, ApeAudioSourceGroup group )
{
	ApeAudioSource *source = PL_NEW( ApeAudioSource );

	if ( position != nullptr )
	{
		source->position = *position;
	}
	if ( velocity != nullptr )
	{
		source->velocity = *velocity;
	}

	DRIVER_CALLBACK( createSource, source );

	return source;
}

void ape_audio_source_destroy( ApeAudioSource *audioSource )
{
	if ( audioSource == nullptr )
	{
		return;
	}

	DRIVER_CALLBACK( destroySource, audioSource );

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
