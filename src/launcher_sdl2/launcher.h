/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_console.h>

#include "public/SharedBase.h"

extern int launcherLog;

void Sys_DisplayMessageBox( SysMessage messageType, const char *message, ... );

#define Print( ... ) PlLogMessage( launcherLog, __VA_ARGS__ )
#define PrintWarn( ... )                                           \
	{                                                              \
		PlLogMessage( launcherLog, __VA_ARGS__ );                  \
		Sys_DisplayMessageBox( SYS_MESSAGE_WARNING, __VA_ARGS__ ); \
	}
#define PrintError( ... )                                        \
	{                                                            \
		PlLogMessage( launcherLog, __VA_ARGS__ );                \
		Sys_DisplayMessageBox( SYS_MESSAGE_ERROR, __VA_ARGS__ ); \
		exit( EXIT_FAILURE );                                    \
	}
