// SPDX-License-Identifier: MIT
// Ape Config Markup
// Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

//#define ACM_TEST

#include <plcore/pl_parse.h>
#if defined( ACM_TEST )
#	include <plcore/pl_timer.h>
#endif

#include "acm_private.h"

typedef struct AcmLexerReservedWord
{
	const char  *string;
	AcmTokenType type;
} AcmLexerReservedWord;

static AcmLexerReservedWord reservedWords[] = {
        {"string",  ACM_TOKEN_TYPE_TYPENAME     },
        {"bool",    ACM_TOKEN_TYPE_TYPENAME     },
        {"object",  ACM_TOKEN_TYPE_TYPENAME     },
        {"array",   ACM_TOKEN_TYPE_TYPENAME     },
        {"uint8",   ACM_TOKEN_TYPE_TYPENAME     },
        {"uint16",  ACM_TOKEN_TYPE_TYPENAME     },
        {"uint32",  ACM_TOKEN_TYPE_TYPENAME     },
        {"uint",    ACM_TOKEN_TYPE_TYPENAME     }, // shorthand uint32
        {"uint64",  ACM_TOKEN_TYPE_TYPENAME     },
        {"int8",    ACM_TOKEN_TYPE_TYPENAME     },
        {"int16",   ACM_TOKEN_TYPE_TYPENAME     },
        {"int32",   ACM_TOKEN_TYPE_TYPENAME     },
        {"int",     ACM_TOKEN_TYPE_TYPENAME     }, // shorthand int32
        {"int64",   ACM_TOKEN_TYPE_TYPENAME     },
        {"float",   ACM_TOKEN_TYPE_TYPENAME     },
        {"float64", ACM_TOKEN_TYPE_TYPENAME     },

        {"{",       ACM_TOKEN_TYPE_OPEN_BRACKET },
        {"}",       ACM_TOKEN_TYPE_CLOSE_BRACKET},
};
static const unsigned int NUM_RESERVED_WORDS = PL_ARRAY_ELEMENTS( reservedWords );

static AcmTokenType get_token_type_for_symbol( const char *symbol )
{
	if ( *symbol == '\0' )
	{
		return ACM_TOKEN_TYPE_EOF;
	}

	for ( unsigned int i = 0; i < NUM_RESERVED_WORDS; ++i )
	{
		const char *p = reservedWords[ i ].string;
		if ( strcmp( p, symbol ) != 0 )
		{
			continue;
		}

		return reservedWords[ i ].type;
	}

	return ACM_TOKEN_TYPE_IDENTIFIER;
}

static void parse_line( const char *p, const char *file, unsigned int lineNum, PLLinkedList *list )
{
	const char *o = p;
	while ( true )
	{
		PlSkipWhitespace( &p );

		if ( *p == '\0' )
		{
			break;
		}
		else if ( *p == ';' )
		{
			if ( *( p + 1 ) == '*' )
			{
				// multi-line comment
				p += 2;
				while ( *p != '*' && *( p + 1 ) != ';' )
				{
					int l = PlGetLineEndType( p );
					if ( l != PL_PARSE_NL_INVALID )
					{
						p += l;
						continue;
					}

					p++;
				}
				p += 2;
				continue;
			}
			else
			{
				// single-line comment
				break;
			}
		}

		AcmTokenType type = ACM_TOKEN_TYPE_INVALID;

		unsigned int linePos = ( p - o ) + 1;
		unsigned int symbolSize;
		char        *symbol;
		if ( *p == '\"' )
		{
			symbolSize = PlDetermineEnclosedStringLength( p ) + 1;
			symbol     = PL_NEW_( char, symbolSize );
			if ( PlParseEnclosedString( &p, symbol, symbolSize ) == NULL )
			{
				Warning( "Failed to parse enclosed string: %u:%u\n", lineNum, linePos );
				break;
			}

			type = ACM_TOKEN_TYPE_STRING;
		}
		else
		{
			symbolSize = PlDetermineTokenLength( p ) + 1;
			symbol     = PL_NEW_( char, symbolSize );
			if ( PlParseToken( &p, symbol, symbolSize ) == NULL )
			{
				Warning( "Failed to parse token for lexer: %u:%u\n", lineNum, linePos );
				break;
			}

			if ( isdigit( *symbol ) || *symbol == '-' )
			{
				type = ACM_TOKEN_TYPE_INTEGER;
				for ( unsigned int i = 0; i < symbolSize; ++i )
				{
					if ( symbol[ i ] == '.' )
					{
						if ( type == ACM_TOKEN_TYPE_DECIMAL )
						{
							Warning( "Unexpected token in num: %u:%u\n", lineNum, linePos );
							break;
						}

						type = ACM_TOKEN_TYPE_DECIMAL;
					}
				}
			}
			else
			{
				type = get_token_type_for_symbol( symbol );
			}
		}

		if ( type != ACM_TOKEN_TYPE_INVALID )
		{
			AcmLexerToken *token = PL_NEW( AcmLexerToken );
			snprintf( token->symbol, sizeof( token->symbol ), "%s", symbol );
			snprintf( token->path, sizeof( token->path ), "%s", file );//TODO: can be simplified!!!
			token->lineNum = lineNum;
			token->linePos = linePos;
			token->type    = type;
			token->node    = PlInsertLinkedListNode( list, token );

			PL_DELETE( symbol );
		}
		else
		{
			Warning( "Unexpected character: %u:%u\n", lineNum, linePos );
		}
	}
}

AcmLexer *acm_lexer_parse_buffer_( AcmLexer *self, const char *buf, const char *file )
{
	if ( self == NULL )
	{
		self         = PL_NEW( AcmLexer );
		self->tokens = PlCreateLinkedList();
		snprintf( self->originPath, sizeof( self->originPath ), "%s", file );
	}

#if defined( ACM_TEST )
	double startTime = PlGetCurrentSeconds();
#endif

	unsigned int curLineNum = 0;
	const char  *p          = buf;
	while ( *p != '\0' )
	{
		curLineNum++;

		if ( *p == ';' )
		{
			if ( *( p + 1 ) == '*' )
			{
				// multi-line comment
				p += 2;
				while ( *p != '*' && *( p + 1 ) != ';' )
				{
					int l = PlGetLineEndType( p );
					if ( l != PL_PARSE_NL_INVALID )
					{
						p += l;
						continue;
					}

					p++;
				}
				p += 2;
				continue;
			}

			// single-line comment
			PlSkipLine( &p );
			continue;
		}

		// tokenise the line
		unsigned int bufSize = PlDetermineLineLength( p ) + 1;
		if ( bufSize > 1 )
		{
			char *line = PL_NEW_( char, bufSize );
			PlParseLine( &p, line, bufSize );

			parse_line( line, file, curLineNum, self->tokens );

			PL_DELETE( line );
		}
		else
		{
			PlSkipLine( &p );
		}
	}

#if defined( ACM_TEST )
	// output the result from the lexer
	printf( "%5s %20s %10s %10s\n", "TYPE", "SYMBOL", "LINE", "LPOS" );
	const AcmLexerToken *token;
	PL_ITERATE_LINKED_LIST( token, AcmLexerToken, self->tokens, i )
	{
		printf( "%5d %20s %10u %10u\n", token->type, token->symbol, token->lineNum, token->linePos );
	}

	double endTime = PlGetCurrentSeconds();
	printf( "Lexer took " PL_FMT_double "s for \"%s\"\n", endTime - startTime, file );
#endif

	return self;
}
