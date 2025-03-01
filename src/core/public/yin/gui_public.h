// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_math.h>
#include <plgraphics/plg_camera.h>
#include <plgraphics/plg_texture.h>

#include <yin/core_renderer.h>

PL_EXTERN_C

typedef struct ApeVector2i
{
	int x, y;
} ApeVector2i;

/****************************************
 * Canvas
 ****************************************/

typedef struct ApeGuiCanvas ApeGuiCanvas;// represents what the GUI draws to

ApeGuiCanvas *ape_gui_canvas_create( int width, int height );
void          ape_gui_destroy_canvas( ApeGuiCanvas *canvas );
void          ape_gui_canvas_set_size( ApeGuiCanvas *canvas, int width, int height );
void          ape_gui_get_canvas_size( ApeGuiCanvas *canvas, int *width, int *height );
PLGTexture   *ape_gui_get_canvas_texture( ApeGuiCanvas *canvas );

/****************************************
 ****************************************/

typedef struct ApeGuiFont ApeGuiFont;

bool ape_gui_initialize_( void );
void ape_gui_shutdown_( void );

void gui_canvas_make_active( ApeGuiCanvas *canvas );
void gui_canvas_display( ApeGuiCanvas *canvas );

typedef enum GuiMouseButton
{
	GUI_MOUSE_BUTTON_LEFT,
	GUI_MOUSE_BUTTON_RIGHT,
	GUI_MOUSE_BUTTON_MIDDLE,
	GUI_MAX_MOUSE_BUTTONS
} GuiMouseButton;

void ape_gui_update_mouse_position_( int x, int y );
void gui_update_mouse_wheel( float x, float y );
void guiUpdateMouseButton( GuiMouseButton button, bool isDown );

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

#define GUI_FONT_SHADOW_DEFAULT 1.0f, 1.0f

/**
 * Retrieves the line spacing value from the specified GuiFont.
 *
 * @param font 	Pointer to the GuiFont structure from which to get the line spacing.
 * @return 		The line spacing value of the given font.
 */
float gui_font_get_line_spacing( const ApeGuiFont *font );

/**
 * Returns the specified default font.
 */
ApeGuiFont *gui_get_default_font( GuiFontDefaultType defaultType );

void guiDestroyFont( ApeGuiFont *font );

/**
 * Loads a GUI font from a file specified by the given path.
 *
 * This function attempts to open the font file at the specified path,
 * deserialize its contents into a GuiFont structure, and return a pointer
 * to the newly loaded GuiFont. If the font file cannot be opened or
 * deserialized, a warning message will be logged and a null pointer will be returned.
 *
 * @param path 	The file path to the font file to be loaded.
 * @return 		A pointer to the loaded GuiFont structure, or nullptr if loading failed.
 */
ApeGuiFont *gui_font_load( const char *path );

void  guiGetCharacterPixelSize( const ApeGuiFont *font, float scale, uint32_t character, float *dw, float *dh );
float guiGetCharacterPixelWidth( const ApeGuiFont *font, float scale, uint32_t character );

/**
 * @brief Sets the font slant angle for the GUI.
 *
 * This function adjusts the angle of inclination for the font used in the graphical user interface.
 *
 * @param slant The new slant angle for the font, in degrees. Positive values indicate a forward slant,
 * 				while negative values indicate a backward slant.
 */
void gui_font_set_slant( float slant );

/**
 * Sets the shadow offset for the font.
 *
 * @param x X offset.
 * @param y Y offset.
 */
void gui_font_set_shadow_offset( float x, float y );

/**
 * Calculates the pixel size of a given string when rendered with the specified font and scale.
 *
 * This function computes the width and height in pixels that a string would occupy when rendered
 * using the provided font at a specific scale. It takes into account special characters like
 * newlines and tabs, which affect the height and width calculations respectively.
 *
 * <b>WARNING: the height will only increment if a new line is provided!</b>
 *
 * @param self   Pointer to the GuiFont structure containing font information.
 * @param scale  Scaling factor to apply to the font sizes.
 * @param string The string for which to calculate the pixel size.
 * @param length Length of the string in characters.
 * @param dw     Pointer to a float where the calculated width will be stored (can be NULL).
 * @param dh     Pointer to a float where the calculated height will be stored (can be NULL).
 */
void gui_font_get_string_pixel_size( const ApeGuiFont *self, float scale, const char *string, size_t length, float *dw, float *dh );

/**
 * Draws a single character from a font at specified coordinates with a given scale and color.
 *
 * @param font 		Pointer to a GuiFont structure containing font information.
 * @param x 		The x-coordinate where the character will be drawn.
 * @param y 		The y-coordinate where the character will be drawn.
 * @param scale 	Scaling factor for the character size.
 * @param colour 	Pointer to a PLColour structure defining the color for the character.
 * @param character Unicode code point of the character to be drawn.
 */
void gui_font_draw_character( const ApeGuiFont *font, float x, float y, float scale, const PLColour *colour, uint32_t character );

/**
 * Draws a string using the specified font.
 *
 * <b>WARNING: the height will only increment if a new line is provided!</b>
 *
 * @param self 		A pointer to the GuiFont structure containing font data.
 * @param x 		The starting x-coordinate for the string.
 * @param y 		The starting y-coordinate for the string.
 * @param ox 		Output parameter. If not NULL, it will be updated with the x-coordinate after drawing the string.
 * @param oy 		Output parameter. If not NULL, it will be updated with the y-coordinate after drawing the string.
 * @param scale 	The scale factor applied to the font size.
 * @param colour 	A pointer to the PLColour structure defining the colour of the text.
 * @param string 	The string to be drawn.
 * @param length 	The length of the string to be drawn.
 * @param shadow 	If true, the text will be drawn with a shadow effect.
 */
void gui_font_draw_string( const ApeGuiFont *self, float x, float y, float *ox, float *oy, float scale, const PLColour *colour, const char *string, size_t length, bool shadow );

/**
 * @brief Renders the given bitmap font on the screen.
 *
 * This function sets up the necessary state in the rendering pipeline,
 * configures the appropriate shader program for font rendering, and
 * draws the font's associated mesh.
 *
 * @param font A pointer to a GuiFont structure containing the font's texture, glyphs, and mesh data.
 */
void gui_font_display( ApeGuiFont *font );

PL_EXTERN_C_END
