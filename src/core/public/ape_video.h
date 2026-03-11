// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

typedef struct ApeVideo ApeVideo;

ApeVideo *ape_video_load( const char *path );
void      ape_video_destroy( ApeVideo *video );
