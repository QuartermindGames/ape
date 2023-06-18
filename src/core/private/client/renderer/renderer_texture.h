// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <yin/core_renderer.h>

typedef struct ApeTexture
{
	ApeMemoryReference reference;
	PLGTexture *internal;
} ApeTexture;
