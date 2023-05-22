// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_math.h>
#include <plgraphics/plg_camera.h>
#include <plgraphics/plg_texture.h>

#include <yin/core_renderer.h>

PL_EXTERN_C

typedef struct GUIVector2
{
	int x, y;
} GUIVector2;

/****************************************
 * Canvas
 ****************************************/

typedef struct GUICanvas GUICanvas;// represents what the GUI draws to

GUICanvas *GUI_CreateCanvas( int width, int height );
void GUI_DestroyCanvas( GUICanvas *canvas );
void GUI_SetCanvasSize( GUICanvas *canvas, int width, int height );
void GUI_GetCanvasSize( GUICanvas *canvas, int *width, int *height );
PLGTexture *GUI_GetCanvasTexture( GUICanvas *canvas );

/****************************************
 ****************************************/

typedef struct GUIFont GUIFont;
typedef struct GUIStyleSheet GUIStyleSheet;

typedef struct GUIPanel GUIPanel;

bool GUI_Initialize( void );
void GUI_Shutdown( void );

const GUIStyleSheet *GUI_CacheStyleSheet( const char *path );
void GUI_SetStyleSheet( const GUIStyleSheet *styleSheet );
const GUIStyleSheet *GUI_GetActiveStyleSheet( void );

void GUI_Tick( GUIPanel *root );
void GUI_Draw( GUICanvas *canvas, GUIPanel *root );

typedef enum GUIMouseButton
{
	GUI_MOUSE_BUTTON_LEFT,
	GUI_MOUSE_BUTTON_RIGHT,
	GUI_MOUSE_BUTTON_MIDDLE,
	GUI_MAX_MOUSE_BUTTONS
} GUIMouseButton;

void GUI_UpdateMousePosition( int x, int y );
void GUI_UpdateMouseWheel( float x, float y );
void GUI_UpdateMouseButton( GUIMouseButton button, bool isDown );

/****************************************
 * Panel
 ****************************************/

typedef enum GUIPanelBackground
{
	GUI_PANEL_BACKGROUND_NONE,
	GUI_PANEL_BACKGROUND_DEFAULT,
	GUI_PANEL_BACKGROUND_SOLID,
} GUIPanelBackground;

typedef enum GUIPanelBorder
{
	GUI_PANEL_BORDER_NONE,
	GUI_PANEL_BORDER_INSET,
	GUI_PANEL_BORDER_OUTSET,

	GUI_MAX_BORDER_STYLES
} GUIPanelBorder;

GUIPanel *GUI_Panel_Create( GUIPanel *parent, int x, int y, int w, int h, GUIPanelBackground background, GUIPanelBorder border );
void GUI_Panel_Destroy( GUIPanel *self );

void GUI_Panel_SetStyleSheet( GUIPanel *self, const GUIStyleSheet *styleSheet );

void GUI_Panel_Draw( GUIPanel *self );
void GUI_Panel_DrawBackground( GUIPanel *self );
void GUI_Panel_Tick( GUIPanel *self );

void GUI_Panel_SetBackgroundColour( GUIPanel *self, const PLColour *colour );
PLColour GUI_Panel_GetBackgroundColour( GUIPanel *self );

void GUI_Panel_SetBorder( GUIPanel *self, GUIPanelBorder border );
void GUI_Panel_SetBackground( GUIPanel *self, GUIPanelBackground background );

GUIPanel *GUI_Panel_GetParent( GUIPanel *self );

void GUI_Panel_GetPosition( GUIPanel *self, int *x, int *y );
void GUI_Panel_GetContentPosition( GUIPanel *self, int *x, int *y );
void GUI_Panel_GetAbsolutePosition( GUIPanel *self, int *x, int *y );

void GUI_Panel_SetPosition( GUIPanel *self, int x, int y );

void GUI_Panel_GetSize( GUIPanel *self, int *w, int *h );
void GUI_Panel_GetContentSize( GUIPanel *self, int *w, int *h );

void GUI_Panel_SetSize( GUIPanel *self, int w, int h );

bool GUI_Panel_IsMouseOver( GUIPanel *self, int mx, int my );

bool GUI_Panel_HandleMouseEvent( GUIPanel *self, int mx, int my, int wheel, int button, bool buttonUp );
bool GUI_Panel_HandleKeyboardEvent( GUIPanel *self, int button, bool buttonUp );

void GUI_Panel_SetVisible( GUIPanel *self, bool flag );

/****************************************
 * Cursor
 ****************************************/

GUIPanel *GUI_Cursor_Create( GUIPanel *parent, int x, int y );
void GUI_Cursor_Destroy( GUIPanel *self );

/****************************************
 ****************************************/

/****************************************
 * Font
 ****************************************/

typedef enum GUIFontDefaultType
{
	GUI_FONT_DEFAULT_LARGE,
	GUI_FONT_DEFAULT_MEDIUM,
	GUI_FONT_DEFAULT_SMALL,
	GUI_FONT_DEFAULT_TINY,

	GUI_MAX_FONT_DEFAULTS
} GUIFontDefaultType;

float GUI_Font_GetLineSpacing( const GUIFont *font );

GUIFont *GUI_Font_GetDefault( GUIFontDefaultType defaultType );

void GUI_Font_Destroy( GUIFont *font );

GUIFont *GUI_Font_Deserialize( PLFile *file );
GUIFont *GUI_Font_LoadFile( const char *path );

void GUI_Font_GetStringPixelSize( const GUIFont *font, float scale, const char *string, size_t length, float *dw, float *dh );
void GUI_Font_DrawCharacter( const GUIFont *font, float x, float y, float scale, const PLColour *colour, uint32_t character );
void GUI_Font_DrawString( const GUIFont *font, float x, float y, float *ox, float *oy, float scale, const PLColour *colour, const char *string, size_t length, bool shadow );
void GUI_Font_Display( GUIFont *font );

PL_EXTERN_C_END
