/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include "common.h"

PL_EXTERN_C

typedef struct NLNode NLNode;

#define NL_DEFAULT_EXTENSION ".node"

typedef enum NLErrorCode
{
	NL_ERROR_SUCCESS,

	NL_ERROR_IO_READ,  /* read failure */
	NL_ERROR_IO_WRITE, /* write failure */

	NL_ERROR_MEM_ALLOC, /* alloc failure */

	NL_ERROR_INVALID_ARGUMENT,
	NL_ERROR_INVALID_TYPE,     /* invalid node parent/child type */
	NL_ERROR_INVALID_ELEMENTS, /* unexpected number of elements */
} NLErrorCode;

typedef enum NLFileType
{
	NL_FILE_INVALID = -1,
	NL_FILE_BINARY,
	NL_FILE_ASCII,

	NL_MAX_FILE_TYPES
} NLFileType;

typedef enum NLPropertyType
{
	NL_PROP_UNDEFINED = -1,

	NL_PROP_OBJ,
	NL_PROP_LINK, /* todo */
	NL_PROP_ARRAY,

	NL_PROP_STR,
	NL_PROP_BOOL,

	NL_PROP_F32, // float
	NL_PROP_F64, // double
	NL_PROP_I8,  // int8
	NL_PROP_I16, // int16
	NL_PROP_I32, // int32
	NL_PROP_I64, // int64
	NL_PROP_UI8, // uint8
	NL_PROP_UI16,// uint16
	NL_PROP_UI32,// uint32
	NL_PROP_UI64,// uint64

	NL_MAX_PROPERTYTYPES
} NLPropertyType;

typedef union NLPropertyData_U
{
	float    f32;
	double   f64;
	int8_t   i8;
	int16_t  i16;
	int32_t  i32;
	int64_t  i64;
	uint8_t  ui8;
	uint16_t ui16;
	uint32_t ui32;
	uint64_t ui64;
} NLPropertyData_U;

extern const char *NL_GetErrorMessage( void );
extern NLErrorCode NL_GetError( void );

extern unsigned int NL_GetNumOfChildren( const NLNode *parent ); /* only valid for object/array */
extern NLNode *     NL_GetFirstChild( NLNode *parent );
extern NLNode *     NL_GetNextChild( NLNode *node );
extern NLNode *     NL_GetChildByName( NLNode *parent, const char *name ); /* only valid for object */
extern NLNode *     NL_GetChildByIndex( NLNode *parent, unsigned int i );  /* only valid for array */
extern NLNode *     NL_GetParent( NLNode *node );

extern const char *   NL_GetName( const NLNode *node );
extern NLPropertyType NL_GetType( const NLNode *node );

extern NLErrorCode NL_GetBool( const NLNode *node, bool *dest );
extern NLErrorCode NL_GetStr( const NLNode *node, char *dest, size_t length );
extern NLErrorCode NL_GetF32( const NLNode *node, float *dest );
extern NLErrorCode NL_GetF64( const NLNode *node, double *dest );
extern NLErrorCode NL_GetI8( const NLNode *node, int8_t *dest );
extern NLErrorCode NL_GetI16( const NLNode *node, int16_t *dest );
extern NLErrorCode NL_GetI32( const NLNode *node, int32_t *dest );
extern NLErrorCode NL_GetI64( const NLNode *node, int64_t *dest );
extern NLErrorCode NL_GetUI8( const NLNode *node, uint8_t *dest );
extern NLErrorCode NL_GetUI16( const NLNode *node, uint16_t *dest );
extern NLErrorCode NL_GetUI32( const NLNode *node, uint32_t *dest );
extern NLErrorCode NL_GetUI64( const NLNode *node, uint64_t *dest );

extern NLErrorCode NL_GetStrArray( NLNode *parent, const char **buf, unsigned int numElements );
extern NLErrorCode NL_GetI8Array( NLNode *parent, int8_t *buf, unsigned int numElements );
extern NLErrorCode NL_GetI16Array( NLNode *parent, int16_t *buf, unsigned int numElements );
extern NLErrorCode NL_GetI32Array( NLNode *parent, int32_t *buf, unsigned int numElements );
extern NLErrorCode NL_GetUI32Array( NLNode *parent, uint32_t *buf, unsigned int numElements );
extern NLErrorCode NL_GetF32Array( NLNode *parent, float *buf, unsigned int numElements );

extern bool        NL_GetBoolByName( NLNode *node, const char *name, bool fallback );
extern const char *NL_GetStrByName( NLNode *node, const char *name, const char *fallback );
extern int8_t      NL_GetI8ByName( NLNode *node, const char *name, int8_t fallback );
extern int16_t     NL_GetI16ByName( NLNode *node, const char *name, int16_t fallback );
extern int32_t     NL_GetI32ByName( NLNode *node, const char *name, int32_t fallback );
extern float       NL_GetF32ByName( NLNode *node, const char *name, float fallback );
extern double      NL_GetF64ByName( NLNode *node, const char *name, double fallback );

extern NLNode *NL_PushBackObj( NLNode *node, const char *name );
extern NLNode *NL_PushBackStr( NLNode *parent, const char *name, const char *var );
extern NLNode *NL_PushBackBool( NLNode *parent, const char *name, bool var );
extern NLNode *NL_PushBackI8( NLNode *parent, const char *name, int8_t var );
extern NLNode *NL_PushBackI16( NLNode *parent, const char *name, int16_t var );
extern NLNode *NL_PushBackI32( NLNode *parent, const char *name, int32_t var );
extern NLNode *NL_PushBackF32( NLNode *parent, const char *name, float var );
extern NLNode *NL_PushBackF64( NLNode *parent, const char *name, double var );

extern NLNode *NL_PushBackObjArray( NLNode *parent, const char *name );
extern NLNode *NL_PushBackStrArray( NLNode *parent, const char *name, const char **array, unsigned int numElements );
extern NLNode *NL_PushBackI32Array( NLNode *parent, const char *name, const int32_t *array, unsigned int numElements );
extern NLNode *NL_PushBackF32Array( NLNode *parent, const char *name, const float *array, unsigned int numElements );

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

extern NLNode *NL_CopyNode( NLNode *node );
extern void    NL_DestroyNode( NLNode *node );

extern NLNode *NL_LoadFile( const char *path, const char *objectType );
extern void    NL_WriteFile( const char *path, NLNode *root, NLFileType fileType );

extern NLNode *NL_ParseBuffer( const char *buf, size_t length );

/* debugging */
extern void NL_PrintNodeTree( NLNode *node, int index );

/* deserialisation */
PLMatrix4 *       NL_DS_DeserializeMatrix4( NLNode *in, PLMatrix4 *out );
float *           NL_DS_DeserializeVector( NLNode *in, float *out, uint8_t numElements );
PLVector2 *       NL_DS_DeserializeVector2( NLNode *in, PLVector2 *out );
PLVector3 *       NL_DS_DeserializeVector3( NLNode *in, PLVector3 *out );
PLVector4 *       NL_DS_DeserializeVector4( NLNode *in, PLVector4 *out );
PLQuaternion *    NL_DS_DeserializeQuaternion( NLNode *in, PLQuaternion *out );
PLColour *        NL_DS_DeserializeColour( NLNode *in, PLColour *out );
struct PLGVertex *NL_DS_DeserializeVertex( NLNode *in, struct PLGVertex *out );

PL_EXTERN_C_END

#ifdef __cplusplus

static inline NLNode *NL_PushBackArray( NLNode *parent, const char *name, const int32_t *array, unsigned int numElements )
{
	return NL_PushBackI32Array( parent, name, array, numElements );
}

static inline NLNode *NL_PushBackArray( NLNode *parent, const char *name, const float *array, unsigned int numElements )
{
	return NL_PushBackF32Array( parent, name, array, numElements );
}

static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, const char *var )
{
	return NL_PushBackStr( parent, name, var );
}

static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, bool var )
{
	return NL_PushBackBool( parent, name, var );
}

static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, int32_t var )
{
	return NL_PushBackI32( parent, name, var );
}

static inline NLNode *NL_PushBackVariable( NLNode *parent, const char *name, float var )
{
	return NL_PushBackF32( parent, name, var );
}

#endif
