// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmos/public/qm_os_memory.h"

#include <plcore/pl.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_driver_interface.h>

#include <yin/core_base.h>

extern int launcherLog;

#define Print( ... ) ape_console_log_push_message( launcherLog, __VA_ARGS__ )
#define PrintWarn( ... )                                                         \
	{                                                                            \
		ape_console_log_push_message( launcherLog, __VA_ARGS__ );                \
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, __VA_ARGS__ ); \
	}

#ifdef NDEBUG
#	define PrintError( ... )                                                      \
		{                                                                          \
			ape_console_log_push_message( launcherLog, __VA_ARGS__ );              \
			shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, __VA_ARGS__ ); \
			exit( EXIT_FAILURE );                                                  \
		}
#else
#	define PrintError( ... )                                                      \
		{                                                                          \
			ape_console_log_push_message( launcherLog, __VA_ARGS__ );              \
			shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, __VA_ARGS__ ); \
			abort();                                                               \
		}
#endif
