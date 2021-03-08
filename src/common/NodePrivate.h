/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include "common/Node.h"

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

/* upper limits used for the parser */
#define NL_MAX_NAME_LENGTH 256
#define NL_MAX_STRING_LENGTH 256
#define NL_MAX_BOOL_LENGTH 8 /* 0, 1, true, false */
#define NL_MAX_TYPE_LENGTH 16

typedef struct NLVarString {
	char *strBuf;
	unsigned int strBufLength;
} NLVarString;

typedef struct NLNode {
	NLVarString name;
	NLPropertyType type;
	NLPropertyType childType; /* used for array types */
	NLVarString data;
	NLNode *parent;

	PLLinkedListNode *linkedListNode;
	PLLinkedList *linkedList;
} NLNode;

char *xNL_PreProcessScript( char *buf, size_t *length, bool isHead );
NLNode *xNL_PushBackNode( NLNode *parent, const char *name, NLPropertyType propertyType );
