// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "audio.h"

#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_STDIO
#include "stb_vorbis.c"

ApeAudioSample *ape_audio_format_vorbis_load_( PLFile *file )
{
	const uint8 *data   = PlGetFileData( file );
	const int    length = ( int ) PlGetFileSize( file );

	int    channels;
	int    sampleRate;
	short *output;

	int samples = stb_vorbis_decode_memory( data, length, &channels, &sampleRate, &output );
	if ( samples < 0 )
	{
		PL_DELETE( output );

		ape_warning_( "Failed to decode ogg!\n" );
		return nullptr;
	}
	else if ( channels == 0 || channels > 2 )
	{
		PL_DELETE( output );

		ape_warning_( "Invalid number of channels for ogg!\n" );
		return nullptr;
	}

	ApeAudioSample *sample = PL_NEW( ApeAudioSample );
	sample->buffer         = output;
	sample->bufferSize     = samples * channels * sizeof( int16_t );
	sample->sampleRate     = sampleRate;
	sample->channels       = channels;
	if ( sample->channels == 1 )
	{
		sample->type = APE_AUDIO_SAMPLE_FORMAT_MONO16;
	}
	else if ( sample->channels == 2 )
	{
		sample->type = APE_AUDIO_SAMPLE_FORMAT_STEREO16;
	}

	return sample;
}
