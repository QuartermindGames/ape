// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Private header for common library

#pragma once

#include <plcore/pl_console.h>

#include "common.h"

void com_print_( const char *m, ... );
void com_warning_( const char *m, ... );
void com_error_( const char *m, ... );

void com_pack_pkg_register_( void );
