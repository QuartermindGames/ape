/* ======================================================================
 * Yin C/VM Suite
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================== */

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_console.h>

extern unsigned int LOG_LEVEL_DEFAULT;
extern unsigned int LOG_LEVEL_WARNING;
extern unsigned int LOG_LEVEL_ERROR;

#define Print( ... )   PlLogMessage( LOG_LEVEL_DEFAULT, __VA_ARGS__ )
#define Warning( ... ) PlLogMessage( LOG_LEVEL_WARNING, __VA_ARGS__ )
#define Error( ... )   PlLogMessage( LOG_LEVEL_ERROR, __VA_ARGS__ )
