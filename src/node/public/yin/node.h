// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_filesystem.h>
#include <plcore/pl_math.h>

PL_EXTERN_C

typedef struct NdBranch NdBranch;

#define ND_DEFAULT_EXTENSION ".n"

typedef enum NdErrorCode
{
	ND_ERROR_SUCCESS,

	ND_ERROR_IO_READ,   /* read failure */
	ND_ERROR_IO_WRITE,  /* write failure */

	NL_ERROR_MEM_ALLOC, /* alloc failure */

	ND_ERROR_INVALID_ARGUMENT,
	ND_ERROR_INVALID_TYPE,     /* invalid node parent/child type */
	ND_ERROR_INVALID_ELEMENTS, /* unexpected number of elements */
} NdErrorCode;

typedef enum NdFileType
{
	ND_FILE_INVALID = -1,
	ND_FILE_BINARY,
	ND_FILE_UTF8,

	ND_MAX_FILE_TYPES
} NdFileType;

typedef enum NdPropertyType
{
	ND_PROPERTY_INVALID = -1,

	ND_PROPERTY_OBJECT,
	ND_PROPERTY_LINK, /* todo */
	ND_PROPERTY_ARRAY,

	ND_PROPERTY_STRING,
	ND_PROPERTY_BOOL,

	ND_PROPERTY_F32, // float
	ND_PROPERTY_F64, // double
	ND_PROPERTY_I8,  // int8
	ND_PROPERTY_I16, // int16
	ND_PROPERTY_I32, // int32
	ND_PROPERTY_I64, // int64
	ND_PROPERTY_UI8, // uint8
	ND_PROPERTY_UI16,// uint16
	ND_PROPERTY_UI32,// uint32
	ND_PROPERTY_UI64,// uint64

	ND_MAX_PROPERTY_TYPES
} NdPropertyType;

typedef union NdPropertyData
{
	float f32;
	double f64;
	int8_t i8;
	int16_t i16;
	int32_t i32;
	int64_t i64;
	uint8_t ui8;
	uint16_t ui16;
	uint32_t ui32;
	uint64_t ui64;
} NdPropertyData;

void ndSetupLogs( void );

const char *ndGetErrorMessage( void );
NdErrorCode ndGetError( void );

unsigned int ndGetNumOfChildren( const NdBranch *parent ); /* only valid for object/array */
NdBranch *ndGetFirstChild( NdBranch *parent );
NdBranch *ndGetNextChild( NdBranch *node );
NdBranch *ndGetChildByName( NdBranch *parent, const char *name ); /* only valid for object */
NdBranch *ndGetParent( NdBranch *node );

const char *ndGetName( const NdBranch *node );
NdPropertyType ndGetType( const NdBranch *node );

NdErrorCode ndGetBool( const NdBranch *node, bool *dest );
NdErrorCode ndGetStr( const NdBranch *node, char *dest, size_t length );
NdErrorCode ndGetF32( const NdBranch *node, float *dest );
NdErrorCode ndGetF64( const NdBranch *node, double *dest );
NdErrorCode ndGetI8( const NdBranch *node, int8_t *dest );
NdErrorCode ndGetI16( const NdBranch *node, int16_t *dest );
NdErrorCode ndGetI32( const NdBranch *node, int32_t *dest );
NdErrorCode ndGetI64( const NdBranch *node, int64_t *dest );
NdErrorCode ndGetUI8( const NdBranch *node, uint8_t *dest );
NdErrorCode ndGetUI16( const NdBranch *node, uint16_t *dest );
NdErrorCode ndGetUI32( const NdBranch *node, uint32_t *dest );
NdErrorCode ndGetUI64( const NdBranch *node, uint64_t *dest );

NdErrorCode ndGetStringArray( NdBranch *parent, const char **buf, unsigned int numElements );
NdErrorCode ndGetI8Array( NdBranch *parent, int8_t *buf, unsigned int numElements );
NdErrorCode ndGetI16Array( NdBranch *parent, int16_t *buf, unsigned int numElements );
NdErrorCode ndGetI32Array( NdBranch *parent, int32_t *buf, unsigned int numElements );
NdErrorCode ndGetUI32Array( NdBranch *parent, uint32_t *buf, unsigned int numElements );
NdErrorCode ndGetF32Array( NdBranch *parent, float *buf, unsigned int numElements );

bool ndGetBoolByName( NdBranch *root, const char *name, bool fallback );
const char *ndGetStringByName( NdBranch *node, const char *name, const char *fallback );
int32_t ndGetI32ByName( NdBranch *node, const char *name, int32_t fallback );
float ndGetF32ByName( NdBranch *node, const char *name, float fallback );
double ndGetF64ByName( NdBranch *node, const char *name, double fallback );

NdBranch *ndPushBackBranch( NdBranch *parent, NdBranch *child );
NdBranch *ndPushBackObject( NdBranch *node, const char *name );
NdBranch *ndPushBackString( NdBranch *parent, const char *name, const char *var );
NdBranch *ndPushBackBool( NdBranch *parent, const char *name, bool var );
NdBranch *ndPushBackI8( NdBranch *parent, const char *name, int8_t var );
NdBranch *ndPushBackI16( NdBranch *parent, const char *name, int16_t var );
NdBranch *ndPushBackI32( NdBranch *parent, const char *name, int32_t var );
NdBranch *ndPushBackUI32( NdBranch *parent, const char *name, uint32_t var );
NdBranch *ndPushBackF32( NdBranch *parent, const char *name, float var );
NdBranch *ndPushBackF64( NdBranch *parent, const char *name, double var );

NdBranch *ndPushBackObjectArray( NdBranch *parent, const char *name );
NdBranch *ndPushBackStringArray( NdBranch *parent, const char *name, const char **array, unsigned int numElements );
NdBranch *ndPushBackI32Array( NdBranch *parent, const char *name, const int32_t *array, unsigned int numElements );
NdBranch *ndPushBackF32Array( NdBranch *parent, const char *name, const float *array, unsigned int numElements );

NdBranch *ndCopyBranch( NdBranch *node );
void ndDestroyBranch( NdBranch *node );

NdBranch *ndParseFile( PLFile *file, const char *objectType );
NdBranch *ndLoadFile( const char *path, const char *objectType );
bool ndWriteFile( const char *path, NdBranch *root, NdFileType fileType );

NdBranch *ndParseBuffer( const char *buf, size_t length );

/* debugging */
void ndPrintTree( NdBranch *node, int index );

/* deserialisation/serialisation */

float *ndDS_DeserializeVector( NdBranch *in, float *out, uint8_t numElements );

PLVector2 *ndDS_DeserializeVector2( NdBranch *in, PLVector2 *out );
NdBranch *ndDS_SerializeVector2( NdBranch *parent, const char *name, const PLVector2 *vector2 );

PLVector3 *ndDS_DeserializeVector3( NdBranch *in, PLVector3 *out );
NdBranch *ndDS_SerializeVector3( NdBranch *parent, const char *name, const PLVector3 *vector3 );

PLVector4 *ndDS_DeserializeVector4( NdBranch *in, PLVector4 *out );
PLQuaternion *ndDS_DeserializeQuaternion( NdBranch *in, PLQuaternion *out );

PLColour *ndDS_DeserializeColour( NdBranch *in, PLColour *out );
NdBranch *ndDS_SerializeColour( NdBranch *parent, const char *name, const PLColour *colour );
PLColourF32 *ndDS_DeserializeColourF32( NdBranch *in, PLColourF32 *out );

NdBranch *ndDS_SerializeCollisionAABB( NdBranch *parent, const char *name, const struct PLCollisionAABB *collisionAabb );

PL_EXTERN_C_END
