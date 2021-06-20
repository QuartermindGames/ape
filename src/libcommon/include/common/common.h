/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

#include "public/SharedBase.h"

#if defined( COMMON_DLL )
#include <plcore/pl_console.h>

#include <assert.h>

#define COMMON_API PL_EXPORT

extern int logLevelPrint;
extern int logLevelWarn;
#define Message( FORMAT, ... ) PlLogWFunction( logLevelPrint, FORMAT, ##__VA_ARGS__ )
#define Warning( FORMAT, ... ) PlLogWFunction( logLevelWarn, FORMAT, ##__VA_ARGS__ )
#else
#define COMMON_API PL_IMPORT
#endif

#define MAGIC_TO_NUM( A, B, C, D ) ( ( ( D ) << 24 ) + ( ( C ) << 16 ) + ( ( B ) << 8 ) + ( A ) )

#define CVar( NAME, STORE )                           \
	static PLConsoleVariable *( STORE ) = NULL;       \
	if ( ( STORE ) == NULL )                          \
	{                                                 \
		( STORE ) = PlGetConsoleVariable( ( NAME ) ); \
		u_assert( ( STORE ) != NULL );                \
	}

#define COM_FMT_float   "%f"
#define COM_FMT_double  "%lf"
#define COM_FMT_int16   "%hd"
#define COM_FMT_uint16  "%hu"
#define COM_FMT_int32   "%d"
#define COM_FMT_uint32  "%u"
#define COM_FMT_int64   "%ld"
#define COM_FMT_uint64  "%lu"
#define COM_FMT_hex     "%x"
#define COM_FMT_string  "%s"
#define COM_FMT_address "%p"

PL_EXTERN_C

extern void ComLib_Initialize( void );

extern const char *ComFS_GetDataDirectory( void );

PL_EXTERN_C_END
