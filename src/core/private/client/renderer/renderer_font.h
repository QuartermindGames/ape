/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

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

void YR_Font_Initialize( void );
void Font_Shutdown( void );
