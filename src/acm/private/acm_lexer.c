// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_parse.h>

#include "acm_private.h"

typedef enum AcmTokenType
{
	ACM_TOKEN_TYPE_INVALID,

	ACM_TOKEN_TYPE_EOF,

	ACM_TOKEN_TYPE_TYPENAME,
	ACM_TOKEN_TYPE_IDENTIFIER,
	ACM_TOKEN_TYPE_STRING,

	ACM_TOKEN_TYPE_OPEN_BRACKET, // {
	ACM_TOKEN_TYPE_CLOSE_BRACKET,// }

	ACM_TOKEN_TYPE_SEMICOLON,// ;
} AcmTokenType;

#define ACM_MAX_SYMBOL_LENGTH 128
typedef char AcmSymbolName[ ACM_MAX_SYMBOL_LENGTH ];

typedef struct AcmLexerToken
{
	AcmSymbolName     symbol;
	AcmTokenType      type;
	PLPath            path;
	unsigned int      lineNum;
	unsigned int      linePos;
	PLLinkedListNode *node;
} AcmLexerToken;

typedef struct AcmLexer
{
	PLPath        originPath;
	PLLinkedList *tokens;
} AcmLexer;

typedef struct AcmLexerReservedWord
{
	const char  *string;
	AcmTokenType type;
} AcmLexerReservedWord;

static AcmLexerReservedWord reservedWords[] = {
        {"string",   ACM_TOKEN_TYPE_TYPENAME     },
        { "bool",    ACM_TOKEN_TYPE_TYPENAME     },
        { "object",  ACM_TOKEN_TYPE_TYPENAME     },
        { "array",   ACM_TOKEN_TYPE_TYPENAME     },
        { "uint8",   ACM_TOKEN_TYPE_TYPENAME     },
        { "uint16",  ACM_TOKEN_TYPE_TYPENAME     },
        { "uint32",  ACM_TOKEN_TYPE_TYPENAME     },
        { "uint",    ACM_TOKEN_TYPE_TYPENAME     }, // shorthand uint32
        { "uint64",  ACM_TOKEN_TYPE_TYPENAME     },
        { "int8",    ACM_TOKEN_TYPE_TYPENAME     },
        { "int16",   ACM_TOKEN_TYPE_TYPENAME     },
        { "int32",   ACM_TOKEN_TYPE_TYPENAME     },
        { "int",     ACM_TOKEN_TYPE_TYPENAME     }, // shorthand int32
        { "int64",   ACM_TOKEN_TYPE_TYPENAME     },
        { "float",   ACM_TOKEN_TYPE_TYPENAME     },
        { "float64", ACM_TOKEN_TYPE_TYPENAME     },

        { "{",       ACM_TOKEN_TYPE_OPEN_BRACKET },
        { "}",       ACM_TOKEN_TYPE_CLOSE_BRACKET},

        { ";",       ACM_TOKEN_TYPE_SEMICOLON    },
};
static const unsigned int NUM_RESERVED_WORDS = PL_ARRAY_ELEMENTS( reservedWords );

static AcmTokenType get_token_type_for_symbol( const char *symbol, unsigned int *length )
{
	if ( *symbol == '\0' )
	{
		return ACM_TOKEN_TYPE_EOF;
	}

	for ( unsigned int i = 0; i < NUM_RESERVED_WORDS; ++i )
	{
		const char *p = reservedWords[ i ].string;
		if ( strncmp( p, symbol, strlen( p ) ) != 0 )
		{
			continue;
		}

		if ( length != NULL )
		{
			*length = strlen( p );
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
	}
}

void acm_test_lexer( void )
{
	static const char *longString = "";//todo
}
