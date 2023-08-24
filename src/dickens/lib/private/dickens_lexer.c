/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_parse.h>
#include <plcore/pl_linkedlist.h>

#include "dickens_private.h"

/*--------------------------------------------------------------------------------------
 *  Lexer.
 *------------------------------------------------------------------------------------*/

typedef struct LexerReservedWord {
	const char *string;
	DKTokenType type;
	const char *description;
} LexerReservedWord;

static LexerReservedWord tokenCompareTable[] = {
        {"float",    DK_TOKENTYPE_TYPENAME  },
        { "char",    DK_TOKENTYPE_TYPENAME  },
        { "uchar",   DK_TOKENTYPE_TYPENAME  },
        { "short",   DK_TOKENTYPE_TYPENAME  },
        { "ushort",  DK_TOKENTYPE_TYPENAME  },
        { "int",     DK_TOKENTYPE_TYPENAME  },
        { "uint",    DK_TOKENTYPE_TYPENAME  },
        { "string",  DK_TOKENTYPE_TYPENAME  },
        { "void",    DK_TOKENTYPE_TYPENAME  },

        { "if",      DK_TOKENTYPE_IF        },
        { "then",    DK_TOKENTYPE_THEN      },
        { "else",    DK_TOKENTYPE_ELSE      },

        { "do",      DK_TOKENTYPE_DO        },
        { "end",     DK_TOKENTYPE_END       },

        { "decl",    DK_TOKENTYPE_DECLARE   },
        { "struct",  DK_TOKENTYPE_STRUCTURE },
        { "union",   DK_TOKENTYPE_UNION     },
        { "label",   DK_TOKENTYPE_LABEL     },
        { "const",   DK_TOKENTYPE_CONST     },
        { "init",    DK_TOKENTYPE_INITIAL   },
        { "typedef", DK_TOKENTYPE_TYPEDEF   },
        { "proc",    DK_TOKENTYPE_PROCEDURE },
        { "extern",  DK_TOKENTYPE_EXTERNAL  },
        { "public",  DK_TOKENTYPE_PUBLIC    },
        { "native",  DK_TOKENTYPE_NATIVE    },
        { "at",      DK_TOKENTYPE_AT        },

        { "goto",    DK_TOKENTYPE_GOTO      },
        { "by",      DK_TOKENTYPE_BY        },
        { "case",    DK_TOKENTYPE_CASE      },
        { "while",   DK_TOKENTYPE_WHILE     },
        { "leave",   DK_TOKENTYPE_LEAVE     },
        { "iterate", DK_TOKENTYPE_ITERATE   },

        { "call",    DK_TOKENTYPE_CALL      },
        { "return",  DK_TOKENTYPE_RETURN    },
        { "halt",    DK_TOKENTYPE_HALT      },

        { "or",      DK_TOKENTYPE_OR        },
        { "and",     DK_TOKENTYPE_AND       },
        { "xor",     DK_TOKENTYPE_XOR       },
        { "not",     DK_TOKENTYPE_NOT       },

        { ":=",      DK_TOKENTYPE_ASSIGN    },

        { "<>",      DK_TOKENTYPE_NOTEQ     },
        { "<=",      DK_TOKENTYPE_LTEQ      },
        { ">=",      DK_TOKENTYPE_GTEQ      },

        { ":",       DK_TOKENTYPE_COLON     },

        { "=",       DK_TOKENTYPE_EQ        },
        { "%",       DK_TOKENTYPE_MOD       },
        { "+",       DK_TOKENTYPE_PLUS      },
        { "-",       DK_TOKENTYPE_MINUS     },
        { "*",       DK_TOKENTYPE_MUL       },

        { "/",       DK_TOKENTYPE_SLASH     },

        { "<",       DK_TOKENTYPE_LT        },
        { ">",       DK_TOKENTYPE_GT        },

        { "@",       DK_TOKENTYPE_REFERENCE },

        { ";",       DK_TOKENTYPE_SEMICOLON },
        { ",",       DK_TOKENTYPE_COMMA     },
        { "'",       DK_TOKENTYPE_QUOTE     },
        { ".",       DK_TOKENTYPE_STOP      },
        { "(",       DK_TOKENTYPE_LEFTBRACE },
        { ")",       DK_TOKENTYPE_RIGHTBRACE},

        { "|",       DK_TOKENTYPE_OR        },
        { "&",       DK_TOKENTYPE_AND       },
        { "^",       DK_TOKENTYPE_XOR       },
        { "!",       DK_TOKENTYPE_NOT       },

        { "$",       DK_TOKENTYPE_DOLLAR    },
};
static const unsigned int reservedTableElements = PL_ARRAY_ELEMENTS( tokenCompareTable );

/**
 * Determine the type based on the given symbol.
 */
static DKTokenType GetTokenTypeForSymbol( const char *symbol, unsigned int *length ) {
	if ( *symbol == '\0' )
		return DK_TOKENTYPE_EOF;

	for ( unsigned int i = 0; i < reservedTableElements; ++i ) {
		const char *p = tokenCompareTable[ i ].string;
		if ( strncmp( p, symbol, strlen( p ) ) != 0 )
			continue;

		if ( length != NULL )
			*length = ( unsigned int ) strlen( tokenCompareTable[ i ].string );

		return tokenCompareTable[ i ].type;
	}

	return DK_TOKENTYPE_IDENT;
}

void DKLexer_ParseLine( const char *p, const char *file, unsigned int lineNum, PLLinkedList *list ) {
	const char *o = p;
	while ( true ) {
		PlSkipWhitespace( &p );

		DKLexerToken *token = PlMAllocA( sizeof( DKLexerToken ) );

		token->lineNum = lineNum;
		token->linePos = ( p - o ) + 1;
		snprintf( token->path, sizeof( token->path ), "%s", file );

		if ( isalpha( *p ) || *p == '_' ) {
			int i = 0;
			while ( isalpha( *p ) || *p == '_' ) {
				if ( i >= YC_MAX_SYMBOL_LENGTH )
					Error( "Unexpected symbol length!\n" );

				token->symbol[ i++ ] = *p++;
			}
			token->type = GetTokenTypeForSymbol( token->symbol, NULL );
		} else if ( isdigit( *p ) ) {
			token->type = DK_TOKENTYPE_INT;

			int i = 0;
			while ( isdigit( *p ) || *p == '.' ) {
				if ( i >= YC_MAX_SYMBOL_LENGTH )
					Error( "Unexpected symbol length!\n" );

				if ( *p == '.' ) {
					if ( token->type == DK_TOKENTYPE_DEC )
						Error( "Unexpected token in num: %u:%u\n", token->lineNum, token->linePos );

					token->type = DK_TOKENTYPE_DEC;
				}
				token->symbol[ i++ ] = *p++;
			}
		} else if ( *p == '\'' ) {
			token->type = DK_TOKENTYPE_STRING;
			p++;
			int i = 0;
			do {
				if ( i >= YC_MAX_SYMBOL_LENGTH )
					Error( "Unexpected symbol length!\n" );

				token->symbol[ i++ ] = *p;
			} while ( *++p != '\0' && *p != '\'' );
			if ( *p != '\'' )
				Error( "String is not enclosed: %u:%u\n", token->lineNum, token->linePos );
			p++;
		} else if ( *p == '\0' ) {
			token->type = DK_TOKENTYPE_EOF;
			strcpy( token->symbol, "\\0" );
		}

		if ( token->type == DK_TOKENTYPE_INVALID ) {
			unsigned int l;
			token->type = GetTokenTypeForSymbol( p, &l );
			if ( token->type != DK_TOKENTYPE_INVALID ) {
				strncpy( token->symbol, p, l );
				p += l;
			}
		}

		if ( token->type != DK_TOKENTYPE_INVALID ) {
			token->node = PlInsertLinkedListNode( list, token );

			if ( token->type == DK_TOKENTYPE_EOF )
				break;

			continue;
		}

		PlFree( token );

		Warning( "Unexpected character: %u:%u\n", token->lineNum, token->linePos );
		p++;
	}
}

DKLexer *DKLexer_GenerateTokenList( DKLexer *handle, const char *buf, const char *file ) {
	if ( handle == NULL ) {
		handle = PL_NEW( DKLexer );
		handle->tokens = PlCreateLinkedList();
		snprintf( handle->originFile, sizeof( handle->originFile ), "%s", file );
	}

	double startTime = PlGetCurrentSeconds();

	unsigned int curLineNum = 0;
	const char *p = buf;
	while ( *p != '\0' ) {
		curLineNum++;

		if ( *p == '/' && *( p + 1 ) == '/' ) { /* single-line comment */
			PlSkipLine( &p );
			continue;
		} else if ( *p == '/' && *( p + 1 ) == '*' ) { /* multi-line comment */
			p += 2;
			while ( *p != '*' && *( p + 1 ) != '/' ) {
				int l = PlGetLineEndType( p );
				if ( l != PL_PARSE_NL_INVALID ) {
					p += l;
					curLineNum++;
					continue;
				}

				p++;
			}
			p += 2;
			continue;
		}

		/* tokenise the line */
		unsigned int bufSize = PlDetermineLineLength( p ) + 1;
		char *line = PlMAllocA( bufSize );
		PlParseLine( &p, line, bufSize );

		DKLexer_ParseLine( line, file, curLineNum, handle->tokens );

		PlFree( line );
	}

	/* output the result from the lexer */
#if !defined( NDEBUG )
	Print( "%5s %20s %10s %10s\n", "TYPE", "SYMBOL", "LINE", "LPOS" );
	PLLinkedListNode *node = PlGetFirstNode( handle->tokens );
	while ( node != NULL ) {
		const DKLexerToken *token = PlGetLinkedListNodeUserData( node );
		printf( "%5d %20s %10u %10u\n", token->type, token->symbol, token->lineNum, token->linePos );
		node = PlGetNextLinkedListNode( node );
	}
#endif

	double endTime = PlGetCurrentSeconds();

	Print( "Lexer took " PL_FMT_double "s for \"%s\"\n", endTime - startTime, file );

	return handle;
}

void Yang_Lexer_Test( void ) {
	static const char *longString =
	        "// data declares the constant value\n"
	        "decl someConstant int const( 1 );\n"
	        "/* blah blah blah blah blah\n"
	        "lbvdjgjfhdsjgfhdskghfkdj gfhsjgfh\n"
	        "*/\n"
	        "// initial sets the initial value\n"
	        "decl someInitialisedVar int initial( 1 );\n"
	        "// and can of course also be used with arrays\n"
	        "decl someArrayInitVar(4) int initial( 1, 2, 3, 4 );\n"
	        "\n"
	        "decl PI float const(3.1415927);\n"
	        "\n"
	        "println: proc(msg) extern;\n"
	        "    decl msg string;\n"
	        "end println;\n"
	        ">\n>=\n<\n<=\n:=\n"
	        "* @ / ( ) $\n"
	        ">5<=4:=5\n";

	DKLexer_GenerateTokenList( NULL, longString, NULL );
}
