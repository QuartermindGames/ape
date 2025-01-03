// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "yin/core_renderer.h"

typedef struct ApeBitmapFont
{
	struct PLGMesh *mesh; /* for batching */
	struct ApeMaterial *material;
	int w, h, cw, ch;
	char path[ PL_SYSTEM_MAX_PATH ];
	unsigned int start, end;

	ApeMemoryReference mem;
} ApeBitmapFont;

void ape_initialize_bitmap_fonts_( void );
void ape_shutdown_bitmap_fonts_( void );
