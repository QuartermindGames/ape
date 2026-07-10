// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Private header for common library

#pragma once

#include "qmos/public/qm_os_memory.h"

#include "aux/public/aux.h"

void com_print_( const char *m, ... );
void com_warning_( const char *m, ... );
void com_error_( const char *m, ... );

void com_pack_pkg_register_( void );
