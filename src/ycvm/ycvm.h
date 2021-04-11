/* ======================================================================
 * Yin C/VM Suite
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================== */

#pragma once

#include <PL/platform.h>
#include <PL/platform_console.h>

extern unsigned int
        LOG_LEVEL_DEFAULT,
        LOG_LEVEL_WARNING,
        LOG_LEVEL_ERROR;

#define Print( ... ) plLogMessage( LOG_LEVEL_DEFAULT, __VA_ARGS__ )
#define Warning( ... ) plLogMessage( LOG_LEVEL_WARNING, __VA_ARGS__ )
#define Error( ... ) plLogMessage( LOG_LEVEL_ERROR, __VA_ARGS__ )
