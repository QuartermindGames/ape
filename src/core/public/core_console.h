// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmmath/public/qm_math_colour.h"

enum
{
	QM_OS_BIT_FLAG( APE_CONSOLE_VAR_FLAG_ARCHIVE, 0 ),
	QM_OS_BIT_FLAG( APE_CONSOLE_VAR_FLAG_CHEAT, 1 ),
};

void ape_console_var_register( const char *name, const char *desc, const char *value, PLVariableType type, void *ptr, void ( *callback )( PLConsoleVariable * ), unsigned int flags );

int  ape_console_log_register_input( const char *prefix, QmMathColour4ub colour, bool isActive );
void ape_console_log_push_message( int id, const char *msg, ... );
