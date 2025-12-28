// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "aux.h"

PL_EXTERN_C

#define COM_FORMAT_FONT_MAGIC   PL_MAGIC_TO_NUM( 'O', 'S', 'F', 'N' )
#define COM_FORMAT_FONT_VERSION 2

typedef struct ComFontGlyph
{
	uint32_t codepoint;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;

	QmMathVector4i rect;
} ComFontGlyph;

PL_EXTERN_C_END
