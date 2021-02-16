/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include <PL/platform.h>

#if defined( COMMON_DLL )
#include <PL/platform_console.h>

#define COMMON_API PL_EXPORT

extern int logLevelPrint;
extern int logLevelWarn;
#define Message( ... ) plLogMessage( logLevelPrint, __VA_ARGS__ )
#define Warning( ... ) plLogMessage( logLevelWarn, __VA_ARGS__ )
#else
#define COMMON_API PL_IMPORT
#endif

#define MAGIC_TO_NUM( A, B, C, D ) ( ( D << 24 ) + ( C << 16 ) + ( B << 8 ) + A )

#define CVar( NAME, STORE )                           \
	static PLConsoleVariable *( STORE ) = NULL;       \
	if ( ( STORE ) == NULL ) {                        \
		( STORE ) = plGetConsoleVariable( ( NAME ) ); \
		u_assert( ( STORE ) != NULL );                \
	}                                                 \
	( STORE )

extern void CommonLibrary_Initialize( void );
