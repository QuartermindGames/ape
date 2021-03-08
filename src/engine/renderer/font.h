/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

typedef struct BitmapFont BitmapFont;

void Font_Initialize( void );
void Font_Shutdown( void );

BitmapFont *Font_LoadBitmap( const char *materialPath, int w, int h, int cw, int ch, unsigned int start, unsigned int end );
void Font_Destroy( BitmapFont *font );

BitmapFont *Font_GetDefault( void );

void Font_DrawBitmapCharacter( BitmapFont *font, float x, float y, float scale, PLColour colour, char character );
void Font_DrawBitmapString( BitmapFont *font, float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow );
