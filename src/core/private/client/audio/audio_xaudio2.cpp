// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

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
static bool audio3DSupported = false;
#	endif

static IXAudio2 *audioEngineInstance               = nullptr;
static IXAudio2MasteringVoice *audioMasteringVoice = nullptr;

typedef struct VoiceWrapper
{
	IXAudio2SourceVoice *voice;
	bool autoCleanup;
} VoiceWrapper;
PLLinkedList *voicesList;

static bool Audio_XAudio2_Initialize()
{
	PRINT( "Attempting to initialize XAudio2 driver...\n" );

	HRESULT result = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
	if ( result != S_OK && result != S_FALSE && result != RPC_E_CHANGED_MODE )
	{
		PRINT_WARNING( "COINIT_MULTITHREADED failed (%X)!\n", result );
		return false;
	}

	if ( FAILED( XAudio2Create( &audioEngineInstance, 0, XAUDIO2_DEFAULT_PROCESSOR ) ) )
	{
		PRINT_WARNING( "Failed to create XAudio2 instance!\n" );
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
		PRINT_WARNING( "Failed to create mastering voice!\n" );
		return false;
	}

	XAUDIO2_VOICE_DETAILS voiceDetails;
	audioMasteringVoice->GetVoiceDetails( &voiceDetails );
	PRINT( "Channels:    %u\n", voiceDetails.InputChannels );
	PRINT( "Sample Rate: %u\n", voiceDetails.InputSampleRate );

	DWORD channelMask;
	audioMasteringVoice->GetChannelMask( &channelMask );

	voicesList = PlCreateLinkedList();

#	if defined( ENABLE_3D_AUDIO )
	// now we're going to try initializing X3DAudio
	PRINT( "Setting up X3DAudio... " );
	if ( FAILED( X3DAudioInitialize( SPEAKER_STEREO, X3DAUDIO_SPEED_OF_SOUND, audio3DHandle ) ) )
		PRINT_WARNING( "Failed to initialize 3D audio!\n" );
	else
	{
		audio3DSupported = true;
		PRINT( "Successfully initialized X3DAudio!\n" );
	}
#	endif

	PRINT( "XAudio2 driver initialized successfully!\n" );

	return true;
}

static void Audio_XAudio2_Shutdown()
{
	PlDestroyLinkedList( voicesList );

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
	X3DAUDIO_LISTENER listener;
	PL_ZERO_( listener );

	PLVector3 v;
	v                   = Audio_GetListenerPosition();
	listener.Position.x = v.x;
	listener.Position.y = v.y;
	listener.Position.z = v.z;
	v                   = Audio_GetListenerVelocity();
	listener.Velocity.x = v.x;
	listener.Velocity.y = v.y;
	listener.Velocity.z = v.z;

	PLVector3 angles = Audio_GetListenerAngles();

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

static void Audio_XAudio2_EmitSample( ApeAudioSample *audioSample, int8_t volume )
{
	XAUDIO2_BUFFER buffer;
	PL_ZERO_( buffer );
	buffer.AudioBytes = audioSample->bufferSize;
	buffer.pAudioData = audioSample->buffer;
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

	source->user = voice;
	return true;
}

static void Audio_XAudio2_DestroySource( ApeAudioSource *source )
{
	IXAudio2SourceVoice *voice = ( IXAudio2SourceVoice * ) source->user;
	voice->DestroyVoice();
}

extern "C" const ApeAudioDriverInterface *Audio_XAudio2_GetDriverInterface()
{
	static ApeAudioDriverInterface driverInterface;
	PL_ZERO_( driverInterface );

	driverInterface.Initialize = Audio_XAudio2_Initialize;
	driverInterface.Shutdown   = Audio_XAudio2_Shutdown;
	driverInterface.Tick       = Audio_XAudio2_Tick;
	driverInterface.Pause      = Audio_XAudio2_Pause;

	driverInterface.EmitSample = Audio_XAudio2_EmitSample;
	driverInterface.FreeSample = Audio_XAudio2_FreeSample;

	driverInterface.CreateSource  = Audio_XAudio2_CreateSource;
	driverInterface.DestroySource = Audio_XAudio2_DestroySource;

	return &driverInterface;
}

#else

extern "C" const ApeAudioDriverInterface *Audio_XAudio2_GetDriverInterface()
{
	return nullptr;
}

#endif
