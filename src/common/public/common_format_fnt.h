// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "common.h"

PL_EXTERN_C

#define COM_FORMAT_FONT_MAGIC   PL_MAGIC_TO_NUM( 'O', 'S', 'F', 'N' )
#define COM_FORMAT_FONT_VERSION 1

typedef struct ComFontGlyph
{
	uint32_t codepoint;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
} ComFontGlyph;

PL_EXTERN_C_END
