// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "qmos/public/qm_os_random.h"

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
const unsigned int APE_NUM_AUDIO_EFFECT_TYPES = QM_OS_ARRAY_ELEMENTS( APE_AUDIO_EFFECT_TYPES );

#if defined( APE_SUPPORT_OPENAL )
extern ApeAudioDriverInterface ape_audioDriverOpenAL_;
#endif

static const ApeAudioDriverInterface *audioDriverInterfaces[] = {
        nullptr,
#if defined( APE_SUPPORT_OPENAL )
        &ape_audioDriverOpenAL_,
#endif
};
static constexpr unsigned int numAudioDriverInterfaces = QM_OS_ARRAY_ELEMENTS( audioDriverInterfaces );

static ApeConsoleVarString            audioDriverInterfaceName;
static const ApeAudioDriverInterface *audioDriverInterface;
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
	QmMathVector3f position;
	QmMathVector3f angles;
	QmMathVector3f velocity;
} audioListener;

APE_MEMORY_IMPLEMENT_INTERFACE( ape_audio_sample, ApeAudioSample, reference )

static void play_audio_command( unsigned int argc, const char *const *argv )
{
	const char     *path   = ( argc > 1 ) ? argv[ 1 ] : "sounds/testing/ping.wav";
	ApeAudioSample *sample = ape_audio_sample_cache( path );
	if ( sample == nullptr )
	{
		return;
	}

	ape_audio_sample_emit( sample, nullptr, 100, 1.0 );
	ape_audio_sample_release_reference( sample );
}

static void pause_audio_command( unsigned int argc, const char *const *argv )
{
	ape_audio_pause_( !audioPaused );
}

static void test_3d_command( unsigned int argc, const char *const *argv )
{
	ApeAudioSample *sample = ape_audio_sample_cache( "sounds/testing/ping.wav" );
	if ( sample == nullptr )
	{
		return;
	}

	unsigned int   seed     = qm_os_random_seed_initialize();
	QmMathVector3f position = {
	        .x = qm_os_random_float( &seed, 1024.0f ) - qm_os_random_float( &seed, 1024.0f ),
	        .y = qm_os_random_float( &seed, 1024.0f ) - qm_os_random_float( &seed, 1024.0f ),
	        .z = qm_os_random_float( &seed, 1024.0f ) - qm_os_random_float( &seed, 1024.0f ),
	};

	ape_audio_sample_emit( sample, &position, 100, qm_os_random_float( &seed, 2.0f ) );
	ape_audio_sample_release_reference( sample );
}

static void audio_driver_set_interface( const char *name )
{
	if ( audioDriverInterface != nullptr && strcmp( audioDriverInterface->name, name ) == 0 )
	{
		return;
	}

	const ApeAudioDriverInterface *newInterface = nullptr;
	for ( unsigned int i = 0; i < numAudioDriverInterfaces; ++i )
	{
		if ( audioDriverInterfaces[ i ] == nullptr )
		{
			continue;
		}

		if ( strcmp( name, audioDriverInterfaces[ i ]->name ) != 0 )
		{
			continue;
		}

		newInterface = audioDriverInterfaces[ i ];
		break;
	}

	if ( newInterface == nullptr )
	{
		ape_console_warning_( "Failed to find specified audio driver (%s)!\n", name );
		return;
	}

	if ( audioDriverInterface != nullptr )
	{
		audioDriverInterface->shutdown();
	}

	audioDriverInterface = newInterface;
}

static void audio_driver_callback( ApeConsoleVar *self )
{
	audio_driver_set_interface( self->s_value );
}

void ape_audio_initialize_( void )
{
	if ( PlHasCommandLineArgument( "/nosound" ) || audioInitialized )
	{
		return;
	}

	ape_console_print_( "Initializing audio\n" );

	// initialise the driver interface (TODO: allow us to pick the backend we want)

	ape_console_var_register( "audio.driver", "Audio driver to use.", "openal", PL_VAR_STRING, audioDriverInterfaceName, audio_driver_callback, APE_CONSOLE_VAR_FLAG_ARCHIVE );

	audio_driver_set_interface( audioDriverInterfaceName );
	if ( audioDriverInterface == nullptr || !audioDriverInterface->initialize() )
	{
		ape_console_warning_( "Failed to initialize audio driver!\n" );
		return;
	}

	ape_console_cmd_register( "audio_play", "Play a specific sound. If no sound is specified, plays a test sound.", -1, play_audio_command );
	ape_console_cmd_register( "audio_pause", "Pause all audio.", 0, pause_audio_command );
	ape_console_cmd_register( "audio_test_3d", "Test a 3D audio source.", 0, test_3d_command );

	// reset listener
	ape_audio_clear_listener();

	audioInitialized = true;
}

void ape_audio_register_console_variables_( void )
{
	ape_console_var_register( "audio.volume", "Set the global audio volume.", "1.0", PL_VAR_F32, &audioVolume, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
}

static void destroy_sample( void *user )
{
	ApeAudioSample *sample = user;
	assert( sample != nullptr );

	DRIVER_CALLBACK( freeSample, sample );

	qm_os_memory_free( sample->buffer );
	qm_os_memory_free( sample );
}

ApeAudioSample *ape_audio_format_vorbis_load_( QmFsFile *file );
ApeAudioSample *ape_audio_format_wav_load_( QmFsFile *file );
ApeAudioSample *ape_audio_sample_cache( const char *path )
{
	if ( !audioInitialized )
	{
		return nullptr;
	}

	ApeAudioSample *sample = ape_memory_get_from_pool_( path, APE_CACHE_POOL_SAMPLES );
	if ( sample != nullptr )
	{
		ape_memory_reference_add( &sample->reference );
		return sample;
	}

	const char *extension = PlGetFileExtension( path );
	if ( extension == nullptr )
	{
		ape_console_warning_( "Failed to get audio file extension (%s)!\n", path );
		return nullptr;
	}

	QmFsFile *file = qm_fs_file_open( path, false );
	if ( file == nullptr )
	{
		ape_console_warning_( "Failed to open audio file (%s): %s\n", path, PlGetError() );
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
		ape_console_warning_( "Unknown audio format (%s)!\n", path );
		return nullptr;
	}

	if ( audioDriverInterface != nullptr && audioDriverInterface->cacheSample != nullptr )
	{
		if ( !audioDriverInterface->cacheSample( sample ) )
		{
			ape_console_warning_( "Driver upload for audio sample failed!\n" );
			destroy_sample( sample );
			return nullptr;
		}
	}

	//todo: there's an issue with this at the moment...
	//ape_memory_manager_add_to_pool_( path, APE_CACHE_POOL_SAMPLES, sample );

	ape_memory_setup_reference( path, APE_CACHE_POOL_SAMPLES, &sample->reference, destroy_sample, sample );
	ape_memory_reference_add( &sample->reference );

	ape_console_verbose_( "Cached sound, \"%s\"\n", path );

	return sample;
}

ApeAudioSample *ape_audio_sample_create_from_memory( const void *buffer, unsigned int bufferSize, ApeAudioSampleFormat format, unsigned int channels, unsigned int sampleRate )
{
	ApeAudioSample *sample = QM_OS_MEMORY_NEW( ApeAudioSample );
	sample->buffer         = QM_OS_MEMORY_NEW_( uint8_t, bufferSize );
	sample->bufferSize     = bufferSize;
	sample->channels       = channels;
	sample->type           = format;
	sample->sampleRate     = sampleRate;

	memcpy( sample->buffer, buffer, bufferSize );

	if ( audioDriverInterface != nullptr && audioDriverInterface->cacheSample != nullptr )
	{
		if ( !audioDriverInterface->cacheSample( sample ) )
		{
			ape_console_warning_( "Driver upload for audio sample failed!\n" );
			destroy_sample( sample );
			return nullptr;
		}
	}

	ape_memory_setup_reference( "sound_proc", APE_CACHE_POOL_SAMPLES, &sample->reference, destroy_sample, sample );
	ape_memory_reference_add( &sample->reference );

	return sample;
}

void ape_audio_sample_emit( ApeAudioSample *audioSample, const QmMathVector3f *position, float volume, float pitch )
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

	ape_console_verbose_( "Audio %s\n", audioPaused ? "paused" : "unpaused" );
}

/****************************************
 * Sources
 ****************************************/

ApeAudioSource *ape_audio_source_create( const QmMathVector3f *position, const QmMathVector3f *velocity, ApeAudioSourceGroup group )
{
	ApeAudioSource *source = QM_OS_MEMORY_NEW( ApeAudioSource );

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

void ape_audio_source_destroy( ApeAudioSource *self )
{
	if ( self == nullptr )
	{
		return;
	}

	DRIVER_CALLBACK( destroySource, self );

	qm_os_memory_free( self );
}

bool ape_audio_source_is_playing( const ApeAudioSource *self )
{
	if ( audioDriverInterface == nullptr )
	{
		return false;
	}

	assert( audioDriverInterface->isSourcePlaying != nullptr );
	return audioDriverInterface->isSourcePlaying( self );
}

void ape_audio_source_set_position( ApeAudioSource *self, const QmMathVector3f *position )
{
	DRIVER_CALLBACK( setSourcePosition, self, position );
}

void ape_audio_source_set_velocity( ApeAudioSource *self, const QmMathVector3f *velocity )
{
	DRIVER_CALLBACK( setSourceVelocity, self, velocity );
}

void ape_audio_source_set_pitch( ApeAudioSource *self, float pitch )
{
	DRIVER_CALLBACK( setSourcePitch, self, pitch );
}

void ape_audio_source_set_volume( ApeAudioSource *self, float volume )
{
	DRIVER_CALLBACK( setSourceVolume, self, volume );
}

void ape_audio_source_set_loop( ApeAudioSource *self, bool loop )
{
	DRIVER_CALLBACK( setSourceLoop, self, loop );
}

void ape_audio_source_emit( ApeAudioSource *self, ApeAudioSample *audioSample )
{
	DRIVER_CALLBACK( emitSource, self, audioSample );
}

/****************************************
 * Listener
 ****************************************/

void ape_audio_update_listener( const QmMathVector3f *position, const QmMathVector3f *angles, const QmMathVector3f *velocity )
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
	QM_OS_ZERO_( audioListener );
}

QmMathVector3f ape_audio_get_listener_position( void )
{
	return audioListener.position;
}

QmMathVector3f ape_audio_get_listener_angles( void )
{
	return audioListener.angles;
}

QmMathVector3f ape_audio_get_listener_velocity( void )
{
	return audioListener.velocity;
}
