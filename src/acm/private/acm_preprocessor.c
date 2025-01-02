// SPDX-License-Identifier: MIT
// Ape Config Markup
// Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

#if 0//unused...

#	include <plcore/pl_parse.h>
#	include <plcore/pl_filesystem.h>

#	include "acm_private.h"

#	define MAX_MACROS            512
#	define MAX_MACRO_NAME_LENGTH 16
#	define MAX_MACRO_LENGTH      1024

typedef struct PreProcessorMacro
{
	char name[ MAX_MACRO_NAME_LENGTH ];
	char body[ MAX_MACRO_LENGTH ];
} PreProcessorMacro;

typedef struct PreProcessorContext
{
	PreProcessorMacro macros[ MAX_MACROS ];
	unsigned int numMacros;
} PreProcessorContext;

static PreProcessorContext ctx;

static const PreProcessorMacro *GetPreprocessorMacroByName( const char *name )
{
	for ( unsigned int i = 0; i < ctx.numMacros; ++i )
		if ( pl_strcasecmp( ctx.macros[ i ].name, name ) == 0 )
			return &ctx.macros[ i ];

	return NULL;
}

/**
 * Check if the specified macro exists. Typically going to be
 * used to avoid registering duplicates.
 */
static bool IsMacroRegistered( const char *name )
{
	if ( GetPreprocessorMacroByName( name ) != NULL )
		return true;

	return false;
}

char *acm_preprocess_script_( char *buf, size_t *length, bool isHead )
{
	size_t actualLength = 0;
	size_t maxLength = *length;
	char *dstBuffer = PlCAllocA( maxLength, sizeof( char ) );
	char *dstPos = dstBuffer;

	if ( isHead )
		PL_ZERO_( ctx );

	const char *srcPos = buf;
	char *srcEnd = buf + *length;
	while ( srcPos < srcEnd && *srcPos != '\0' )
	{
		if ( *srcPos == ';' )
		{
			PlSkipLine( &srcPos );
			continue;
		}
		else if ( *srcPos == '$' )
		{
			srcPos++;
			char token[ 32 ];
			PlParseToken( &srcPos, token, sizeof( token ) );
			if ( pl_strcasecmp( token, "include" ) == 0 )
			{
				PlSkipWhitespace( &srcPos );

				/* pull the path - needs to be enclosed otherwise this'll fail */
				char path[ PL_SYSTEM_MAX_PATH ];
				PlParseEnclosedString( &srcPos, path, sizeof( path ) );

				PLFile *file = PlOpenFile( path, true );
				if ( file != NULL )
				{
					/* allocate a temporary buffer */
					size_t includeLength = PlGetFileSize( file );
					char *includeBody = PlMAlloc( includeLength, true );
					memcpy( includeBody, PlGetFileData( file ), includeLength );

					/* close the current file, to avoid recursively opening files
                     * and hitting any limits */
					PlCloseFile( file );

					/* now throw it into the pre-processor */
					includeBody = acm_preprocess_script_( includeBody, &includeLength, false );

					/* and finally, push it into our destination */
					dstPos = pl_strinsert( includeBody, &dstBuffer, &actualLength, &maxLength );
					PlFree( includeBody );
				}
				else
					Warning( "Failed to load include \"%s\": %s\n", path, PlGetError() );

				PlSkipLine( &srcPos );
				continue;
			}
			else if ( pl_strcasecmp( token, "insert" ) == 0 )
			{
				PlSkipWhitespace( &srcPos );
				PlParseToken( &srcPos, token, sizeof( token ) );

				const PreProcessorMacro *macro = GetPreprocessorMacroByName( token );
				if ( macro == NULL )
				{
					Warning( "Unknown macro \"%s\" was used!\n", token );
					continue;
				}

				dstPos = pl_strinsert( macro->body, &dstBuffer, &actualLength, &maxLength );
				continue;
			}
			else if ( pl_strcasecmp( token, "define" ) == 0 )
			{
				PlSkipWhitespace( &srcPos );

				PreProcessorMacro *macro = &ctx.macros[ ctx.numMacros ];

				/* read in the macro name */
				PlParseToken( &srcPos, macro->name, sizeof( macro->name ) );
				PlSkipWhitespace( &srcPos );

				/* if it's already registered, skip it */
				if ( IsMacroRegistered( macro->name ) )
				{
					Warning( "Macro \"%s\" is being redeclared\n", macro->name );
					if ( *srcPos == '(' )
					{
						while ( *srcPos != '\0' && *srcPos != ')' ) srcPos++;
						if ( *srcPos != '\0' ) srcPos++;
					}
					else
						PlSkipLine( &srcPos );
				}
				else if ( *srcPos == '(' )
				{
					srcPos++;
					/* copy the block into our macro list */
					char *mbody = macro->body;
					while ( *srcPos != ')' && *srcPos != '\0' )
						*mbody++ = *srcPos++;

					if ( *srcPos != '\0' ) srcPos++;
				}
				else
					PlParseToken( &srcPos, macro->body, sizeof( macro->body ) );

				continue;
			}
			else
				Warning( "Unknown preprocessor token \"%s\"!\n", token );
		}

		/* if we exceed the maximum allowed length... */
		if ( ++actualLength > maxLength )
		{
			++maxLength;
			char *oldDstBuffer = dstBuffer;
			dstBuffer = PlReAllocA( dstBuffer, maxLength );
			dstPos = dstBuffer + ( dstPos - oldDstBuffer );
		}

		*dstPos++ = *srcPos++;
	}

	/* free the original buffer that was passed in */
	PlFree( buf );

	/* resize and update buf to match */
	*length = actualLength;

	return dstBuffer;
}

#endif
