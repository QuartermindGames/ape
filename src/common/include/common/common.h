/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include <PL/platform.h>

#if !defined( __bool_true_false_are_defined )
typedef unsigned char bool;
enum { false,
	   true };
#endif

#if defined( COMMON_DLL )
#define COMMON_API PL_EXPORT
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
