/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include "Common.h"

PL_EXTERN_C

typedef struct NLNode NLNode;

#define NL_DEFAULT_EXTENSION ".node"

typedef enum NLErrorCode {
	NL_ERROR_SUCCESS,

	NL_ERROR_IO_READ, /* read failure */
    NL_ERROR_IO_WRITE, /* write failure */

	NL_ERROR_MEM_ALLOC, /* alloc failure */

	NL_ERROR_INVALID_ARGUMENT,
	NL_ERROR_INVALID_TYPE, /* invalid node parent/child type */
	NL_ERROR_INVALID_ELEMENTS, /* unexpected number of elements */
} NLErrorCode;

typedef enum NLFileType {
	NL_FILE_INVALID = -1,
	NL_FILE_BINARY,
	NL_FILE_ASCII,

	NL_MAX_FILE_TYPES
} NLFileType;

typedef enum NLPropertyType {
	NODE_PROPERTY_INVALID = -1,
	NODE_PROPERTY_OBJECT,
	NODE_PROPERTY_LINK, /* todo */
	NODE_PROPERTY_ARRAY,
	NODE_PROPERTY_STRING,
	NODE_PROPERTY_BOOLEAN,
	NODE_PROPERTY_FLOAT,
	NODE_PROPERTY_INTEGER,

	NL_MAX_PROPERTY_TYPES
} NLPropertyType;

extern const char *NL_GetErrorMessage( void );
extern NLErrorCode NL_GetError( void );

extern unsigned int NL_GetNumOfChildren( const NLNode *parent ); /* only valid for object/array */
extern NLNode *NL_GetFirstChild( NLNode *parent );
extern NLNode *NL_GetNextChild( NLNode *node );
extern NLNode *NL_GetChildByName( NLNode *parent, const char *name ); /* only valid for object */
extern NLNode *NL_GetChildByIndex( NLNode *parent, unsigned int i ); /* only valid for array */
extern NLNode *NL_GetParent( NLNode *node );

extern const char *NL_GetName( const NLNode *node );
extern NLPropertyType NL_GetType( const NLNode *node );
extern const char *NL_GetString( const NLNode *node );
extern bool NL_GetBoolean( const NLNode *node );
extern float NL_GetFloat( const NLNode *node );
extern int *NL_GetIntArray( NLNode *parent, int *buf );
extern float *NL_GetFloatArray( NLNode *parent, float *buf );
extern PLVector2 NL_GetVec2( NLNode *node );
extern PLVector3 NL_GetVec3( NLNode *node );
extern PLVector4 NL_GetVec4( NLNode *node );

extern NLNode *NL_PushBackObj( NLNode *node, const char *name );
extern NLNode *NL_PushBackString( NLNode *parent, const char *name, const char *var );
extern NLNode *NL_PushBackBool( NLNode *parent, const char *name, bool var );
extern NLNode *NL_PushBackInt( NLNode *parent, const char *name, int var );
extern NLNode *NL_PushBackFloat( NLNode *parent, const char *name, float var );
extern NLNode *NL_PushBackIntArray( NLNode *parent, const char *name, const int *array, unsigned int numElements );
extern NLNode *NL_PushBackFloatArray( NLNode *parent, const char *name, const float *array, unsigned int numElements );
extern NLNode *NL_PushBackVec2( NLNode *parent, const char *name, const PLVector2 *var );
extern NLNode *NL_PushBackVec3( NLNode *parent, const char *name, const PLVector3 *var );
extern NLNode *NL_PushBackVec4( NLNode *parent, const char *name, const PLVector4 *var );

#ifndef __cplusplus
#define NL_PushBackArray( PARENT, NAME, ARRAY, NUM ) \
	_Generic( ( ARRAY ),                             \
	          int                                    \
	          : NL_PushBackIntArray,                 \
	            float                                \
	          : NL_PushBackFloatArray )( PARENT, NAME, ARRAY, NUM )
#define NL_PushBackVariable( ROOT, NAME, VAR ) \
	_Generic( ( VAR ),                         \
	          bool                             \
	          : NL_PushBackBool,               \
	            int                            \
	          : NL_PushBackInt,                \
	            float                          \
	          : NL_PushBackFloat,              \
	            PLVector2 *                    \
	          : NL_PushBackVec2,               \
	            PLVector3 *                    \
	          : NL_PushBackVec3,               \
	            PLVector4 *                    \
	          : NL_PushBackVec4 )( ROOT, NAME, VAR )
#endif

extern void NL_DestroyNode( NLNode *node );

extern NLNode *NL_LoadFile( const char *path, const char *objectType );
extern void NL_WriteFile( const char *path, NLNode *root, NLFileType fileType );

extern NLNode *NL_ParseBuffer( const char *buf, size_t length );

/* debugging */
extern void NL_PrintNodeTree( NLNode *node, int index );

PL_EXTERN_C_END

#ifdef __cplusplus
static inline NLNode *NL_PushBackArray( NLNode *parent, const char *name, const int *array, unsigned int numElements ) {
	return NL_PushBackIntArray( parent, name, array, numElements );
}
static inline NLNode *NL_PushBackArray( NLNode *parent, const char *name, const float *array, unsigned int numElements ) {
	return NL_PushBackFloatArray( parent, name, array, numElements );
}
static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, const char *var ) {
	return NL_PushBackString( parent, name, var );
}
static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, bool var ) {
	return NL_PushBackBool( parent, name, var );
}
static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, int var ) {
	return NL_PushBackInt( parent, name, var );
}
static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, float var ) {
	return NL_PushBackFloat( parent, name, var );
}
static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, const PLVector2 *var ) {
	return NL_PushBackVec2( parent, name, var );
}
static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, const PLVector3 *var ) {
	return NL_PushBackVec3( parent, name, var );
}
static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, const PLVector4 *var ) {
	return NL_PushBackVec4( parent, name, var );
}
#endif
