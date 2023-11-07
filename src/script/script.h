// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_linkedlist.h>

#define SS_SCRIPT_MAX_SYMBOL_LENGTH 128
typedef char SS_ScriptSymbolName[ SS_SCRIPT_MAX_SYMBOL_LENGTH ];

typedef enum SS_ScriptDataType
{
	SS_SCRIPT_DATA_TYPE_VOID, //0
	SS_SCRIPT_DATA_TYPE_FLOAT,//4
	SS_SCRIPT_DATA_TYPE_INT,  //4
	SS_SCRIPT_DATA_TYPE_STRING,
} SS_ScriptDataType;

typedef enum SS_ScriptLexerTokenType
{
	SS_SCRIPT_TOKEN_TYPE_INVALID = 0,
	SS_SCRIPT_TOKEN_TYPE_EOF,

	SS_SCRIPT_TOKEN_TYPE_INT,
	SS_SCRIPT_TOKEN_TYPE_DEC,
	SS_SCRIPT_TOKEN_TYPE_STRING,

	SS_SCRIPT_TOKEN_TYPE_IDENT,
	SS_SCRIPT_TOKEN_TYPE_COLON,

	SS_SCRIPT_TOKEN_TYPE_IF,
	SS_SCRIPT_TOKEN_TYPE_ELSE,
	SS_SCRIPT_TOKEN_TYPE_ELIF,
	SS_SCRIPT_TOKEN_TYPE_ENDI,
} SS_ScriptLexerTokenType;

typedef struct SS_ScriptLexerToken
{
	SS_ScriptSymbolName symbol;
	SS_ScriptLexerTokenType type;

	PLPath path;
	unsigned int lineNum;
	unsigned int linePos;

	PLLinkedListNode *node;
} SS_ScriptLexerToken;

/**
 * Container for script lexer output.
 */
typedef struct SS_ScriptLexer
{
	PLPath source;
	PLLinkedList *tokens;
} SS_ScriptLexer;

#define THROW_ERROR( ... )               \
	{                                    \
		printf( "ERROR: " __VA_ARGS__ ); \
		abort();                         \
	}
