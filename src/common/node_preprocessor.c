/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>
#include <PL/pl_parse.h>
#include <PL/platform_filesystem.h>

#include "common/Common.h"
#include "node_private.h"

/**
 * Inserts the given string into an existing string buffer.
 * Automatically reallocs buffer if it doesn't fit.
 * todo: consider cleaning this up and making part of API?
 */
static char *InsertString( const char *string, char **buf, size_t *bufSize, size_t *maxBufSize ) {
	/* check if it's going to fit first */
	size_t strLength = strlen( string );
	size_t originalSize = *bufSize;
	*bufSize += strLength;
	if ( *bufSize >= *maxBufSize ) {
		*maxBufSize = *bufSize + strLength;
		*buf = pl_realloc( *buf, *maxBufSize );
	}

	/* now copy it into our buffer */
	strncpy( *buf + originalSize, string, strLength );

	return *buf + originalSize + strLength;
}

#define MAX_MACROS 512
#define MAX_MACRO_NAME_LENGTH 16
#define MAX_MACRO_LENGTH 1024

typedef struct PreProcessorMacro {
	char name[ MAX_MACRO_NAME_LENGTH ];
	char body[ MAX_MACRO_LENGTH ];
} PreProcessorMacro;

typedef struct PreProcessorContext {
	PreProcessorMacro macros[ MAX_MACROS ];
	unsigned int numMacros;
} PreProcessorContext;

static PreProcessorContext ctx;

static const PreProcessorMacro *GetPreprocessorMacroByName( const char *name ) {
	for ( unsigned int i = 0; i < ctx.numMacros; ++i ) {
		if ( pl_strcasecmp( ctx.macros[ i ].name, name ) == 0 ) {
			return &ctx.macros[ i ];
		}
	}

	return NULL;
}

/**
 * Check if the specified macro exists. Typically going to be
 * used to avoid registering duplicates.
 */
static bool IsMacroRegistered( const char *name ) {
	if ( GetPreprocessorMacroByName( name ) != NULL ) {
		return true;
	}

	return false;
}

char *xNL_PreProcessScript( char *buf, size_t *length, bool isHead ) {
	size_t actualLength = 0;
	size_t maxLength = *length;
	char *dstBuffer = pl_calloc( maxLength, sizeof( char ) );
	char *dstPos = dstBuffer;

	if ( isHead ) {
		memset( &ctx, 0, sizeof( PreProcessorContext ) );
	}

	const char *srcPos = buf;
	char *srcEnd = buf + *length;
	while ( srcPos < srcEnd && *srcPos != '\0' ) {
		if ( *srcPos == ';' ) {
			plSkipLine( &srcPos );
			continue;
		} else if ( *srcPos == '$' ) {
			srcPos++;
			char token[ 32 ];
			plParseToken( &srcPos, token, sizeof( token ) );
			if ( pl_strcasecmp( token, "include" ) == 0 ) {
				plSkipWhitespace( &srcPos );

				/* pull the path - needs to be enclosed otherwise this'll fail */
				char path[ PL_SYSTEM_MAX_PATH ];
				plParseEnclosedString( &srcPos, path, sizeof( path ) );

				PLFile *file = plOpenFile( path, true );
				if ( file != NULL ) {
					/* allocate a temporary buffer */
					size_t includeLength = plGetFileSize( file );
					char *includeBody = pl_malloc( includeLength );
					memcpy( includeBody, plGetFileData( file ), includeLength );

					/* close the current file, to avoid recursively opening files
                     * and hitting any limits */
					plCloseFile( file );

					/* now throw it into the pre-processor */
					includeBody = xNL_PreProcessScript( includeBody, &includeLength, false );

					/* and finally, push it into our destination */
					dstPos = InsertString( includeBody, &dstBuffer, &actualLength, &maxLength );
					pl_free( includeBody );
				} else {
					Warning( "Failed to load include \"%s\": %s\n", path, plGetError() );
				}

				plSkipLine( &srcPos );
				continue;
			} else if ( pl_strcasecmp( token, "insert" ) == 0 ) {
			    plSkipWhitespace( &srcPos );
				plParseToken( &srcPos, token, sizeof( token ) );

				const PreProcessorMacro *macro = GetPreprocessorMacroByName( token );
				if ( macro == NULL ) {
					Warning( "Unknown macro \"%s\" was used!\n", token );
					continue;
				}

				dstPos = InsertString( macro->body, &dstBuffer, &actualLength, &maxLength );
				continue;
			} else if ( pl_strcasecmp( token, "define" ) == 0 ) {
				plSkipWhitespace( &srcPos );

				PreProcessorMacro *macro = &ctx.macros[ ctx.numMacros ];

				/* read in the macro name */
				plParseToken( &srcPos, macro->name, sizeof( macro->name ) );
				plSkipWhitespace( &srcPos );

				/* if it's already registered, skip it */
				if ( IsMacroRegistered( macro->name ) ) {
					Warning( "Macro \"%s\" is being redeclared\n", macro->name );
					if ( *srcPos == '(' ) {
						while ( *srcPos != '\0' && *srcPos != ')' ) srcPos++;
						if ( *srcPos != '\0' ) srcPos++;
					} else {
						plSkipLine( &srcPos );
					}
				} else if ( *srcPos == '(' ) {
					srcPos++;
					/* copy the block into our macro list */
					char *mbody = macro->body;
					while ( *srcPos != ')' && *srcPos != '\0' ) {
						*mbody++ = *srcPos++;
					}
					if ( *srcPos != '\0' ) srcPos++;
				} else {
					plParseToken( &srcPos, macro->body, sizeof( macro->body ) );
				}

				continue;
			} else {
				Warning( "Unknown preprocessor token \"%s\"!\n" );
			}
		}

		/* if we exceed the maximum allowed length... */
		if ( ++actualLength > maxLength ) {
			++maxLength;
			char *oldDstBuffer = dstBuffer;
			dstBuffer = pl_realloc( dstBuffer, maxLength );
			dstPos = dstBuffer + ( dstPos - oldDstBuffer );
		}

		*dstPos++ = *srcPos++;
	}

	/* free the original buffer that was passed in */
	pl_free( buf );

	/* resize and update buf to match */
	*length = actualLength;

	return dstBuffer;
}
