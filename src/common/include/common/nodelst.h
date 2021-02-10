/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include "common.h"

typedef struct NLNode NLNode;

#define NL_DEFAULT_EXTENSION ".node"

typedef enum NLFileType {
	NL_FILE_INVALID = -1,
	NL_FILE_BINARY,
	NL_FILE_ASCII,

	NL_MAX_FILE_TYPES
} NLFileType;

typedef enum NLPropertyType {
	NODE_PROPERTY_INVALID = -1,
	NODE_PROPERTY_OBJECT,
	NODE_PROPERTY_STRING,
	NODE_PROPERTY_BOOLEAN,
	NODE_PROPERTY_FLOAT,
	NODE_PROPERTY_INTEGER,

	NODE_PROPERTY_NUMERIC_ARRAY,

	NL_MAX_PROPERTY_TYPES
} NLPropertyType;

extern const char *NL_GetError( void );

extern unsigned int NL_GetNumOfChildren( const NLNode *root );
extern NLNode *NL_GetFirstChild( NLNode *root );
extern NLNode *NL_GetNextChild( NLNode *node );

extern const char *NL_GetNodeName( const NLNode *node );
extern const char *NL_GetNodeStringData( const NLNode *node );
extern bool NL_GetNodeBooleanData( const NLNode *node );
extern float NL_GetNodeNumericData( const NLNode *node );

extern NLNode *NL_AddObject( NLNode *node, const char *name );
extern NLNode *NL_AddStringVar( NLNode *root, const char *name, const char *var );
extern NLNode *NL_AddBooleanVar( NLNode *root, const char *name, bool var );
extern NLNode *NL_AddNumericVar( NLNode *root, const char *name, float var );

extern NLNode *NL_LoadFile( const char *path );
extern void NL_WriteFile( const char *path, NLNode *root, NLFileType fileType );
