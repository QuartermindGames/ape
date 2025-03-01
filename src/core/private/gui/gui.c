// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <acm/acm.h>

#include "gui_private.h"

ApeGUIState ape_guiState_;

bool ape_gui_initialize_fonts_( void );
void ape_gui_initialize_draw_( void );

/**
 * Initialize the GUI sub-system.
 */
bool ape_gui_initialize_( void )
{
	PL_ZERO_( ape_guiState_ );

	ape_gui_initialize_draw_();
	if ( !ape_gui_initialize_fonts_() )
	{
		ape_warning_( "Font initialization failed!\n" );
		return false;
	}

	ape_print_( "GUI initialized!\n" );
	return true;
}

void ape_gui_shutdown_( void )
{
	guiShutdownDraw_();
}

void ape_gui_update_mouse_position_( int x, int y )
{
	ape_guiState_.mouseOldPos = ape_guiState_.mousePos;
	ape_guiState_.mousePos.x  = x;
	ape_guiState_.mousePos.y  = y;
}

void gui_update_mouse_wheel( float x, float y )
{
	ape_guiState_.mouseOldWheel = ape_guiState_.mouseWheel;
	ape_guiState_.mouseWheel.x  = x;
	ape_guiState_.mouseWheel.y  = y;
}

void guiUpdateMouseButton( GuiMouseButton button, bool isDown )
{
}
