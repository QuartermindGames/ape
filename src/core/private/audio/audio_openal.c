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
#	define XAL_CALL( X )                     \
		{                                     \
			alGetError();                     \
			X;                                \
			unsigned int _err = alGetError(); \
			assert( _err == AL_NO_ERROR );    \
		}
#else
#	define XAL_CALL( X ) X
#endif

typedef struct XALAudioSample
{
	ALuint id;
} XALAudioSample;

typedef struct XALAudioSource
{
	ALuint id;
} XALAudioSource;

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

static void shutdown_openal( void );

static bool initialize_openal( void )
{
	xalDevice = alcOpenDevice( NULL );
	if ( xalDevice == NULL )
	{
		PRINT_WARNING( "Failed to open default OpenAL device!\n" );
		return false;
	}

	xalContext = alcCreateContext( xalDevice, NULL );
	if ( xalContext == NULL )
	{
		PRINT_WARNING( "Failed to create OpenAL context!\n" );
		shutdown_openal();
		return false;
	}

	bool status;
	status = alcMakeContextCurrent( xalContext );
	if ( !status )
	{
		PRINT_WARNING( "Failed to make OpenAL context current!\n" );
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
	xalContext = NULL;

	alcCloseDevice( xalDevice );
	xalDevice = NULL;
}

static void Audio_OpenAL_Tick( void )
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
}

static void Pause( bool pause )
{
}

static bool cache_sample( ApeAudioSample *sample )
{
	return true;
}

static void FreeSample( ApeAudioSample *sample )
{
}

static void emit_sample( ApeAudioSample *sample, int8_t volume )
{
}

static bool create_source( ApeAudioSource *source )
{
	source->user = PL_NEW( XALAudioSource );
	XAL_CALL( alGenSources( 1, &( ( XALAudioSource * ) source->user )->id ) );
	return true;
}

static void destroy_source( ApeAudioSource *source )
{
	if ( source->user == NULL )
		return;

	XAL_CALL( alSourcei( ( ( XALAudioSource * ) source->user )->id, AL_LOOPING, AL_FALSE ) );
	XAL_CALL( alSourcei( ( ( XALAudioSource * ) source->user )->id, AL_BUFFER, 0 ) );
	XAL_CALL( alDeleteSources( 1, &( ( XALAudioSource * ) source->user )->id ) );
	source->user = NULL;
}

const ApeAudioDriverInterface *ape_audio_get_driver_interface_( void )
{
	static ApeAudioDriverInterface driverInterface;
	PL_ZERO_( driverInterface );

	driverInterface.initialize    = initialize_openal;
	driverInterface.shutdown      = shutdown_openal;
	driverInterface.tick          = Audio_OpenAL_Tick;
	driverInterface.pause         = Pause;
	driverInterface.cacheSample   = cache_sample;
	driverInterface.freeSample    = FreeSample;
	driverInterface.emitSample    = emit_sample;
	driverInterface.createSource  = create_source;
	driverInterface.destroySource = destroy_source;

	return &driverInterface;
}
