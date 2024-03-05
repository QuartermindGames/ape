// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_console.h>

#define DICKENS_VERSION_MAJOR 1
#define DICKENS_VERSION_MINOR 0
#define DICKENS_VERSION_PATCH 0

#define DICKENS_LOG_PATH "dickens_output.txt"

typedef enum DKTokenType
{
	DK_TOKENTYPE_INVALID = 0,

	DK_TOKENTYPE_EOF,

	DK_TOKENTYPE_INT,
	DK_TOKENTYPE_DEC,

	DK_TOKENTYPE_IDENT,

	// Data type
	DK_TOKENTYPE_TYPENAME,
	DK_TOKENTYPE_STRING,
	// Conditionals
	DK_TOKENTYPE_IF,  // if
	DK_TOKENTYPE_THEN,// then
	DK_TOKENTYPE_ELSE,// else
	// Statement groups
	DK_TOKENTYPE_DO,
	DK_TOKENTYPE_END,
	// Declarations
	DK_TOKENTYPE_DECLARE,  // decl
	DK_TOKENTYPE_STRUCTURE,// struct
	DK_TOKENTYPE_UNION,    // union
	DK_TOKENTYPE_LABEL,    // label
	DK_TOKENTYPE_CONST,    // const
	DK_TOKENTYPE_INITIAL,  // initial
	DK_TOKENTYPE_TYPEDEF,  // typedef
	DK_TOKENTYPE_BASED,    // based
	DK_TOKENTYPE_PROCEDURE,// proc
	DK_TOKENTYPE_EXTERNAL, // extern
	DK_TOKENTYPE_PUBLIC,   // public
	DK_TOKENTYPE_NATIVE,   // native
	DK_TOKENTYPE_AT,
	// Flow control / branching
	DK_TOKENTYPE_GOTO,   // goto
	DK_TOKENTYPE_BY,     // by
	DK_TOKENTYPE_CASE,   // case
	DK_TOKENTYPE_WHILE,  // while
	DK_TOKENTYPE_LEAVE,  // leave
	DK_TOKENTYPE_ITERATE,// iterate
	//
	DK_TOKENTYPE_CALL,  // call
	DK_TOKENTYPE_RETURN,// return
	DK_TOKENTYPE_HALT,  // halt
	/////////////////////////////////////
	// Operators
	DK_TOKENTYPE_OR,       // |
	DK_TOKENTYPE_AND,      // &
	DK_TOKENTYPE_XOR,      // ^
	DK_TOKENTYPE_NOT,      // !
	DK_TOKENTYPE_ASSIGN,   // :=
	DK_TOKENTYPE_EQ,       // =
	DK_TOKENTYPE_NOTEQ,    // <>
	DK_TOKENTYPE_MOD,      // %
	DK_TOKENTYPE_PLUS,     // +
	DK_TOKENTYPE_MINUS,    // -
	DK_TOKENTYPE_LT,       // <
	DK_TOKENTYPE_LTEQ,     // <=
	DK_TOKENTYPE_GT,       // >
	DK_TOKENTYPE_GTEQ,     // >=
	DK_TOKENTYPE_MUL,      // *
	DK_TOKENTYPE_SLASH,    // /
	DK_TOKENTYPE_REFERENCE,// @
	// Delimiters
	DK_TOKENTYPE_COLON,     // :
	DK_TOKENTYPE_SEMICOLON, // ;
	DK_TOKENTYPE_COMMA,     // ,
	DK_TOKENTYPE_QUOTE,     // '
	DK_TOKENTYPE_STOP,      // .
	DK_TOKENTYPE_LEFTBRACE, // (
	DK_TOKENTYPE_RIGHTBRACE,// )
	// Directives
	DK_TOKENTYPE_DOLLAR,// $

	DK_MAX_TOKENTYPES
} DKTokenType;

typedef struct DkLexer DkLexer;
typedef struct DkParser DkParser;

PL_EXTERN_C

PL_EXPORT DkLexer *dk_generate_token_list( DkLexer *handle, const char *buf, const char *file );
PL_EXPORT DkParser *dk_parse_program( DkLexer *lexer );
PL_EXPORT bool dk_assemble_from_buffer( const DkParser *parser );

PL_EXTERN_C_END
