// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "core/public/core_console.h"

void ape_console_register_commands_( bool isDedicated );
void ape_console_register_variables_( bool isDedicated );

void ape_console_draw_( const ApeViewport *viewport );
void ape_console_register_cl_commands_( void );
void ape_console_register_cl_variables_( void );

void ape_console_print_( const char *message, ... );
void ape_console_verbose_( const char *message, ... );
void ape_console_warning_( const char *message, ... );
void ape_console_error_( bool die, const char *message, ... );

/////////////////////////////////////////////////////////////////////////////////////
// Console Variables
/////////////////////////////////////////////////////////////////////////////////////

void ape_console_var_initialize_();
void ape_console_var_shutdown_();

bool ape_console_var_parse_( const char *name, unsigned int argc, const char *const *argv );

void ape_console_var_find_( const char *term );
bool ape_console_var_help_( const char *name );

/////////////////////////////////////////////////////////////////////////////////////
// Console Commands
/////////////////////////////////////////////////////////////////////////////////////

void ape_console_cmd_initialize_();
void ape_console_cmd_shutdown_();

bool ape_console_cmd_parse_( const char *name, unsigned int argc, const char *const *argv );

void ape_console_cmd_find_( const char *term );
bool ape_console_cmd_help_( const char *name );

/////////////////////////////////////////////////////////////////////////////////////
// Console Log
/////////////////////////////////////////////////////////////////////////////////////

void ape_console_log_initialize_();
void ape_console_log_shutdown_();

void ape_console_push_message_( const char *message, QmMathColour4ub colour );
