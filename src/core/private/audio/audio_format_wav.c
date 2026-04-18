// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

// we can just use sdl for this :)
#include <SDL3/SDL.h>

#include "audio.h"

ApeAudioSample *ape_audio_format_wav_load_( QmFsFile *file )
{
	uint32_t size = qm_fs_file_get_size( file );
	uint8_t *buf  = QM_OS_MEMORY_NEW_( uint8_t, size );

	ApeAudioSample *sample = nullptr;

	if ( qm_file_read( file, buf, sizeof( uint8_t ), size ) == size )
	{
		uint8_t      *dstBuf;
		SDL_AudioSpec dstSpec;
		if ( SDL_LoadWAV_IO( SDL_IOFromConstMem( buf, ( int ) size ), 1, &dstSpec, &dstBuf, &size ) )
		{
			if ( dstSpec.channels > 0 && dstSpec.channels <= 2 )
			{
				ApeAudioSampleFormat format;
				if ( dstSpec.format == SDL_AUDIO_U8 )
				{
					format = ( dstSpec.channels == 1 ) ? APE_AUDIO_SAMPLE_FORMAT_MONO8 : APE_AUDIO_SAMPLE_FORMAT_STEREO8;
				}
				else if ( dstSpec.format == SDL_AUDIO_S16 )
				{
					format = ( dstSpec.channels == 1 ) ? APE_AUDIO_SAMPLE_FORMAT_MONO16 : APE_AUDIO_SAMPLE_FORMAT_STEREO16;
				}
				else
				{
					format = APE_AUDIO_SAMPLE_FORMAT_INVALID;
				}

				if ( format != APE_AUDIO_SAMPLE_FORMAT_INVALID )
				{
					sample             = QM_OS_MEMORY_NEW( ApeAudioSample );
					sample->buffer     = QM_OS_MEMORY_NEW_( uint8_t, size );
					sample->bufferSize = size;
					sample->channels   = dstSpec.channels;
					sample->type       = format;
					sample->sampleRate = dstSpec.freq;

					memcpy( sample->buffer, dstBuf, size );
				}
			}
			else
			{
				ape_console_warning_( "Unsupported number of channels in wav (%u)!\n", dstSpec.channels );
			}

			SDL_free( dstBuf );
		}
		else
		{
			ape_console_warning_( "Failed to load wav file: %s\n", SDL_GetError() );
		}
	}
	else
	{
		ape_console_warning_( "Failed to read wav file: %s\n", PlGetError() );
	}

	qm_os_memory_free( buf );

	return sample;
}
