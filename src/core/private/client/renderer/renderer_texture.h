// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <yin/core_renderer.h>

typedef struct OgeTexture
{
	ApeMemoryReference reference;
	PLGTexture *internal;
} OgeTexture;
