// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

// we can just use sdl2 for this :)
#include <SDL2/SDL.h>

#include "audio.h"

ApeAudioSample *ape_audio_format_wav_load_( PLFile *file )
{
	uint32_t size = PlGetFileSize( file );
	uint8_t *buf  = PL_NEW_( uint8_t, size );

	ApeAudioSample *sample = nullptr;

	if ( PlReadFile( file, buf, sizeof( uint8_t ), size ) == size )
	{
		uint8_t      *dstBuf;
		SDL_AudioSpec dstSpec;
		if ( SDL_LoadWAV_RW( SDL_RWFromConstMem( buf, ( int ) size ), 1, &dstSpec, &dstBuf, &size ) != nullptr )
		{
			if ( dstSpec.channels > 0 && dstSpec.channels <= 2 )
			{
				ApeAudioSampleFormat format;
				if ( dstSpec.format == AUDIO_U8 )
				{
					format = ( dstSpec.channels == 1 ) ? APE_AUDIO_SAMPLE_FORMAT_MONO8 : APE_AUDIO_SAMPLE_FORMAT_STEREO8;
				}
				else if ( dstSpec.format == AUDIO_S16 )
				{
					format = ( dstSpec.channels == 1 ) ? APE_AUDIO_SAMPLE_FORMAT_MONO16 : APE_AUDIO_SAMPLE_FORMAT_STEREO16;
				}
				else
				{
					format = APE_AUDIO_SAMPLE_FORMAT_INVALID;
				}

				if ( format != APE_AUDIO_SAMPLE_FORMAT_INVALID )
				{
					sample             = PL_NEW( ApeAudioSample );
					sample->buffer     = PL_NEW_( uint8_t, size );
					sample->bufferSize = size;
					sample->channels   = dstSpec.channels;
					sample->type       = format;
					sample->sampleRate = dstSpec.freq;

					memcpy( sample->buffer, dstBuf, size );
				}
			}
			else
			{
				ape_warning_( "Unsupported number of channels in wav (%u)!\n", dstSpec.channels );
			}

			SDL_FreeWAV( dstBuf );
		}
		else
		{
			ape_warning_( "Failed to load wav file: %s\n", SDL_GetError() );
		}
	}
	else
	{
		ape_warning_( "Failed to read wav file: %s\n", PlGetError() );
	}

	PL_DELETE( buf );

	return sample;
}
