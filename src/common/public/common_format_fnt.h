// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "common.h"

PL_EXTERN_C

#define OSW_FONT_MAGIC   PL_MAGIC_TO_NUM( 'O', 'S', 'F', 'N' )
#define OSW_FONT_VERSION 1

typedef struct OSWFontGlyph {
	uint32_t codepoint;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
} OSWFontGlyph;

PL_EXTERN_C_END
