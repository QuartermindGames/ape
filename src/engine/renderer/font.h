/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

void Font_Initialize( void );
void Font_Shutdown( void );

void Font_DrawBitmapCharacter( float x, float y, float scale, PLColour colour, char character );
void Font_DrawBitmapString( float x, float y, float spacing, float scale, PLColour colour, const char *msg, bool shadow );
