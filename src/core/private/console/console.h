// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "core/public/core_console.h"

void ape_console_log_initialize_();
void ape_console_log_shutdown_();

void ape_console_push_message_( const char *message, QmMathColour4ub colour );
