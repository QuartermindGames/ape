// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor.h"

int ed_logLevels[ ED_MAX_LOG_LEVELS ];

void edInitialize( void )
{
	ed_logLevels[ ED_LOG_GENERAL ] = PlAddLogLevel( "ed/print", PL_COLOUR_ALICE_BLUE, true );
	ed_logLevels[ ED_LOG_WARN ]    = PlAddLogLevel( "ed/warning", PL_COLOUR_ORANGE_RED, true );
	ed_logLevels[ ED_LOG_ERROR ]   = PlAddLogLevel( "ed/error", PL_COLOUR_ORANGE_RED, true );
	ed_logLevels[ ED_LOG_DEBUG ]   = PlAddLogLevel( "ed/debug", PL_COLOUR_ALICE_BLUE,
#if !defined( NDEBUG )
	                                              true
#else
	                                              false
#endif
	);

	edInitializeMaterialSelector_();
}

void edRegisterConsoleVariables( void )
{
}

void edShutdown( void )
{
	edShutdownMaterialSelector_();
}

void edTick( void )
{
}

void edDraw( void )
{
}

void edPrint_( EdLogLevel logLevel, const char *message, ... )
{
	va_list args;
	va_start( args, message );

	int length = pl_vscprintf( message, args ) + 1;
	if ( length <= 0 )
	{
		return;
	}

	char *buf = PL_NEW_( char, length );
	vsnprintf( buf, length, message, args );

	va_end( args );

	PlLogMessage( ed_logLevels[ logLevel ], buf );

	PL_DELETE( buf );
}

void edError_( const char *message, ... )
{
	va_list args;
	va_start( args, message );

	int length = pl_vscprintf( message, args ) + 1;
	if ( length > 0 )
	{
		char *buf = PL_NEW_( char, length );
		vsnprintf( buf, length, message, args );

		va_end( args );

		PlLogMessage( ED_LOG_ERROR, buf );

		PL_DELETE( buf );
	}

	//TODO: error out properly...

	abort();
}
