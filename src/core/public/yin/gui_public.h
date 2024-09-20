// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

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

GuiCanvas  *ape_gui_canvas_create( int width, int height );
void        guiDestroyCanvas( GuiCanvas *canvas );
void        gui_canvas_set_size( GuiCanvas *canvas, int width, int height );
void        guiGetCanvasSize( GuiCanvas *canvas, int *width, int *height );
PLGTexture *guiGetCanvasTexture( GuiCanvas *canvas );

/****************************************
 ****************************************/

typedef struct GuiFont       GuiFont;
typedef struct GuiStyleSheet GuiStyleSheet;

typedef struct GuiPanel GuiPanel;

bool ape_gui_initialize_( void );
void ape_gui_shutdown_( void );

const GuiStyleSheet *ape_gui_cache_style_sheet( const char *path );
void                 ape_gui_set_style_sheet( const GuiStyleSheet *styleSheet );
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

/**
 * Creates a new GUI panel with the given parameters.
 *
 * @param parent      The parent panel. Position will be relative to parent.
 * @param x           The x-coordinate of the panel. Absolute if no parent.
 * @param y           The y-coordinate of the panel. Absolute if no parent.
 * @param w           The width of the panel. Absolute if no parent.
 * @param h           The height of the panel. Absolute if no parent.
 * @param background  The background type of the panel.
 * @param border      The border type of the panel.
 * @return            A pointer to the newly created GuiPanel.
 */
GuiPanel *ape_gui_panel_create( GuiPanel *parent, int x, int y, int w, int h, GuiPanelBackground background, GuiPanelBorder border );

/**
 * Destroys the given GuiPanel instance and all its children.
 *
 * This function will first ensure the panel is not NULL and then remove the panel
 * from its parent's child list if it has a parent. It will then recursively destroy
 * all children of the panel and finally destroy the panel's child list and free
 * the panel memory itself.
 *
 * @param self A pointer to the GuiPanel to be destroyed.
 */
void ape_gui_panel_destroy( GuiPanel *self );

void guiSetPanelStyleSheet( GuiPanel *self, const GuiStyleSheet *styleSheet );

void guiDrawPanel( GuiPanel *self );
void guiDrawPanelBackground( GuiPanel *self );
void guiTickPanel( GuiPanel *self );

void     guiSetPanelBackgroundColour( GuiPanel *self, const PLColour *colour );
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

/**
 * Sets the size of the specified GUI panel.
 *
 * Adjusts the width and height of the given `GuiPanel` object to the provided values.
 *
 * @param self 	A pointer to the GuiPanel object to be resized.
 * @param w 	The new width for the panel.
 * @param h 	The new height for the panel.
 */
void gui_panel_set_size( GuiPanel *self, int w, int h );

bool guiIsMouseOverPanel( GuiPanel *self, int mx, int my );

bool guiHandleMousePanelEvent( GuiPanel *self, int mx, int my, int wheel, int button, bool buttonUp );
bool guiHandleKeyboardPanelEvent( GuiPanel *self, int button, bool buttonUp );

/**
 * Sets the visibility state of the specified GuiPanel.
 *
 * This function updates the visibility flag of the GuiPanel, determining
 * whether the panel should be shown or hidden.
 *
 * @param self Pointer to the GuiPanel whose visibility is to be set.
 * @param flag Boolean value indicating the desired visibility state.
 *             If true, the panel will be visible; if false, it will be hidden.
 */
void ape_gui_panel_set_visible( GuiPanel *self, bool flag );

/****************************************
 * Cursor
 ****************************************/

GuiPanel *ape_gui_cursor_create( GuiPanel *parent, int x, int y );
void      guiDestroyCursor( GuiPanel *self );

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

/**
 * Retrieves the line spacing value from the specified GuiFont.
 *
 * @param font 	Pointer to the GuiFont structure from which to get the line spacing.
 * @return 		The line spacing value of the given font.
 */
float gui_font_get_line_spacing( const GuiFont *font );

/**
 * Returns the specified default font.
 */
GuiFont *gui_get_default_font( GuiFontDefaultType defaultType );

void guiDestroyFont( GuiFont *font );

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
GuiFont *gui_font_load( const char *path );

void  guiGetCharacterPixelSize( const GuiFont *font, float scale, uint32_t character, float *dw, float *dh );
float guiGetCharacterPixelWidth( const GuiFont *font, float scale, uint32_t character );

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
void gui_font_get_string_pixel_size( const GuiFont *self, float scale, const char *string, size_t length, float *dw, float *dh );

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
void gui_font_draw_character( const GuiFont *font, float x, float y, float scale, const PLColour *colour, uint32_t character );

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
void gui_font_draw_string( const GuiFont *self, float x, float y, float *ox, float *oy, float scale, const PLColour *colour, const char *string, size_t length, bool shadow );

/**
 * @brief Renders the given bitmap font on the screen.
 *
 * This function sets up the necessary state in the rendering pipeline,
 * configures the appropriate shader program for font rendering, and
 * draws the font's associated mesh.
 *
 * @param font A pointer to a GuiFont structure containing the font's texture, glyphs, and mesh data.
 */
void gui_font_display( GuiFont *font );

/****************************************
 * Desktop
 * A simplified GUI environment for
 * tooling.
 ****************************************/

typedef struct GuiDesktop GuiDesktop;

GuiDesktop *guiCreateDesktop( GuiPanel *parent );
void        guiDestroyDesktop( GuiDesktop *desktop );

PL_EXTERN_C_END
