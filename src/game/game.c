/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "game.h"

OSSystemInterface globalSystem;
OSEngineInterface globalEngine;

int globalGameLog;
int globalGameDebugLog;
int globalGameWarningLog;
int globalGameErrorLog;

bool OSGameDll_Initialize( void )
{
	return true;
}

void OSGameDll_PlayerConnected( const char *name, unsigned int id )
{
}

void OSGameDll_PlayerDisconnected( unsigned int id ) {}

PL_EXPORT GameInterface *GetDllInterface( uint32_t version, const OSSystemInterface *systemInterface, const OSEngineInterface *engineInterface )
{
	globalGameLog        = PlAddLogLevel( "game", PL_COLOUR_GREEN, true );
	globalGameWarningLog = PlAddLogLevel( "game/warning", PL_COLOUR_ORANGE, true );
	globalGameErrorLog   = PlAddLogLevel( "game/error", PL_COLOUR_RED, true );
	globalGameDebugLog   = PlAddLogLevel( "game/debug", PL_COLOUR_WHITE,
#if !defined( NDEBUG )
	                                    true
#else
	                                    false
#endif
	);

	static GameInterface gameInterface = {
	        .version[ 0 ]       = GAME_INTERFACE_VERSION_MAJOR,
	        .version[ 1 ]       = GAME_INTERFACE_VERSION_MINOR,
	        .Initialize         = OSGameDll_Initialize,
	        .PlayerConnected    = OSGameDll_PlayerConnected,
	        .PlayerDisconnected = OSGameDll_PlayerDisconnected,
	};

	return &gameInterface;
}
