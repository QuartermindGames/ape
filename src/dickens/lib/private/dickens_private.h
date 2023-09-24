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

typedef enum DkOpCode
{
	DK_VM_OP_NOP,
	DK_VM_OP_RETURN,
	DK_VM_OP_OR,
	DK_VM_OP_AND,
	DK_VM_OP_CALL,

	DK_VM_OP_MUL,
	DK_VM_OP_INC,
	DK_VM_OP_ADD,
	DK_VM_OP_SUB,
	DK_VM_OP_NEG,

	DK_VM_MAX_OPCODES
} DkOpCode;

#define DK_MAX_SYMBOL_LENGTH 128
typedef char DkSymbolName[ DK_MAX_SYMBOL_LENGTH ];

typedef enum DkSymbolVisibility
{
	DK_SYMBOL_VISIBILITY_PRIVATE,
	DK_SYMBOL_VISIBILITY_PUBLIC,
} DkSymbolVisibility;

typedef struct DkSymbol
{
	DkSymbolName name;
	DkSymbolVisibility visibility;
} DkSymbol;

typedef enum DkDataType
{
	DK_DATA_TYPE_VOID,  //0
	DK_DATA_TYPE_FLOAT, //4
	DK_DATA_TYPE_CHAR,  //1
	DK_DATA_TYPE_UCHAR, //1
	DK_DATA_TYPE_SHORT, //2
	DK_DATA_TYPE_USHORT,//2
	DK_DATA_TYPE_INT,   //4
	DK_DATA_TYPE_UINT,  //4
	DK_DATA_TYPE_LONG,  //8
	DK_DATA_TYPE_ULONG, //8
	DK_DATA_TYPE_STRING,
} DkDataType;

typedef struct DkLexerToken
{
	DkSymbolName symbol;
	DKTokenType type;
	PLPath path;
	unsigned int lineNum;
	unsigned int linePos;
	PLLinkedListNode *node;
} DkLexerToken;

typedef struct DkLexer
{
	PLPath originFile;
	PLLinkedList *tokens;
} DkLexer;
