/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

enum
{
	PL_BITFLAG( PGFX_CHANNEL_RED, 0 ),
	PL_BITFLAG( PGFX_CHANNEL_GREEN, 1 ),
	PL_BITFLAG( PGFX_CHANNEL_BLUE, 2 ),
	PL_BITFLAG( PGFX_CHANNEL_ALPHA, 3 ),
};

enum
{
	PGFX_FORMAT_CLUSTER    = 0,
	PGFX_FORMAT_DXT1       = 1,
	PGFX_FORMAT_DXT1_ALPHA = 2,
	PGFX_FORMAT_DXT3       = 3,
	PGFX_FORMAT_DXT5       = 4,
};

#define GFX_IDENTIFIER "GFX1"
