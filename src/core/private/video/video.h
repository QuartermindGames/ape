// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape_video.h"

typedef struct ApeTexture ApeTexture;

typedef struct ApeVideo
{
	unsigned int curFrame;
	unsigned int numFrames;
	ApeTexture **frames;

	ApeAudioSample *audioSample;
	ApeAudioSource *audioSource;

	float playtime;
	float framerate;

	bool isPaused;

	unsigned int width;
	unsigned int height;
} ApeVideo;

ApeVideo *ape_video_smk_load_( const char *path );
