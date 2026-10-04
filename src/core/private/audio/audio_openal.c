// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: OpenAL driver for ApeTech audio system.

#if defined( APE_SUPPORT_OPENAL )

#	include <AL/al.h>
#	include <AL/alc.h>
#	include <AL/efx.h>
#	include <AL/efx-presets.h>

#	include "audio.h"

/**
 * Cheekily based on the work I'd previously done here...
 * 	https://github.com/TalonBraveInfo/OpenHoW/blob/master/src/engine/audio/AudioManager.cpp
 */

static constexpr ALuint XAL_INVALID = ( ALuint ) -1;

#	if !defined( NDEBUG )

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

	ape_console_warning_( "Encountered an OpenAL error: %s (%u) (%s:%u)\n", desc, err, file, line );

	assert( err == AL_NO_ERROR );
}

#		define XAL_CALL( X )                                \
			{                                                \
				alGetError();                                \
				X;                                           \
				unsigned int _err = alGetError();            \
				handle_al_error( _err, __FILE__, __LINE__ ); \
			}
#	else
#		define XAL_CALL( X ) X
#	endif

static ALCdevice  *xalDevice;
static ALCcontext *xalContext;

enum
{
	XAL_EXTENSION_EFX,
	XAL_EXTENSION_SOFT_BUFFER_SAMPLES,

	XAL_MAX_EXTENSIONS
};
static bool xalExtensions[ XAL_MAX_EXTENSIONS ];

static constexpr unsigned int MAX_TEMPORARY_SOURCES = 4096;
static ApeAudioSource         temporarySources[ MAX_TEMPORARY_SOURCES ];
static QmOsLinkedList        *activeSources;

static void shutdown_openal( void );

/////////////////////////////////////////////////////////////////////////////////////
// Effect Management
// Wishlist...
//	-	At some point it might be worth exploring custom reverb setups?
//		We could probably define these through a file that can be overriden.
//	-	Combine this with the light grid, so we compute the ideal audio setup per
//		cell. Probably expensive, but would be really cool for simulating occlusion,
//		and automatically handling reverb based on surrounding surfaces.
/////////////////////////////////////////////////////////////////////////////////////

typedef struct XalEffect
{
	ALuint slot;
	ALuint aux;
} XalEffect;

static XalEffect effectSlots[ APE_AUDIO_REVERB_PRESET_MAX ];

static constexpr EFXEAXREVERBPROPERTIES reverbProperties[ APE_AUDIO_REVERB_PRESET_MAX ] = {
        [APE_AUDIO_REVERB_PRESET_FOREST]          = EFX_REVERB_PRESET_FOREST,
        [APE_AUDIO_REVERB_PRESET_DEFAULT]         = EFX_REVERB_PRESET_GENERIC,
        [APE_AUDIO_REVERB_PRESET_PADDEDCELL]      = EFX_REVERB_PRESET_PADDEDCELL,
        [APE_AUDIO_REVERB_PRESET_ROOM]            = EFX_REVERB_PRESET_ROOM,
        [APE_AUDIO_REVERB_PRESET_BATHROOM]        = EFX_REVERB_PRESET_BATHROOM,
        [APE_AUDIO_REVERB_PRESET_LIVINGROOM]      = EFX_REVERB_PRESET_LIVINGROOM,
        [APE_AUDIO_REVERB_PRESET_STONEROOM]       = EFX_REVERB_PRESET_STONEROOM,
        [APE_AUDIO_REVERB_PRESET_AUDITORIUM]      = EFX_REVERB_PRESET_AUDITORIUM,
        [APE_AUDIO_REVERB_PRESET_CONCERTHALL]     = EFX_REVERB_PRESET_CONCERTHALL,
        [APE_AUDIO_REVERB_PRESET_CAVE]            = EFX_REVERB_PRESET_CAVE,
        [APE_AUDIO_REVERB_PRESET_ARENA]           = EFX_REVERB_PRESET_ARENA,
        [APE_AUDIO_REVERB_PRESET_HANGAR]          = EFX_REVERB_PRESET_HANGAR,
        [APE_AUDIO_REVERB_PRESET_CARPETEDHALLWAY] = EFX_REVERB_PRESET_CARPETEDHALLWAY,
        [APE_AUDIO_REVERB_PRESET_HALLWAY]         = EFX_REVERB_PRESET_HALLWAY,
        [APE_AUDIO_REVERB_PRESET_STONECORRIDOR]   = EFX_REVERB_PRESET_STONECORRIDOR,
        [APE_AUDIO_REVERB_PRESET_ALLEY]           = EFX_REVERB_PRESET_ALLEY,
        [APE_AUDIO_REVERB_PRESET_CITY]            = EFX_REVERB_PRESET_CITY,
        [APE_AUDIO_REVERB_PRESET_MOUNTAINS]       = EFX_REVERB_PRESET_MOUNTAINS,
        [APE_AUDIO_REVERB_PRESET_QUARRY]          = EFX_REVERB_PRESET_QUARRY,
        [APE_AUDIO_REVERB_PRESET_PLAIN]           = EFX_REVERB_PRESET_PLAIN,
        [APE_AUDIO_REVERB_PRESET_PARKINGLOT]      = EFX_REVERB_PRESET_PARKINGLOT,
        [APE_AUDIO_REVERB_PRESET_SEWERPIPE]       = EFX_REVERB_PRESET_SEWERPIPE,
        [APE_AUDIO_REVERB_PRESET_UNDERWATER]      = EFX_REVERB_PRESET_UNDERWATER,
};

static LPALGENEFFECTS    alGenEffects;
static LPALDELETEEFFECTS alDeleteEffects;
static LPALISEFFECT      alIsEffect;
static LPALEFFECTI       alEffecti;
static LPALEFFECTF       alEffectf;

static LPALGENAUXILIARYEFFECTSLOTS    alGenAuxiliaryEffectSlots;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
static LPALISAUXILIARYEFFECTSLOT      alIsAuxiliaryEffectSlot;
static LPALAUXILIARYEFFECTSLOTI       alAuxiliaryEffectSloti;

static const XalEffect *get_effect( ApeAudioReverbPreset const preset )
{
	if ( effectSlots[ preset ].slot != XAL_INVALID )
	{
		return &effectSlots[ preset ];
	}

	XAL_CALL( alGenEffects( 1, &effectSlots[ preset ].slot ) );
	XAL_CALL( alEffecti( effectSlots[ preset ].slot, AL_EFFECT_TYPE, AL_EFFECT_REVERB ) );

	const EFXEAXREVERBPROPERTIES *reverb = &reverbProperties[ preset ];
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_DENSITY, reverb->flDensity ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_DIFFUSION, reverb->flDiffusion ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_GAIN, reverb->flGain ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_GAINHF, reverb->flGainHF ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_DECAY_TIME, reverb->flDecayTime ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_DECAY_HFRATIO, reverb->flDecayHFRatio ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_REFLECTIONS_GAIN, reverb->flReflectionsGain ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_REFLECTIONS_DELAY, reverb->flReflectionsDelay ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_LATE_REVERB_GAIN, reverb->flLateReverbGain ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_LATE_REVERB_DELAY, reverb->flLateReverbDelay ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_AIR_ABSORPTION_GAINHF, reverb->flAirAbsorptionGainHF ) );
	XAL_CALL( alEffectf( effectSlots[ preset ].slot, AL_REVERB_ROOM_ROLLOFF_FACTOR, reverb->flRoomRolloffFactor ) );
	XAL_CALL( alEffecti( effectSlots[ preset ].slot, AL_REVERB_DECAY_HFLIMIT, reverb->iDecayHFLimit ) );

	XAL_CALL( alGenAuxiliaryEffectSlots( 1, &effectSlots[ preset ].aux ) );
	XAL_CALL( alAuxiliaryEffectSloti( effectSlots[ preset ].aux, AL_EFFECTSLOT_EFFECT, effectSlots[ preset ].slot ) );

	return &effectSlots[ preset ];
}

static void clear_effects()
{
	for ( unsigned int i = 0; i < APE_AUDIO_REVERB_PRESET_MAX; ++i )
	{
		// unsurprisingly, al doesn't like us giving him our invalid slots
		if ( effectSlots[ i ].slot == XAL_INVALID )
		{
			continue;
		}

		XAL_CALL( alDeleteAuxiliaryEffectSlots( 1, &effectSlots[ i ].aux ) );
		XAL_CALL( alDeleteEffects( 1, &effectSlots[ i ].slot ) );
	}

	QM_OS_SET_ARRAY( effectSlots, XAL_INVALID, APE_AUDIO_REVERB_PRESET_MAX );
}

static void setup_effects()
{
	bool status;
	XAL_CALL( status = alcIsExtensionPresent( xalDevice, "ALC_EXT_EFX" ) );
	if ( !status )
	{
		ape_console_warning_( "ALC_EXT_EFX is unsupported!\n" );
	}

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

	QM_OS_SET_ARRAY( effectSlots, XAL_INVALID, APE_AUDIO_REVERB_PRESET_MAX );
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

static bool initialize_openal( void )
{
	xalDevice = alcOpenDevice( nullptr );
	if ( xalDevice == NULL )
	{
		ape_console_warning_( "Failed to open default OpenAL device!\n" );
		return false;
	}

	xalContext = alcCreateContext( xalDevice, nullptr );
	if ( xalContext == NULL )
	{
		ape_console_warning_( "Failed to create OpenAL context!\n" );
		shutdown_openal();
		return false;
	}

	bool status;
	XAL_CALL( status = alcMakeContextCurrent( xalContext ) );
	if ( !status )
	{
		ape_console_warning_( "Failed to make OpenAL context current!\n" );
		shutdown_openal();
		return false;
	}

	QM_OS_ZERO( xalExtensions, sizeof( bool ) * XAL_MAX_EXTENSIONS );

	XAL_CALL( status = alIsExtensionPresent( "AL_SOFT_buffer_samples" ) );
	if ( status )
	{
		xalExtensions[ XAL_EXTENSION_SOFT_BUFFER_SAMPLES ] = true;
	}
	else
	{
		ape_console_warning_( "ALC_EXT_EFX is unsupported!\n" );
	}

	setup_effects();

	XAL_CALL( alDopplerFactor( 4.0f ) );
	XAL_CALL( alDopplerVelocity( 350.0f ) );
	XAL_CALL( alDistanceModel( AL_EXPONENT_DISTANCE ) );

	activeSources = qm_os_linked_list_create();
	if ( activeSources == nullptr )
	{
		ape_console_error_( true, "Failed to create active sources list: %s\n", PlGetError() );
	}

	return true;
}

static void shutdown_openal( void )
{
	ape_console_print_( "Shutting down OpenAL interface\n" );

	alcDestroyContext( xalContext );
	xalContext = nullptr;

	alcCloseDevice( xalDevice );
	xalDevice = nullptr;

	qm_os_memory_free( activeSources );
}

static bool xal_source_create( ApeAudioSource *source );

static ApeAudioSource *get_free_temporary_source()
{
	for ( unsigned int i = 0; i < MAX_TEMPORARY_SOURCES; ++i )
	{
		if ( temporarySources[ i ].user == 0 )
		{
			if ( !xal_source_create( &temporarySources[ i ] ) )
			{
				return nullptr;
			}

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

static void xal_tick( void )
{
	QmMathVector3f position = ape_audio_get_listener_position();
	XAL_CALL( alListenerfv( AL_POSITION, ( ALfloat * ) &position ) );

	QmMathVector3f angles = ape_audio_get_listener_angles();
	QmMathVector3f up, forward;
	PlAnglesAxes( angles, nullptr, &up, &forward );
	XAL_CALL( alListenerfv( AL_ORIENTATION, ( float[] ) { forward.x, forward.y, forward.z, up.x, up.y, up.z } ) );

	QmMathVector3f velocity = ape_audio_get_listener_velocity();
	XAL_CALL( alListenerfv( AL_VELOCITY, ( ALfloat * ) &velocity ) );

	XAL_CALL( alListenerf( AL_GAIN, ape_audio_get_global_volume_() ) );

	ApeAudioSource *source;
	QM_OS_LINKED_LIST_ITERATE( source, activeSources, i )
	{
		ALint state;
		XAL_CALL( alGetSourcei( source->user, AL_SOURCE_STATE, &state ) );
		if ( state != AL_PLAYING )
		{
			XAL_CALL( alSourcei( source->user, AL_BUFFER, 0 ) );
			if ( source->sample != nullptr )
			{
				ape_audio_sample_release_reference( source->sample );
				source->sample = nullptr;
			}

			qm_os_memory_free( i );
		}
	}
}

static void xal_pause( bool pause )
{
}

static bool xal_cache_sample( ApeAudioSample *sample )
{
	ALenum format;
	assert( sample->type != APE_AUDIO_SAMPLE_FORMAT_INVALID );
	switch ( sample->type )
	{
		default:
			ape_console_warning_( "Invalid or unsupported sample type (%u) for OpenAL!\n", sample->type );
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

	XAL_CALL( alGenBuffers( 1, ( ALuint * ) &sample->user ) );
	XAL_CALL( alBufferData( sample->user, format, sample->buffer, ( ALsizei ) sample->bufferSize, ( ALsizei ) sample->sampleRate ) );

	return true;
}

static void xal_free_sample( ApeAudioSample *sample )
{
	XAL_CALL( alDeleteBuffers( 1, ( ALuint * ) &sample->user ) );
}

static void xal_emit_sample( ApeAudioSample *sample, const QmMathVector3f *position, float volume, float pitch, ApeAudioReverbPreset reverb )
{
	ApeAudioSource *source = get_free_temporary_source();
	if ( source == nullptr )
	{
		ape_console_warning_( "Failed to get a free audio source!\n" );
		return;
	}

	if ( position == nullptr )
	{
		position = &QM_MATH_VECTOR3F_ZERO;
	}

	XAL_CALL( alSourcei( source->user, AL_BUFFER, sample->user ) );
	XAL_CALL( alSourcef( source->user, AL_GAIN, volume ) );
	XAL_CALL( alSourcef( source->user, AL_PITCH, pitch ) );
	XAL_CALL( alSource3f( source->user, AL_POSITION, position->x, position->y, position->z ) );

	if ( xalExtensions[ XAL_EXTENSION_EFX ] && reverb != APE_AUDIO_REVERB_PRESET_NONE )
	{
		const XalEffect *effect = get_effect( reverb );
		if ( effect != nullptr )
		{
			XAL_CALL( alSource3i( source->user, AL_AUXILIARY_SEND_FILTER, effect->aux, 0, AL_FILTER_NULL ) );
		}
	}

	XAL_CALL( alSourcePlay( source->user ) );

	ape_memory_reference_add( &sample->reference );

	source->sample = sample;
	qm_os_linked_list_push_back( activeSources, source );
}

/////////////////////////////////////////////////////////////////////////////////////
// Sources

static bool xal_source_create( ApeAudioSource *source )
{
	XAL_CALL( alGenSources( 1, ( ALuint * ) &source->user ) );

	// set some reasonable (hopefully) defaults
	XAL_CALL( alSourcef( source->user, AL_GAIN, 100.0f ) );
	XAL_CALL( alSourcef( source->user, AL_PITCH, 1.0f ) );

	return true;
}

static void xal_source_destroy( ApeAudioSource *source )
{
	if ( source->sample != nullptr )
	{
		XAL_CALL( alSourceStop( source->user ) );
		XAL_CALL( alSourceUnqueueBuffers( source->user, 1, ( ALuint * ) &source->sample->user ) );
		XAL_CALL( alSourcei( source->user, AL_BUFFER, 0 ) );
		ape_audio_sample_release_reference( source->sample );
	}

	XAL_CALL( alDeleteSources( 1, ( ALuint * ) &source->user ) );
}

static void xal_source_set_position( ApeAudioSource *self, const QmMathVector3f *position )
{
	XAL_CALL( alSource3f( self->user, AL_POSITION, position->x, position->y, position->z ) );
}

static void xal_source_set_velocity( ApeAudioSource *self, const QmMathVector3f *velocity )
{
	XAL_CALL( alSource3f( self->user, AL_VELOCITY, velocity->x, velocity->y, velocity->z ) );
}

static void xal_source_set_pitch( ApeAudioSource *self, float pitch )
{
	XAL_CALL( alSourcef( self->user, AL_PITCH, pitch ) );
}

static void xal_source_set_volume( ApeAudioSource *self, float volume )
{
	XAL_CALL( alSourcef( self->user, AL_GAIN, volume ) );
}

static void xal_source_set_loop( ApeAudioSource *self, bool loop )
{
	XAL_CALL( alSourcei( self->user, AL_LOOPING, loop ) );
}

static void xal_source_set_reverb( ApeAudioSource *self, ApeAudioReverbPreset reverb )
{
	if ( !xalExtensions[ XAL_EXTENSION_EFX ] )
	{
		return;
	}

	ALuint slot = AL_EFFECTSLOT_NULL;
	if ( reverb != APE_AUDIO_REVERB_PRESET_NONE )
	{
		const XalEffect *effect = get_effect( reverb );
		if ( effect != nullptr )
		{
			slot = effect->aux;
		}
	}

	XAL_CALL( alSource3i( self->user, AL_AUXILIARY_SEND_FILTER, slot, 0, AL_FILTER_NULL ) );
}

static bool xal_source_is_playing( const ApeAudioSource *source )
{
	ALint state;
	XAL_CALL( alGetSourcei( source->user, AL_SOURCE_STATE, &state ) );
	return state == AL_PLAYING;
}

static void xal_source_emit( ApeAudioSource *self, ApeAudioSample *sample )
{
	XAL_CALL( alSourcei( self->user, AL_BUFFER, sample->user ) );
	XAL_CALL( alSourcePlay( self->user ) );
}

/////////////////////////////////////////////////////////////////////////////////////

const ApeAudioDriverInterface ape_audioDriverOpenAL_ = {
        .name = "openal",

        .initialize  = initialize_openal,
        .shutdown    = shutdown_openal,
        .tick        = xal_tick,
        .pause       = xal_pause,
        .cacheSample = xal_cache_sample,
        .freeSample  = xal_free_sample,
        .emitSample  = xal_emit_sample,

        .createSource      = xal_source_create,
        .destroySource     = xal_source_destroy,
        .setSourcePosition = xal_source_set_position,
        .setSourceVelocity = xal_source_set_velocity,
        .setSourcePitch    = xal_source_set_pitch,
        .setSourceVolume   = xal_source_set_volume,
        .setSourceLoop     = xal_source_set_loop,
        .setSourceReverb   = xal_source_set_reverb,
        .isSourcePlaying   = xal_source_is_playing,
        .emitSource        = xal_source_emit,
};

#endif
