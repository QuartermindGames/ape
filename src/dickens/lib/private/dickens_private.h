// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "dickens.h"

#include <plcore/pl_timer.h>
#include <plcore/pl_linkedlist.h>

#define DK_ENABLE_PROFILER 1

#define Print( ... )   printf( __VA_ARGS__ )
#define Warning( ... ) printf( "WARNING: " __VA_ARGS__ )
#define Error( ... )                     \
	{                                    \
		printf( "ERROR: " __VA_ARGS__ ); \
		abort();                         \
	}

#define YC_MAX_SYMBOL_LENGTH 128
typedef char DKSymbolName[ YC_MAX_SYMBOL_LENGTH ];

typedef enum DKSymbolVisibility {
	DK_SYMBOLVISIBILITY_PRIVATE,
	DK_SYMBOLVISIBILITY_PUBLIC,
} DKSymbolVisibility;

typedef struct DKSymbol {
	DKSymbolName name;
	DKSymbolVisibility visibility;
} DKSymbol;

typedef enum DKDataType {
	DK_DATATYPE_VOID,
	DK_DATATYPE_FLOAT,
	DK_DATATYPE_CHAR,
	DK_DATATYPE_UCHAR,
	DK_DATATYPE_SHORT,
	DK_DATATYPE_USHORT,
	DK_DATATYPE_INT,
	DK_DATATYPE_UINT,
	DK_DATATYPE_STRING,
} DKDataType;

typedef struct DKLexerToken {
	DKSymbolName symbol;
	DKTokenType type;
	PLPath path;
	unsigned int lineNum;
	unsigned int linePos;
	PLLinkedListNode *node;
} DKLexerToken;

typedef struct DKLexer {
	PLPath originFile;
	PLLinkedList *tokens;
} DKLexer;
