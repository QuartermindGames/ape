/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

/* Collection of flags used for texture properties.
 * If these are altered, be sure to rebuild both the tools
 * and engine.
 * */

typedef enum TextureFlag {
	TEXTURE_FLAG_NONE = 0,

	TEXTURE_FLAG_ALPHA      = ( 1 << 0 ),   /* alpha tested */
	TEXTURE_FLAG_FULLBRIGHT = ( 1 << 1 ),   /* texture remains fully lit */
	TEXTURE_FLAG_EMITTER    = ( 1 << 2 ),   /* texture emits light */
} TextureFlag;
