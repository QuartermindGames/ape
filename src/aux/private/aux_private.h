// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Private header for common library

#pragma once

#include "qmos/public/qm_os_memory.h"
#include "qmos/public/qm_os_string.h"

#include "aux/public/aux.h"

/**
 * Attempts to initialize the log system. This only really matters for debug builds.
 * If it returns false, this suggests the log destination failed to open.
 * (you can continue pushing messages anyway in such a case)
 */
bool aux_log_initialize_();

/**
 * Shuts down the log system. This only really matters for debug builds.
 */
void aux_log_shutdown_();

void com_print_( const char *m, ... );
void com_warning_( const char *m, ... );
void com_error_( const char *m, ... );

void com_pack_pkg_register_();
