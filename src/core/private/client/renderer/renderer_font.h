/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

typedef struct BitmapFont
{
	struct PLGMesh * mesh; /* for batching */
	struct ApeMaterial *material;
	int				 w, h, cw, ch;
	char			 path[ PL_SYSTEM_MAX_PATH ];
	unsigned int	 start, end;

	ApeMemoryReference mem;
} BitmapFont;

void YR_Font_Initialize( void );
void Font_Shutdown( void );

BitmapFont *Font_CacheBitmap( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end );
void		Font_ReleaseBitmap( BitmapFont *font );

BitmapFont *Font_GetDefault( void );
BitmapFont *Font_GetDefaultSmall( void );

void Font_AddBitmapCharacterToPass( const BitmapFont *font, float x, float y, float scale, PLColour colour, uint8_t character );
void Font_AddBitmapStringToPass( const BitmapFont *font, float x, float y, float scale, PLColour colour, const char *msg, size_t length, bool shadow );

void Font_DrawBitmapCharacter( BitmapFont *font, float x, float y, float scale, PLColour colour, char character );
void Font_DrawBitmapString( BitmapFont *font, float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow );

void Font_BeginDraw( BitmapFont *font );
void Font_Draw( BitmapFont *font );
