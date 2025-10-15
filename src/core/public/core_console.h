// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "qmmath/public/qm_math_colour.h"

int  ape_console_log_register_input( const char *prefix, const QmMathColour4ub colour, const bool isActive );
void ape_console_log_push_message( const int id, const char *msg, ... );
