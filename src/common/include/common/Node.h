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

	NODE_PROPERTY_VEC2,
	NODE_PROPERTY_VEC3,
	NODE_PROPERTY_VEC4,

	NODE_PROPERTY_NUMERIC_ARRAY,

	NL_MAX_PROPERTY_TYPES
} NLPropertyType;

extern const char *NL_GetError( void );

extern unsigned int NL_GetNumOfChildren( const NLNode *root );
extern NLNode *NL_GetFirstChild( NLNode *root );
extern NLNode *NL_GetNextChild( NLNode *node );
extern NLNode *NL_GetChildByName( NLNode *root, const char *name );
extern NLNode *NL_GetParent( NLNode *node );

extern const char *Node_GetName( const NLNode *node );
extern NLPropertyType Node_GetType( const NLNode *node );
extern const char *Node_GetString( const NLNode *node );
extern bool Node_GetBoolean( const NLNode *node );
extern float Node_GetFloat( const NLNode *node );
extern PLVector2 Node_GetVec2( const NLNode *node );
extern PLVector3 Node_GetVec3( const NLNode *node );
extern PLVector4 Node_GetVec4( const NLNode *node );

extern NLNode *NL_AddObject( NLNode *node, const char *name );
extern NLNode *NL_AddStringVar( NLNode *root, const char *name, const char *var );
extern NLNode *NL_AddBooleanVar( NLNode *root, const char *name, bool var );
extern NLNode *NL_AddNumericVar( NLNode *root, const char *name, float var );

extern void NL_DestroyNode( NLNode *node );

extern NLNode *NL_LoadFile( const char *path );
extern void NL_WriteFile( const char *path, NLNode *root, NLFileType fileType );

/* debugging */
extern void NL_PrintNodeTree( NLNode *node, int index );
