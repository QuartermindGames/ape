// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/efx-presets.h>

#include "audio.h"

/**
 * OpenAL driver for Yin
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

static ALCdevice  *xalDevice = NULL;
static ALCcontext *xalContext = NULL;

enum
{
	XAL_EXTENSION_EFX,
	XAL_EXTENSION_SOFT_BUFFER_SAMPLES,

	XAL_MAX_EXTENSIONS
};
static bool   xalExtensions[ XAL_MAX_EXTENSIONS ];
static ALuint xalReverbEffectSlot = 0;
static ALuint xalReverbSoundSlot = 0;

static LPALGENEFFECTS    alGenEffects;
static LPALDELETEEFFECTS alDeleteEffects;
static LPALISEFFECT      alIsEffect;
static LPALEFFECTI       alEffecti;
static LPALEFFECTF       alEffectf;

static LPALGENAUXILIARYEFFECTSLOTS    alGenAuxiliaryEffectSlots;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
static LPALISAUXILIARYEFFECTSLOT      alIsAuxiliaryEffectSlot;
static LPALAUXILIARYEFFECTSLOTI       alAuxiliaryEffectSloti;

static void Audio_OpenAL_Shutdown( void );

static bool Initialize( void )
{
	return false;
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
		Audio_OpenAL_Shutdown();
		return false;
	}

	bool status;
	status = alcMakeContextCurrent( xalContext );
	if ( !status )
	{
		PRINT_WARNING( "Failed to make OpenAL context current!\n" );
		Audio_OpenAL_Shutdown();
		return false;
	}

	PL_ZERO( xalExtensions, sizeof( bool ) * XAL_MAX_EXTENSIONS );
	XAL_CALL( status = alcIsExtensionPresent( xalDevice, "ALC_EXT_EFX" ) );
	if ( status )
	{
		PRINT( "ALC_EXT_EFX detected\n" );

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
		PRINT( "AL_SOFT_buffer_samples detected\n" );
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

static void Audio_OpenAL_Shutdown( void )
{
	PRINT( "Shutting down OpenAL interface\n" );

	alcDestroyContext( xalContext );
	xalContext = NULL;

	alcCloseDevice( xalDevice );
	xalDevice = NULL;
}

static void Audio_OpenAL_Tick( void )
{
	PLVector3 position = Audio_GetListenerPosition();
	XAL_CALL( alListenerfv( AL_POSITION, ( ALfloat * ) &position ) );

	PLVector3 angles = Audio_GetListenerAngles();
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

	PLVector3 velocity = Audio_GetListenerVelocity();
	XAL_CALL( alListenerfv( AL_VELOCITY, ( ALfloat * ) &velocity ) );

	XAL_CALL( alListenerf( AL_GAIN, Audio_GetGlobalVolume() ) );
}

static void Pause( bool pause )
{
}

static bool CacheSample( YNCoreAudioSample *sample )
{
	return true;
}

static void FreeSample( YNCoreAudioSample *sample )
{
}

static void EmitSample( YNCoreAudioSample *sample, int8_t volume )
{
}

static bool CreateSource( YNCoreAudioSource *source )
{
	source->user = PL_NEW( XALAudioSource );
	XAL_CALL( alGenSources( 1, &( ( XALAudioSource * ) source->user )->id ) );
	return true;
}

static void DestroySource( YNCoreAudioSource *source )
{
	if ( source->user == NULL )
		return;

	XAL_CALL( alSourcei( ( ( XALAudioSource * ) source->user )->id, AL_LOOPING, AL_FALSE ) );
	XAL_CALL( alSourcei( ( ( XALAudioSource * ) source->user )->id, AL_BUFFER, 0 ) );
	XAL_CALL( alDeleteSources( 1, &( ( XALAudioSource * ) source->user )->id ) );
	source->user = NULL;
}

const YNCoreAudioDriverInterface *YnCore_Audio_OpenAL_GetDriverInterface( void )
{
	static YNCoreAudioDriverInterface driverInterface;
	PL_ZERO_( driverInterface );

	driverInterface.Initialize = Initialize;
	driverInterface.Shutdown = Audio_OpenAL_Shutdown;
	driverInterface.Tick = Audio_OpenAL_Tick;
	driverInterface.Pause = Pause;
	driverInterface.CacheSample = CacheSample;
	driverInterface.FreeSample = FreeSample;
	driverInterface.EmitSample = EmitSample;
	driverInterface.CreateSource = CreateSource;
	driverInterface.DestroySource = DestroySource;

	return &driverInterface;
}
