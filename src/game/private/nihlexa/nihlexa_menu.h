// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "../menu/menu.h"

#include "nihlexa.h"

void nih_menu_initialize_( void );
void nih_menu_shutdown_();
void nih_menu_tick( double delta );
void nih_menu_draw( const ApeViewport *viewport );
