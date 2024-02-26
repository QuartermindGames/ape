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

typedef struct GuiCanvas GuiCanvas;// represents what the GUI draws to

GuiCanvas *ss_gui_canvas_create( int width, int height );
void guiDestroyCanvas( GuiCanvas *canvas );
void gui_canvas_set_size( GuiCanvas *canvas, int width, int height );
void guiGetCanvasSize( GuiCanvas *canvas, int *width, int *height );
PLGTexture *guiGetCanvasTexture( GuiCanvas *canvas );

/****************************************
 ****************************************/

typedef struct GuiFont GuiFont;
typedef struct GuiStyleSheet GuiStyleSheet;

typedef struct GuiPanel GuiPanel;

bool ss_gui_initialize( void );
void ss_gui_shutdown( void );

const GuiStyleSheet *ss_gui_cache_style_sheet( const char *path );
void ss_gui_set_style_sheet( const GuiStyleSheet *styleSheet );
const GuiStyleSheet *guiGetActiveStyleSheet( void );

void gui_panel_tick( GuiPanel *root );
void gui_canvas_draw( GuiCanvas *canvas, GuiPanel *root );

typedef enum GuiMouseButton
{
	GUI_MOUSE_BUTTON_LEFT,
	GUI_MOUSE_BUTTON_RIGHT,
	GUI_MOUSE_BUTTON_MIDDLE,
	GUI_MAX_MOUSE_BUTTONS
} GuiMouseButton;

void guiUpdateMousePosition( int x, int y );
void gui_update_mouse_wheel( float x, float y );
void guiUpdateMouseButton( GuiMouseButton button, bool isDown );

/****************************************
 * Panel
 ****************************************/

typedef enum GuiPanelBackground
{
	GUI_PANEL_BACKGROUND_NONE,
	GUI_PANEL_BACKGROUND_DEFAULT,
	GUI_PANEL_BACKGROUND_SOLID,
} GuiPanelBackground;

typedef enum GuiPanelBorder
{
	GUI_PANEL_BORDER_NONE,
	GUI_PANEL_BORDER_INSET,
	GUI_PANEL_BORDER_OUTSET,

	GUI_MAX_BORDER_STYLES
} GuiPanelBorder;

GuiPanel *ss_gui_panel_create( GuiPanel *parent, int x, int y, int w, int h, GuiPanelBackground background, GuiPanelBorder border );
void ss_gui_panel_destroy( GuiPanel *self );

void guiSetPanelStyleSheet( GuiPanel *self, const GuiStyleSheet *styleSheet );

void guiDrawPanel( GuiPanel *self );
void guiDrawPanelBackground( GuiPanel *self );
void guiTickPanel( GuiPanel *self );

void guiSetPanelBackgroundColour( GuiPanel *self, const PLColour *colour );
PLColour guiGetPanelBackgroundColour( GuiPanel *self );

void guiSetPanelBorder( GuiPanel *self, GuiPanelBorder border );
void guiSetPanelBackground( GuiPanel *self, GuiPanelBackground background );

GuiPanel *guiGetPanelParent( GuiPanel *self );

void guiGetPanelPosition( GuiPanel *self, int *x, int *y );
void guiGetPanelContentPosition( GuiPanel *self, int *x, int *y );
void guiGetPanelAbsolutePosition( GuiPanel *self, int *x, int *y );

void guiSetPanelPosition( GuiPanel *self, int x, int y );

void guiGetPanelSize( GuiPanel *self, int *w, int *h );
void guiGetPanelContentSize( GuiPanel *self, int *w, int *h );

void gui_panel_set_size( GuiPanel *self, int w, int h );

bool guiIsMouseOverPanel( GuiPanel *self, int mx, int my );

bool guiHandleMousePanelEvent( GuiPanel *self, int mx, int my, int wheel, int button, bool buttonUp );
bool guiHandleKeyboardPanelEvent( GuiPanel *self, int button, bool buttonUp );

void ss_gui_panel_set_visible( GuiPanel *self, bool flag );

/****************************************
 * Cursor
 ****************************************/

GuiPanel *ss_gui_cursor_create( GuiPanel *parent, int x, int y );
void guiDestroyCursor( GuiPanel *self );

/****************************************
 ****************************************/

/****************************************
 * Font
 ****************************************/

typedef enum GuiFontDefaultType
{
	GUI_FONT_DEFAULT_LARGE,
	GUI_FONT_DEFAULT_MEDIUM,
	GUI_FONT_DEFAULT_SMALL,
	GUI_FONT_DEFAULT_TINY,

	GUI_MAX_FONT_DEFAULTS
} GuiFontDefaultType;

float guiGetFontLineSpacing( const GuiFont *font );

/**
 * Returns the specified default font.
 */
GuiFont *gui_get_default_font( GuiFontDefaultType defaultType );

void guiDestroyFont( GuiFont *font );

GuiFont *guiDeserializeFont( PLFile *file );
GuiFont *guiLoadFontFile( const char *path );

void guiGetCharacterPixelSize( const GuiFont *font, float scale, uint32_t character, float *dw, float *dh );
float guiGetCharacterPixelWidth( const GuiFont *font, float scale, uint32_t character );

void guiGetStringPixelSize( const GuiFont *font, float scale, const char *string, size_t length, float *dw, float *dh );
void gui_font_draw_character( const GuiFont *font, float x, float y, float scale, const PLColour *colour, uint32_t character );
void gui_font_draw_string( const GuiFont *font, float x, float y, float *ox, float *oy, float scale, const PLColour *colour, const char *string, size_t length, bool shadow );
void gui_font_display( GuiFont *font );

/****************************************
 * Desktop
 * A simplified GUI environment for
 * tooling.
 ****************************************/

typedef struct GuiDesktop GuiDesktop;

GuiDesktop *guiCreateDesktop( GuiPanel *parent );
void guiDestroyDesktop( GuiDesktop *desktop );

PL_EXTERN_C_END
