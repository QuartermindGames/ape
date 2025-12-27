// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "core/public/core_console.h"

void ape_console_log_initialize_();
void ape_console_log_shutdown_();

void ape_console_push_message_( const char *message, QmMathColour4ub colour );
