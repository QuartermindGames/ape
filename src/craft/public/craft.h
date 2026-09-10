// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmos/public/qm_os.h"

QM_OS_EXTERN_C

static constexpr char    CRAFT_TITLE[]     = "Craft";
static constexpr char    CRAFT_TITLE_APP[] = "craft";
static constexpr uint8_t CRAFT_VERSION[]   = { 0, 0, 1 };

/**
 * Attempts to initialize the Craft library.
 * Returns false on fail, or true on success.
 */
bool craft_initialize( int argc, char **argv );

/**
 * Will run the event loop for any UI.
 */
void craft_process_events();

/**
 * Shows a basic on-screen prompt for a warning.
 */
void craft_prompt_warning( const char *msg, ... );

/**
 * Shows a basic on-screen prompt for an error, and then bails.
 */
void craft_prompt_error( const char *msg, ... );

QM_OS_EXTERN_C_END
