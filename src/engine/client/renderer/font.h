/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

typedef struct BitmapFont
{
	struct PLGMesh * mesh; /* for batching */
	struct Material *material;
	int              w, h, cw, ch;
	char             path[ PL_SYSTEM_MAX_PATH ];
	unsigned int     start, end;

	MemRefCnt mem;
} BitmapFont;

void Font_Initialize( void );
void Font_Shutdown( void );

BitmapFont *Font_CacheBitmap( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end );
void        Font_ReleaseBitmap( BitmapFont *font );

BitmapFont *Font_GetDefault( void );

void Font_AddBitmapCharacterToPass( const BitmapFont *font, float x, float y, float scale, PLColour colour, uint8_t character );
void Font_AddBitmapStringToPass( const BitmapFont *font, float x, float y, float scale, PLColour colour, const char *msg, size_t length );

void Font_DrawBitmapCharacter( BitmapFont *font, float x, float y, float scale, PLColour colour, char character );
void Font_DrawBitmapString( BitmapFont *font, float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow );

void Font_BeginDraw( BitmapFont *font );
void Font_Draw( BitmapFont *font );
