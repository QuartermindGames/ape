// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Video API
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "video.h"

#include "renderer/renderer_texture.h"

ApeVideo *ape_video_load( const char *path )
{
	//TODO: do this by magic rather than extension in future...

	const char *ext = strrchr( path, '.' );
	if ( ext != nullptr )
	{
		if ( pl_strcasecmp( ext, ".smk" ) == 0 )
		{
			return ape_video_smk_load_( path );
		}
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
