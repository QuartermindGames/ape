// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "audio.h"

#if defined( _WIN32 )

#	include <Windows.h>
#	define XAUDIO2_HELPER_FUNCTIONS
#	include <xaudio2.h>

#	if defined( _MSC_VER )    // apparently not available under mingw :(
#		define ENABLE_3D_AUDIO// comment this out to disable X3DAudio
#	endif
#	if defined( ENABLE_3D_AUDIO )
#		include <x3daudio.h>

static X3DAUDIO_HANDLE audio3DHandle;
static bool            audio3DSupported = false;
#    endif

static IXAudio2               *audioEngineInstance = nullptr;
static IXAudio2MasteringVoice *audioMasteringVoice = nullptr;

typedef struct VoiceWrapper
{
	IXAudio2SourceVoice *voice;
	bool                 autoCleanup;
} VoiceWrapper;
QmOsLinkedList *voicesList;

static bool Audio_XAudio2_Initialize()
{
	ape_console_print_( "Attempting to initialize XAudio2 driver...\n" );

	HRESULT result = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
	if ( result != S_OK && result != S_FALSE && result != RPC_E_CHANGED_MODE )
	{
		ape_console_warning_( "COINIT_MULTITHREADED failed (%X)!\n", result );
		return false;
	}

	if ( FAILED( XAudio2Create( &audioEngineInstance, 0, XAUDIO2_DEFAULT_PROCESSOR ) ) )
	{
		ape_console_warning_( "Failed to create XAudio2 instance!\n" );
		return false;
	}

	if ( FAILED( audioEngineInstance->CreateMasteringVoice(
	             &audioMasteringVoice,
	             XAUDIO2_DEFAULT_CHANNELS,
	             XAUDIO2_DEFAULT_SAMPLERATE,
	             0,
	             nullptr,
	             nullptr,
	             AudioCategory_GameEffects ) ) )
	{
		ape_console_warning_( "Failed to create mastering voice!\n" );
		return false;
	}

	XAUDIO2_VOICE_DETAILS voiceDetails;
	audioMasteringVoice->GetVoiceDetails( &voiceDetails );
	ape_console_print_( "Channels:    %u\n", voiceDetails.InputChannels );
	ape_console_print_( "Sample Rate: %u\n", voiceDetails.InputSampleRate );

	DWORD channelMask;
	audioMasteringVoice->GetChannelMask( &channelMask );

	voicesList = qm_os_linked_list_create();

#	if defined( ENABLE_3D_AUDIO )
	// now we're going to try initializing X3DAudio
	ape_console_print_( "Setting up X3DAudio... " );
	if ( FAILED( X3DAudioInitialize( SPEAKER_STEREO, X3DAUDIO_SPEED_OF_SOUND, audio3DHandle ) ) )
		ape_console_warning_( "Failed to initialize 3D audio!\n" );
	else
	{
		audio3DSupported = true;
		ape_console_print_( "Successfully initialized X3DAudio!\n" );
	}
#	endif

	ape_console_print_( "XAudio2 driver initialized successfully!\n" );

	return true;
}

static void Audio_XAudio2_Shutdown()
{
	qm_os_memory_free( voicesList );

	if ( audioMasteringVoice != nullptr )
	{
		audioMasteringVoice->DestroyVoice();
		audioMasteringVoice = nullptr;
	}

	audioEngineInstance->StopEngine();
	audioEngineInstance->Release();

	CoUninitialize();
}

static void Audio_XAudio2_Tick()
{
#	if defined( ENABLE_3D_AUDIO )
	X3DAUDIO_LISTENER listener = {};

	QmMathVector3f pos  = ape_audio_get_listener_position();
	listener.Position.x = pos.x;
	listener.Position.y = pos.y;
	listener.Position.z = pos.z;

	QmMathVector3f vel  = ape_audio_get_listener_velocity();
	listener.Velocity.x = vel.x;
	listener.Velocity.y = vel.y;
	listener.Velocity.z = vel.z;

	QmMathVector3f angles = ape_audio_get_listener_angles();

	//X3DAudioCalculate()
#	endif
}

static void Audio_XAudio2_Pause( bool pause )
{
	if ( pause )
	{
		audioEngineInstance->StopEngine();
		return;
	}

	audioEngineInstance->StartEngine();
}

static void Audio_XAudio2_CacheSample( ApeAudioSample *audioSample )
{
}

static void Audio_XAudio2_FreeSample( ApeAudioSample *audioSample )
{
}

static void Audio_XAudio2_EmitSample( ApeAudioSample *audioSample, const QmMathVector3f *position, float volume, float pitch )
{
	XAUDIO2_BUFFER buffer;
	PL_ZERO_( buffer );
	buffer.AudioBytes = audioSample->bufferSize;
	buffer.pAudioData = static_cast< BYTE * >( audioSample->buffer );
	buffer.Flags      = XAUDIO2_END_OF_STREAM;
}

static bool Audio_XAudio2_CreateSource( ApeAudioSource *source )
{
	WAVEFORMATEX waveFormat;
	PL_ZERO_( waveFormat );
	//waveFormat.

	IXAudio2SourceVoice *voice;
	//if ( FAILED( audioEngineInstance->CreateSourceVoice( &voice, &WAVE_FORMAT_PCM ) ) )
	//{
	//	PrintWarn( "Failed to create source voice!\n" );
	//	return false;
	//}

	source->user = reinterpret_cast< intptr_t >( voice );
	return true;
}

static void Audio_XAudio2_DestroySource( ApeAudioSource *source )
{
	IXAudio2SourceVoice *voice = reinterpret_cast< IXAudio2SourceVoice * >( source->user );
	voice->DestroyVoice();
}

extern "C" const ApeAudioDriverInterface *Audio_XAudio2_GetDriverInterface()
{
	static ApeAudioDriverInterface driverInterface;
	PL_ZERO_( driverInterface );

	driverInterface.initialize = Audio_XAudio2_Initialize;
	driverInterface.shutdown   = Audio_XAudio2_Shutdown;
	driverInterface.tick       = Audio_XAudio2_Tick;
	driverInterface.pause      = Audio_XAudio2_Pause;

	driverInterface.emitSample = Audio_XAudio2_EmitSample;
	driverInterface.freeSample = Audio_XAudio2_FreeSample;

	driverInterface.createSource  = Audio_XAudio2_CreateSource;
	driverInterface.destroySource = Audio_XAudio2_DestroySource;

	return &driverInterface;
}

#else

extern "C" const ApeAudioDriverInterface *Audio_XAudio2_GetDriverInterface()
{
	return nullptr;
}

#endif
