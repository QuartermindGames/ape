// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: OpenAL driver for Yin

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/efx-presets.h>

#include "audio.h"

/**
 * Cheekily based on the work I'd previously done here...
 * 	https://github.com/TalonBraveInfo/OpenHoW/blob/master/src/engine/audio/AudioManager.cpp
 */

#if !defined( NDEBUG )

static void handle_al_error( unsigned int err, const char *file, int line )
{
	if ( err == AL_NO_ERROR )
	{
		return;
	}

	const char *desc;
	switch ( err )
	{
		default:
			desc = "UNKNOWN";
			break;
		case AL_INVALID_NAME:
			desc = "INVALID NAME";
			break;
		case AL_INVALID_ENUM:
			desc = "INVALID ENUM";
			break;
		case AL_INVALID_VALUE:
			desc = "INVALID VALUE";
			break;
		case AL_INVALID_OPERATION:
			desc = "INVALID OPERATION";
			break;
		case AL_OUT_OF_MEMORY:
			desc = "OUT OF MEMORY";
			break;
	}

	ape_warning_( "Encountered an OpenAL error: %s (%u) (%s:%u)\n", desc, err, file, line );

	assert( err == AL_NO_ERROR );
}

#	define XAL_CALL( X )                                \
		{                                                \
			alGetError();                                \
			X;                                           \
			unsigned int _err = alGetError();            \
			handle_al_error( _err, __FILE__, __LINE__ ); \
		}
#else
#	define XAL_CALL( X ) X
#endif

static ALCdevice  *xalDevice;
static ALCcontext *xalContext;

enum
{
	XAL_EXTENSION_EFX,
	XAL_EXTENSION_SOFT_BUFFER_SAMPLES,

	XAL_MAX_EXTENSIONS
};
static bool   xalExtensions[ XAL_MAX_EXTENSIONS ];
static ALuint xalReverbEffectSlot;
static ALuint xalReverbSoundSlot;

static LPALGENEFFECTS    alGenEffects;
static LPALDELETEEFFECTS alDeleteEffects;
static LPALISEFFECT      alIsEffect;
static LPALEFFECTI       alEffecti;
static LPALEFFECTF       alEffectf;

static LPALGENAUXILIARYEFFECTSLOTS    alGenAuxiliaryEffectSlots;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
static LPALISAUXILIARYEFFECTSLOT      alIsAuxiliaryEffectSlot;
static LPALAUXILIARYEFFECTSLOTI       alAuxiliaryEffectSloti;

typedef struct TemporarySource
{
	ALuint          user;
	ApeAudioSample *sample;
} TemporarySource;

static constexpr unsigned int MAX_TEMPORARY_SOURCES = 4096;
static TemporarySource        temporarySources[ MAX_TEMPORARY_SOURCES ];

static void shutdown_openal( void );

static bool initialize_openal( void )
{
	xalDevice = alcOpenDevice( nullptr );
	if ( xalDevice == NULL )
	{
		ape_warning_( "Failed to open default OpenAL device!\n" );
		return false;
	}

	xalContext = alcCreateContext( xalDevice, nullptr );
	if ( xalContext == NULL )
	{
		ape_warning_( "Failed to create OpenAL context!\n" );
		shutdown_openal();
		return false;
	}

	bool status;
	status = alcMakeContextCurrent( xalContext );
	if ( !status )
	{
		ape_warning_( "Failed to make OpenAL context current!\n" );
		shutdown_openal();
		return false;
	}

	PL_ZERO( xalExtensions, sizeof( bool ) * XAL_MAX_EXTENSIONS );
	XAL_CALL( status = alcIsExtensionPresent( xalDevice, "ALC_EXT_EFX" ) );
	if ( status )
	{
		ape_print_( "ALC_EXT_EFX detected\n" );

		XAL_CALL( alGenEffects = alGetProcAddress( "alGenEffects" ) );
		XAL_CALL( alDeleteEffects = alGetProcAddress( "alDeleteEffects" ) );
		XAL_CALL( alIsEffect = alGetProcAddress( "alIsEffect" ) );
		XAL_CALL( alEffecti = alGetProcAddress( "alEffecti" ) );
		XAL_CALL( alEffectf = alGetProcAddress( "alEffectf" ) );

		XAL_CALL( alGenAuxiliaryEffectSlots = alGetProcAddress( "alGenAuxiliaryEffectSlots" ) );
		XAL_CALL( alDeleteAuxiliaryEffectSlots = alGetProcAddress( "alDeleteAuxiliaryEffectSlots" ) );
		XAL_CALL( alIsAuxiliaryEffectSlot = alGetProcAddress( "alIsAuxiliaryEffectSlot" ) );
		XAL_CALL( alAuxiliaryEffectSloti = alGetProcAddress( "alAuxiliaryEffectSloti" ) );

		xalExtensions[ XAL_EXTENSION_EFX ] = true;
	}

	XAL_CALL( status = alIsExtensionPresent( "AL_SOFT_buffer_samples" ) );
	if ( status )
	{
		ape_print_( "AL_SOFT_buffer_samples detected\n" );
		xalExtensions[ XAL_EXTENSION_SOFT_BUFFER_SAMPLES ] = true;
	}

	XAL_CALL( alDopplerFactor( 4.0f ) );
	XAL_CALL( alDopplerVelocity( 350.0f ) );

	if ( xalExtensions[ XAL_EXTENSION_EFX ] )
	{
		XAL_CALL( alGenEffects( 1, &xalReverbEffectSlot ) );
		XAL_CALL( alEffecti( xalReverbEffectSlot, AL_EFFECT_TYPE, AL_EFFECT_REVERB ) );
		const EFXEAXREVERBPROPERTIES reverb = EFX_REVERB_PRESET_OUTDOORS_DEEPCANYON;
		// EFX_REVERB_PRESET_OUTDOORS_DEEPCANYON
		// EFX_REVERB_PRESET_OUTDOORS_VALLEY
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_DENSITY, reverb.flDensity ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_DIFFUSION, reverb.flDiffusion ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_GAIN, reverb.flGain ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_GAINHF, reverb.flGainHF ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_DECAY_TIME, reverb.flDecayTime ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_DECAY_HFRATIO, reverb.flDecayHFRatio ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_REFLECTIONS_GAIN, reverb.flReflectionsGain ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_REFLECTIONS_DELAY, reverb.flReflectionsDelay ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_LATE_REVERB_GAIN, reverb.flLateReverbGain ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_LATE_REVERB_DELAY, reverb.flLateReverbDelay ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_AIR_ABSORPTION_GAINHF, reverb.flAirAbsorptionGainHF ) );
		XAL_CALL( alEffectf( xalReverbEffectSlot, AL_REVERB_ROOM_ROLLOFF_FACTOR, reverb.flRoomRolloffFactor ) );
		XAL_CALL( alEffecti( xalReverbEffectSlot, AL_REVERB_DECAY_HFLIMIT, reverb.iDecayHFLimit ) );
		XAL_CALL( alGenAuxiliaryEffectSlots( 1, &xalReverbSoundSlot ) );
		XAL_CALL( alAuxiliaryEffectSloti( xalReverbSoundSlot, AL_EFFECTSLOT_EFFECT, ( ALint ) xalReverbEffectSlot ) );
	}
	XAL_CALL( alDistanceModel( AL_EXPONENT_DISTANCE ) );

	return true;
}

static void shutdown_openal( void )
{
	ape_print_( "Shutting down OpenAL interface\n" );

	alcDestroyContext( xalContext );
	xalContext = nullptr;

	alcCloseDevice( xalDevice );
	xalDevice = nullptr;
}

static TemporarySource *get_free_temporary_source()
{
	for ( unsigned int i = 0; i < MAX_TEMPORARY_SOURCES; ++i )
	{
		if ( temporarySources[ i ].user == 0 )
		{
			XAL_CALL( alGenSources( 1, &temporarySources[ i ].user ) );
			return &temporarySources[ i ];
		}

		ALint state;
		alGetSourcei( temporarySources[ i ].user, AL_SOURCE_STATE, &state );
		if ( state != AL_PLAYING )
		{
			return &temporarySources[ i ];
		}
	}

	return nullptr;
}

static void al_tick( void )
{
	PLVector3 position = ape_audio_get_listener_position();
	XAL_CALL( alListenerfv( AL_POSITION, ( ALfloat * ) &position ) );

	PLVector3 angles = ape_audio_get_listener_angles();
	PLVector3 left, up, forward;
	PlAnglesAxes( angles, &left, &up, &forward );

	float orientation[ 6 ];
	orientation[ 0 ] = forward.z;
	orientation[ 1 ] = forward.y;
	orientation[ 2 ] = forward.z;
	orientation[ 3 ] = up.x;
	orientation[ 4 ] = up.y;
	orientation[ 5 ] = up.z;
	XAL_CALL( alListenerfv( AL_ORIENTATION, orientation ) );

	PLVector3 velocity = ape_audio_get_listener_velocity();
	XAL_CALL( alListenerfv( AL_VELOCITY, ( ALfloat * ) &velocity ) );

	XAL_CALL( alListenerf( AL_GAIN, ape_audio_get_global_volume_() ) );

	for ( unsigned int i = 0; i < MAX_TEMPORARY_SOURCES; ++i )
	{
		if ( temporarySources[ i ].user == 0 )
		{
			continue;
		}

		ALint state;
		XAL_CALL( alGetSourcei( temporarySources[ i ].user, AL_SOURCE_STATE, &state ) );
		if ( state != AL_PLAYING )
		{
			XAL_CALL( alSourcei( temporarySources[ i ].user, AL_BUFFER, 0 ) );
			if ( temporarySources[ i ].sample != nullptr )
			{
				ape_audio_sample_release( temporarySources[ i ].sample );
				temporarySources[ i ].sample = nullptr;
			}
		}
	}
}

static void al_pause( bool pause )
{
}

static bool al_cache_sample( ApeAudioSample *sample )
{
	ALenum format;
	assert( sample->type != APE_AUDIO_SAMPLE_FORMAT_INVALID );
	switch ( sample->type )
	{
		default:
			ape_warning_( "Invalid or unsupported sample type (%u) for OpenAL!\n", sample->type );
			return false;
		case APE_AUDIO_SAMPLE_FORMAT_MONO8:
			format = AL_FORMAT_MONO8;
			break;
		case APE_AUDIO_SAMPLE_FORMAT_STEREO8:
			format = AL_FORMAT_STEREO8;
			break;
		case APE_AUDIO_SAMPLE_FORMAT_MONO16:
			format = AL_FORMAT_MONO16;
			break;
		case APE_AUDIO_SAMPLE_FORMAT_STEREO16:
			format = AL_FORMAT_STEREO16;
			break;
	}

	XAL_CALL( alGenBuffers( 1, &sample->user ) );
	XAL_CALL( alBufferData( sample->user, format, sample->buffer, ( ALsizei ) sample->bufferSize, ( ALsizei ) sample->sampleRate ) );

	return true;
}

static void al_free_sample( ApeAudioSample *sample )
{
	XAL_CALL( alDeleteBuffers( 1, &sample->user ) );
}

static void al_emit_sample( ApeAudioSample *sample, const PLVector3 *position, float volume )
{
	TemporarySource *source = get_free_temporary_source();
	if ( source == nullptr )
	{
		ape_warning_( "Failed to get a free audio source!\n" );
		return;
	}

	if ( position == nullptr )
	{
		position = &pl_vecOrigin3;
	}

	XAL_CALL( alSourcei( source->user, AL_BUFFER, sample->user ) );
	XAL_CALL( alSourcef( source->user, AL_GAIN, volume ) );
	XAL_CALL( alSourcef( source->user, AL_PITCH, PlGenerateRandomFloat( 2.0f ) ) );
	XAL_CALL( alSource3f( source->user, AL_POSITION, position->x, position->y, position->z ) );
	XAL_CALL( alSourcePlay( source->user ) );

	ape_memory_add_reference( &sample->reference );

	source->sample = sample;
}

static bool al_create_source( ApeAudioSource *source )
{
	XAL_CALL( alGenSources( 1, &source->user ) );
	return true;
}

static void al_destroy_source( ApeAudioSource *source )
{
	if ( source->sample != nullptr )
	{
		XAL_CALL( alSourceStop( source->user ) );
		XAL_CALL( alSourcei( source->user, AL_BUFFER, 0 ) );
		ape_audio_sample_release( source->sample );
	}

	XAL_CALL( alDeleteSources( 1, &source->user ) );
}

const ApeAudioDriverInterface *ape_audio_get_driver_interface_( void )
{
	static ApeAudioDriverInterface driverInterface;

	driverInterface.initialize    = initialize_openal;
	driverInterface.shutdown      = shutdown_openal;
	driverInterface.tick          = al_tick;
	driverInterface.pause         = al_pause;
	driverInterface.cacheSample   = al_cache_sample;
	driverInterface.freeSample    = al_free_sample;
	driverInterface.emitSample    = al_emit_sample;
	driverInterface.createSource  = al_create_source;
	driverInterface.destroySource = al_destroy_source;

	return &driverInterface;
}
