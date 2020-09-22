/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

#include <SDL2/SDL_audio.h>

#include "yin.h"
#include "audio.h"

typedef struct AudioSample {

} AudioSample;

static SDL_AudioSpec audioSpec;
static SDL_AudioDeviceID audioDeviceID;

void Audio_Initialize( void ) {
	SDL_AudioSpec desiredSpec;
	memset( &desiredSpec, 0, sizeof( SDL_AudioSpec ) );
	desiredSpec.channels = 2;
	desiredSpec.format = AUDIO_F32;
	desiredSpec.freq = 48000;
	desiredSpec.samples = 4096;
	desiredSpec.callback = NULL;

	audioDeviceID = SDL_OpenAudioDevice( NULL, 0, &desiredSpec, &audioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE );
	if ( audioDeviceID == 0 ) {
		PrintWarn( "Failed to open audio device!\nSDL: %s\n", SDL_GetError() );
		return;
	}
}

void Audio_Shutdown( void ) {
	SDL_CloseAudioDevice( audioDeviceID );
}
