/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 OldTimes Software
 * ====================================================================*/

#include "game.h"

OSInterface globalSystem;
EngineInterface globalEngine;

int globalGameLog;
int globalGameDebugLog;
int globalGameWarningLog;
int globalGameErrorLog;

PL_EXPORT GameInterface *GetDllInterface( uint32_t version, const OSInterface *systemInterface, const EngineInterface *engineInterface ) {
	globalGameLog = PlAddLogLevel( "game", PL_COLOUR_GREEN, true );
	globalGameWarningLog = PlAddLogLevel( "game/warning", PL_COLOUR_ORANGE, true );
	globalGameErrorLog = PlAddLogLevel( "game/error", PL_COLOUR_RED, true );
	globalGameDebugLog = PlAddLogLevel( "game/debug", PL_COLOUR_WHITE,
#if !defined( NDEBUG )
	                                    true
#else
	                                    false
#endif
	);
}
