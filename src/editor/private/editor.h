// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_console.h>

#include <plgraphics/plg.h>

#include "yin/node.h"
#include "yin/core_renderer.h"

#include "ape/editor_public.h"

typedef enum EdLogLevel
{
	ED_LOG_GENERAL,
	ED_LOG_DEBUG,
	ED_LOG_WARN,
	ED_LOG_ERROR,

	ED_MAX_LOG_LEVELS
} EdLogLevel;

extern int ed_logLevels[ ED_MAX_LOG_LEVELS ];

void edPrint_( EdLogLevel logLevel, const char *message, ... );
void edError_( const char *message, ... );

void edInitializeMaterialSelector_( void );
void edShutdownMaterialSelector_( void );
