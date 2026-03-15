// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Integration with libsmacker, for SMK playback.
//			https://libsmacker.sourceforge.net/
// Author:  Mark E. Sowden

#if defined( APE_SUPPORT_SMACKER )

#	include <smacker.h>

// this just appears to be a magic number otherwise...
static constexpr unsigned int SMK_TRACKS = 7;

#	include "yin/core_fs.h"

#	include "ape_private.h"

#	include "video.h"
#	include "renderer/renderer_texture.h"

ApeVideo *ape_video_smk_load_( const char *path )
{
	size_t         bufSize;
	unsigned char *buf = ape_fs_load_file_buffer( path, &bufSize );
	if ( buf == nullptr )
	{
		ape_console_warning_( "Failed to open SMK file (%s)!\n", path );
		return nullptr;
	}

	ApeVideo *video = {};

	smk s = smk_open_memory( buf, bufSize );
	if ( s == nullptr )
	{
		ape_console_warning_( "Failed to open SMK (%s), likely an invalid smacker video!", path );
		goto cleanup;
	}

	unsigned long numFrames;
	double        usf;
	smk_info_all( s, nullptr, &numFrames, &usf );

	unsigned long w;
	unsigned long h;
	smk_info_video( s, &w, &h, nullptr );

	const unsigned int frameSize = w * h;

	unsigned char mask;
	unsigned char channels[ SMK_TRACKS ];
	unsigned char depth[ SMK_TRACKS ];
	unsigned long rate[ SMK_TRACKS ];
	smk_info_audio( s, &mask, channels, depth, rate );

	smk_enable_all( s, SMK_VIDEO_TRACK | SMK_AUDIO_TRACK_0 );

	smk_first( s );

	const QmMathColour3ub *pal = ( QmMathColour3ub * ) smk_get_palette( s );

	ApeTexture **frames = QM_OS_MEMORY_NEW_( ApeTexture *, numFrames );
	for ( unsigned int i = 0; i < numFrames; ++i, smk_next( s ) )
	{
		const unsigned char *vidFrame = smk_get_video( s );
		assert( vidFrame != nullptr );

		// frames are probably small enough we can just do this on the stack
		QmMathColour3ub out[ frameSize ] = {};
		for ( unsigned int j = 0; j < frameSize; ++j )
		{
			out[ j ] = pal[ vidFrame[ j ] ];
		}

		char tmp[ 64 ];
		snprintf( tmp, sizeof( tmp ), "frame_%u.png", i );

#	if 0
		PLImage *image = PlCreateImage( out, w, h, 0, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_RGB8 );
		PlWriteImage( image, tmp, 0 );
		PlDestroyImage( image );
#	endif

		frames[ i ] = ape_texture_generate_( tmp, out, w, h, &QM_IMAGE_FORMAT_RGB8_DESC(), PLG_TEXTURE_FILTER_NEAREST );
	}

	// populate video object
	video            = QM_OS_MEMORY_NEW( ApeVideo );
	video->width     = w;
	video->height    = h;
	video->numFrames = numFrames;
	video->frames    = frames;
	video->framerate = ( float ) usf;

cleanup:
	qm_os_memory_free( buf );

	if ( s != nullptr )
	{
		smk_close( s );
	}

	return video;
}

#endif
