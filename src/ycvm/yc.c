/* ======================================================================
 * Yin C/VM Suite
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================== */

#include <plcore/pl_filesystem.h>

#include "ycvm.h"
#include "yc.h"

unsigned int LOG_LEVEL_DEFAULT;
unsigned int LOG_LEVEL_WARNING;
unsigned int LOG_LEVEL_ERROR;

typedef enum YCTokenType
{
	YC_TOKENTYPE_EOF = -1,
	YC_TOKENTYPE_NL,// New line
	YC_TOKENTYPE_NUMBER,
	YC_TOKENTYPE_STRING,

	// Conditionals
	YC_TOKENTYPE_IF,  // if
	YC_TOKENTYPE_THEN,// then
	YC_TOKENTYPE_ELSE,// else
	// Statement groups
	YC_TOKENTYPE_DO,
	YC_TOKENTYPE_END,
	// Declarations
	YC_TOKENTYPE_DECLARE,  // declare
	YC_TOKENTYPE_STRUCTURE,// structure
	YC_TOKENTYPE_LABEL,
	YC_TOKENTYPE_DATA,
	YC_TOKENTYPE_LITERALLY,
	YC_TOKENTYPE_BASED,
	YC_TOKENTYPE_PROCEDURE,// procedure
	YC_TOKENTYPE_EXTERNAL,
	YC_TOKENTYPE_PUBLIC,
	YC_TOKENTYPE_AT,
	// Flow control / branching
	YC_TOKENTYPE_GOTO, // goto
	YC_TOKENTYPE_BY,   // by
	YC_TOKENTYPE_CASE, // case
	YC_TOKENTYPE_WHILE,// while
	//
	YC_TOKENTYPE_CALL,  // call
	YC_TOKENTYPE_RETURN,// return
	YC_TOKENTYPE_HALT,  // halt
	// Boolean operators
	YC_TOKENTYPE_OR, // or
	YC_TOKENTYPE_AND,// and
	YC_TOKENTYPE_XOR,// xor
	YC_TOKENTYPE_NOT,// not
	// Operators
	YC_TOKENTYPE_ASSIGN,  // :=
	YC_TOKENTYPE_EQ,      // =
	YC_TOKENTYPE_NOTEQ,   // <>
	YC_TOKENTYPE_MOD,     // %
	YC_TOKENTYPE_PLUS,    // +
	YC_TOKENTYPE_MINUS,   // -
	YC_TOKENTYPE_LT,      // <
	YC_TOKENTYPE_LTEQ,    // <=
	YC_TOKENTYPE_GT,      // >
	YC_TOKENTYPE_GTEQ,    // >=
	YC_TOKENTYPE_ASTERISK,// *
	YC_TOKENTYPE_SLASH,   // /
	//
	YC_TOKENTYPE_COLON,    // :
	YC_TOKENTYPE_SEMICOLON,// ;
	YC_TOKENTYPE_COMMA,    // ,

	YC_MAX_TOKENTYPES
} YCTokenType;

static const char *reservedWords[ YC_MAX_TOKENTYPES ] = {
        [YC_TOKENTYPE_IF]   = "if",
        [YC_TOKENTYPE_THEN] = "then",
        [YC_TOKENTYPE_ELSE] = "else",

        [YC_TOKENTYPE_DO]  = "do",
        [YC_TOKENTYPE_END] = "end",

        [YC_TOKENTYPE_DECLARE]   = "declare",
        [YC_TOKENTYPE_PROCEDURE] = "procedure",

        [YC_TOKENTYPE_GOTO]  = "goto",
        [YC_TOKENTYPE_BY]    = "by",
        [YC_TOKENTYPE_CASE]  = "case",
        [YC_TOKENTYPE_WHILE] = "while",

        [YC_TOKENTYPE_OR]  = "or",
        [YC_TOKENTYPE_AND] = "and",
        [YC_TOKENTYPE_XOR] = "xor",
        [YC_TOKENTYPE_NOT] = "not",

        [YC_TOKENTYPE_ASSIGN]   = ":=",
        [YC_TOKENTYPE_EQ]       = "=",
        [YC_TOKENTYPE_NOTEQ]    = "<>",
        [YC_TOKENTYPE_MOD]      = "%",
        [YC_TOKENTYPE_PLUS]     = "+",
        [YC_TOKENTYPE_MINUS]    = "-",
        [YC_TOKENTYPE_LT]       = "<",
        [YC_TOKENTYPE_LTEQ]     = "<=",
        [YC_TOKENTYPE_GT]       = ">",
        [YC_TOKENTYPE_GTEQ]     = ">=",
        [YC_TOKENTYPE_ASTERISK] = "*",
        [YC_TOKENTYPE_SLASH]    = "/",

        [YC_TOKENTYPE_COLON]     = ":",
        [YC_TOKENTYPE_SEMICOLON] = ";",
};

YCTokenType YLex_GetTokenType( const char *c )
{
	if ( c == '\0' )
	{
		return YC_TOKENTYPE_EOF;
	}

	for ( unsigned int i = 0; i < YC_MAX_TOKENTYPES; ++i )
	{
		if ( strcmp( reservedWords[ i ], c ) != 0 )
		{
			continue;
		}

		return ( YCTokenType ) i;
	}

	return YC_TOKENTYPE_STRING;
}

static bool TestLexer( void )
{
	const char *string =
	        "declare myVar float;\n"
	        "myVar := 2;\n";
}

int main( int argc, char **argv )
{
	PlInitialize( argc, argv );

	PlSetupLogOutput( YC_LOG_PATH );
	LOG_LEVEL_DEFAULT = PlAddLogLevel( "yc", PL_COLOUR_GREEN, true );
	LOG_LEVEL_WARNING = PlAddLogLevel( "yc/warning", PL_COLOUR_ORANGE, true );
	LOG_LEVEL_ERROR   = PlAddLogLevel( "yc/error", PL_COLOUR_RED, true );

	Print( "Yin Compiler\n"
	       "Written by Mark E Sowden for Project Yin\n"
	       "===================================\n" );

	if ( argc < 2 )
	{
		Print( "Usage:\n"
		       " <project_path> [-<option> ...]" );
	}

	PLFile *file = PlOpenFile( argv[ 1 ], true );
	if ( file == NULL )
	{
		Error( "Failed to open \"%s\"!\n", argv[ 1 ] );
	}

	const char *buf = PlGetFileData( file );
}
