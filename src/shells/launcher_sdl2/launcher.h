// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_console.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_driver_interface.h>

#include <yin/core_base.h>

extern int launcherLog;

#define Print( ... ) PlLogMessage( launcherLog, __VA_ARGS__ )
#define PrintWarn( ... )                                                            \
	{                                                                               \
		PlLogMessage( launcherLog, __VA_ARGS__ );                                   \
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, __VA_ARGS__ ); \
	}

#ifdef NDEBUG
#	define PrintError( ... )                                                         \
		{                                                                             \
			PlLogMessage( launcherLog, __VA_ARGS__ );                                 \
			ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, __VA_ARGS__ ); \
			exit( EXIT_FAILURE );                                                     \
		}
#else
#	define PrintError( ... )                                                         \
		{                                                                             \
			PlLogMessage( launcherLog, __VA_ARGS__ );                                 \
			ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, __VA_ARGS__ ); \
			abort();                                                                  \
		}
#endif
