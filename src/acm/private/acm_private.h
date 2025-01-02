// SPDX-License-Identifier: MIT
// Ape Config Markup
// Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_linkedlist.h>
#include <plcore/pl_console.h>

#include "acm/public/acm/acm.h"

/* node structure
 *  string
 *      uint32_t length
 *      char buffer[ length ]
 *
 *  string name
 *  uint8_t type
 *  if type == float: float var
 *  if type == integer: uint32_t var
 *  if type == string: string var
 *  if type == boolean: uint8_t var
 *  if type == object:
 *      uint32_t numChildren
 *      for numChildren
 *          read node
 *
 */

extern int nd_LogLevelPrint_;
#define Message( FORMAT, ... ) PlLogMessage( nd_LogLevelPrint_, FORMAT, ##__VA_ARGS__ )
extern int nd_LogLevelWarn_;
#define Warning( FORMAT, ... ) PlLogMessage( nd_LogLevelWarn_, "WARNING: " FORMAT, ##__VA_ARGS__ )

/* upper limits used for the parser */
#define ND_MAX_NAME_LENGTH   256
#define ND_MAX_STRING_LENGTH 256
#define ND_MAX_BOOL_LENGTH   8 /* 0, 1, true, false */
#define ND_MAX_TYPE_LENGTH   16

typedef struct NdVarString
{
	char    *buf;
	uint16_t length;
} NdVarString;

typedef struct AcmBranch
{
	NdVarString     name;
	AcmPropertyType type;
	AcmPropertyType childType; /* used for array types */
	NdVarString     data;
	AcmBranch      *parent;

	PLLinkedListNode *linkedListNode;
	PLLinkedList     *linkedList;
} AcmBranch;

char      *acm_preprocess_script_( char *buf, size_t *length, bool isHead );
AcmBranch *acm_push_new_branch( AcmBranch *parent, const char *name, AcmPropertyType propertyType, AcmPropertyType childType );

AcmBranch *acm_push_variable_( AcmBranch *parent, const char *name, const char *value, AcmPropertyType type );

/////////////////////////////////////////////////////////////////////////////////////
// Lexer

typedef enum AcmTokenType
{
	ACM_TOKEN_TYPE_INVALID,

	ACM_TOKEN_TYPE_EOF,

	ACM_TOKEN_TYPE_TYPENAME,
	ACM_TOKEN_TYPE_IDENTIFIER,
	ACM_TOKEN_TYPE_STRING,
	ACM_TOKEN_TYPE_INTEGER,
	ACM_TOKEN_TYPE_DECIMAL,

	ACM_TOKEN_TYPE_OPEN_BRACKET, // {
	ACM_TOKEN_TYPE_CLOSE_BRACKET,// }
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

AcmLexer *acm_lexer_parse_buffer_( AcmLexer *self, const char *buf, const char *file );
