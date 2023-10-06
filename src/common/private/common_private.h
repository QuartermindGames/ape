// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Private header for common library

#pragma once

#include <plcore/pl_console.h>

#include "common.h"

enum ComLogLevel
{
	COM_LOG_LEVEL_INFO,
	COM_LOG_LEVEL_DEBUG,
	COM_LOG_LEVEL_WARN,
	COM_LOG_LEVEL_ERROR,

	COM_MAX_LOG_LEVELS
};

extern int com_logLevels_[ COM_MAX_LOG_LEVELS ];

#define Message( FORMAT, ... ) PlLogWFunction( com_logLevels_[ COM_LOG_LEVEL_INFO ], FORMAT, ##__VA_ARGS__ )
#define Warning( FORMAT, ... ) PlLogWFunction( com_logLevels_[ COM_LOG_LEVEL_WARN ], FORMAT, ##__VA_ARGS__ )

void com_pack_pkg_register_( void );
void com_pack_vpp_register_( void );
