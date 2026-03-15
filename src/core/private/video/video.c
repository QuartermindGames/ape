// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Video API
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "video.h"

#include "renderer/renderer_texture.h"
#include "renderer/material/material.h"

ApeVideo *ape_video_load( const char *path )
{
	//TODO: do this by magic rather than extension in future...

	const char *ext = strrchr( path, '.' );
	if ( ext != nullptr )
	{
#if defined( APE_SUPPORT_SMACKER )
		if ( pl_strcasecmp( ext, ".smk" ) == 0 )
		{
			return ape_video_smk_load_( path );
		}
#endif
	}

	ape_console_warning_( "Failed to identify video format (%s)!\n", path );
	return nullptr;
}

void ape_video_destroy( ApeVideo *video )
{
	for ( unsigned int i = 0; i < video->numFrames; ++i )
	{
		ape_texture_release_( video->frames[ i ] );
		video->frames[ i ] = nullptr;
	}

	qm_os_memory_free( video->frames );
	qm_os_memory_free( video );
}

void ape_video_draw( ApeVideo *video, const ApeViewport *viewport )
{
	//TODO: all of this is temporary, instead video playback should *probably* be piped through the material system
	//		we could probably treat videos as a special texture type with multiple frames, just not sure how audio
	//		will be handled...

	ApeShaderProgram *program = ape_get_default_shader( APE_SHADER_DEFAULT );
	ape_shader_set_active_( program );

	ApeTexture *frame = video->frames[ video->curFrame ];
	PlgSetTexture( frame->internal, 0 );

	float ratio = QM_OS_MIN( ( float ) viewport->width / video->width,
	                         ( float ) viewport->height / video->height );

	float w = video->width * ratio;
	ape_draw_textured_quad( nullptr, viewport->width / 2.0f - w / 2.0f, 0.0f,
	                        w,
	                        video->height * ratio,
	                        &PL_COLOUR_WHITE, 0 );

	PlgSetTexture( nullptr, 0 );
}

void ape_video_tick( ApeVideo *video, double delta )
{
	if ( !ape_video_is_playing( video ) )
	{
		return;
	}

	video->playtime += delta;

	double secondsPerFrame = video->framerate / 1000000.0;
	while ( video->playtime >= secondsPerFrame )
	{
		video->curFrame++;
		video->playtime -= secondsPerFrame;
	}
}

bool ape_video_is_playing( const ApeVideo *video )
{
	if ( video->curFrame >= video->numFrames )
	{
		return false;
	}

	return !video->isPaused;
}
