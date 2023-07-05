// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Public header for common library

#pragma once

typedef enum CmnDataType
{
	CMN_DATATYPE_BOOL,

	CMN_DATATYPE_INT8,
	CMN_DATATYPE_INT16,
	CMN_DATATYPE_INT32,

	CMN_DATATYPE_UINT8,
	CMN_DATATYPE_UINT16,
	CMN_DATATYPE_UINT32,

	CMN_DATATYPE_FLOAT32,
	CMN_DATATYPE_FLOAT64,

	CMN_DATATYPE_POINTER,

	CMN_MAX_DATATYPES
} CmnDataType;

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

#if defined( COMMON_DLL )
#	include "kernel/plcore/include/plcore/pl_console.h"

#	include <assert.h>

extern int logLevelPrint;
extern int logLevelWarn;
#	define Message( FORMAT, ... ) PlLogWFunction( logLevelPrint, FORMAT, ##__VA_ARGS__ )
#	define Warning( FORMAT, ... ) PlLogWFunction( logLevelWarn, FORMAT, ##__VA_ARGS__ )
#endif

PL_EXTERN_C

void cmnInitialize( void );
const char *cmnGetDataDirectory( void );
const char *cmnGetAppDataDirectory( void );
struct NdBranch *cmnGetConfig( const char *name );// attempts to fetch the specified config, otherwise returns an empty config
bool cmnWriteConfig( struct NdBranch *root, const char *name );

void cmnPkg_WriteHeader( FILE *pack, unsigned int numFiles );
void cmnPkg_AddData( FILE *pack, const char *path, const void *buf, size_t size );

PL_EXTERN_C_END
