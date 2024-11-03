// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_filesystem.h>
#include <plcore/pl_math.h>

PL_EXTERN_C

typedef unsigned char  uchar;
typedef unsigned int   uint;
typedef unsigned short ushort;

typedef struct AcmBranch AcmBranch;

#define ACM_DEFAULT_EXTENSION ".n"

typedef enum AcmErrorCode
{
	ND_ERROR_SUCCESS,

	ND_ERROR_IO_READ,  /* read failure */
	ND_ERROR_IO_WRITE, /* write failure */

	NL_ERROR_MEM_ALLOC, /* alloc failure */

	ND_ERROR_INVALID_ARGUMENT,
	ND_ERROR_INVALID_TYPE,     /* invalid node parent/child type */
	ND_ERROR_INVALID_ELEMENTS, /* unexpected number of elements */
} AcmErrorCode;

typedef enum NdFileType
{
	ND_FILE_INVALID = -1,
	ND_FILE_BINARY,
	ND_FILE_UTF8,

	ND_MAX_FILE_TYPES
} NdFileType;

typedef enum AcmPropertyType
{
	ND_PROPERTY_INVALID = -1,

	// special types
	ND_PROPERTY_OBJECT,
	ND_PROPERTY_LINK, /* todo */
	ND_PROPERTY_ARRAY,
	ND_PROPERTY_STRING,

	// standard types
	ND_PROPERTY_BOOL,
	ND_PROPERTY_FLOAT32,// float
	ND_PROPERTY_FLOAT64,// double
	ND_PROPERTY_INT8,   // int8
	ND_PROPERTY_INT16,  // int16
	ND_PROPERTY_INT32,  // int32
	ND_PROPERTY_INT64,  // int64
	ND_PROPERTY_UI8,    // uint8
	ND_PROPERTY_UI16,   // uint16
	ND_PROPERTY_UI32,   // uint32
	ND_PROPERTY_UI64,   // uint64

	ND_MAX_PROPERTY_TYPES
} AcmPropertyType;

typedef union AcmPropertyData
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
} AcmPropertyData;

/////////////////////////////////////////////////////////////////////////////////////
// Struct Serialization
/////////////////////////////////////////////////////////////////////////////////////

typedef struct AcmStructDescriptor AcmStructDescriptor;

typedef struct AcmStructItemDescriptor
{
	const char          *name;
	AcmPropertyType      type;
	AcmPropertyType      subType;         // if it's an array
	uint                 numElements;     // if it's an array
	AcmStructDescriptor *structDescriptor;// if it's an object
	size_t               offset;          // offset into the struct
} AcmStructItemDescriptor;

typedef struct AcmStructDescriptor
{
	const char             *name;
	AcmStructItemDescriptor items[ 64 ];
	uint                    numItems;
} AcmStructDescriptor;

#define ACM_DECLARE_STRUCT( NAME, NUM, ... )         \
	static AcmStructDescriptor NAME##_descriptor = { \
	        .name     = #NAME,                       \
	        .numItems = ( NUM ),                     \
	        .items    = { __VA_ARGS__ } };
#define ACM_DECLARE_STRUCT_ITEM( TYPE, VAR, DATATYPE ) \
	{                                                  \
	        .name   = #VAR,                            \
	        .type   = ( DATATYPE ),                    \
	        .offset = PL_OFFSETOF( TYPE, VAR ),        \
	}
#define ACM_DECLARE_STRUCT_ITEM_ARRAY( TYPE, VAR, SUBTYPE, SIZE ) \
	{                                                             \
	        .name        = #VAR,                                  \
	        .type        = ND_PROPERTY_ARRAY,                     \
	        .subType     = ( SUBTYPE ),                           \
	        .numElements = ( SIZE ),                              \
	        .offset      = PL_OFFSETOF( TYPE, VAR ),              \
	}
#define ND_DECLARE_STRUCT_ITEM_OBJECT( NAME, STRUCT )

AcmBranch *acm_serialize_struct( const AcmStructDescriptor *descriptor, const void *ptr, AcmErrorCode *errorCode );
AcmBranch *acm_deserialize_struct( const AcmStructDescriptor *descriptor, void *ptr, AcmErrorCode *errorCode );

/////////////////////////////////////////////////////////////////////////////////////

void acm_setup_logs( void );

const char  *acm_get_error_message( void );
AcmErrorCode acm_get_error( void );

uint       acm_branch_get_num_of_children( const AcmBranch *self ); /* only valid for object/array */
AcmBranch *acm_branch_get_first_child( AcmBranch *self );
AcmBranch *acm_branch_get_child_by_name( AcmBranch *self, const char *name ); /* only valid for object */
AcmBranch *acm_branch_get_parent( AcmBranch *self );

AcmBranch *acm_get_next_child( AcmBranch *node );

const char     *acm_branch_get_name( const AcmBranch *self );
AcmPropertyType acm_branch_get_type( const AcmBranch *self );

AcmErrorCode acm_branch_get_bool( const AcmBranch *self, bool *dest );
AcmErrorCode acm_branch_get_string( const AcmBranch *self, char *dest, size_t length );
AcmErrorCode acm_branch_get_float32( const AcmBranch *self, float *dest );
AcmErrorCode acm_branch_get_float64( const AcmBranch *self, double *dest );
AcmErrorCode acm_branch_get_int8( const AcmBranch *self, int8_t *dest );
AcmErrorCode acm_branch_get_int16( const AcmBranch *self, int16_t *dest );
AcmErrorCode acm_branch_get_int32( const AcmBranch *self, int32_t *dest );
AcmErrorCode acm_branch_get_int64( const AcmBranch *self, int64_t *dest );
AcmErrorCode acm_branch_get_uint8( const AcmBranch *self, uint8_t *dest );
AcmErrorCode acm_branch_get_uint16( const AcmBranch *self, uint16_t *dest );
AcmErrorCode acm_branch_get_uint32( const AcmBranch *self, uint32_t *dest );
AcmErrorCode acm_branch_get_uint64( const AcmBranch *self, uint64_t *dest );

AcmErrorCode acm_branch_get_bool_array( AcmBranch *self, bool *buf, uint numElements );
AcmErrorCode acm_branch_get_string_array( AcmBranch *self, char **buf, uint numElements );
AcmErrorCode acm_branch_get_int8_array( AcmBranch *self, int8_t *buf, uint numElements );
AcmErrorCode acm_branch_get_int16_array( AcmBranch *self, int16_t *buf, uint numElements );
AcmErrorCode acm_branch_get_int32_array( AcmBranch *self, int32_t *buf, uint numElements );
AcmErrorCode acm_branch_get_uint32_array( AcmBranch *self, uint32_t *buf, uint numElements );
AcmErrorCode acm_branch_get_float32_array( AcmBranch *self, float *buf, uint numElements );
AcmErrorCode acm_branch_get_float64_array( AcmBranch *self, double *buf, uint numElements );

bool        acm_get_bool( AcmBranch *root, const char *name, bool fallback );
const char *acm_branch_get_child_string( AcmBranch *node, const char *name, const char *fallback );
float       acm_get_f32( AcmBranch *node, const char *name, float fallback );
double      acm_get_f64( AcmBranch *node, const char *name, double fallback );

intmax_t  acm_branch_get_child_int( AcmBranch *root, const char *name, intmax_t fallback );
uintmax_t acm_get_uint( AcmBranch *root, const char *name, uintmax_t fallback );

#define ACM_GET_INT8( ROOT, NAME, FALLBACK )  ( int8_t ) acm_branch_get_child_int( ( ROOT ), ( NAME ), ( FALLBACK ) )
#define ACM_GET_INT16( ROOT, NAME, FALLBACK ) ( int16_t ) acm_branch_get_child_int( ( ROOT ), ( NAME ), ( FALLBACK ) )
#define ACM_GET_INT32( ROOT, NAME, FALLBACK ) ( int32_t ) acm_branch_get_child_int( ( ROOT ), ( NAME ), ( FALLBACK ) )

#define ACM_GET_UINT8( ROOT, NAME, FALLBACK )  ( uint8_t ) acm_branch_get_child_uint( ( ROOT ), ( NAME ), ( FALLBACK ) )
#define ACM_GET_UINT16( ROOT, NAME, FALLBACK ) ( uint16_t ) acm_branch_get_child_uint( ( ROOT ), ( NAME ), ( FALLBACK ) )
#define ACM_GET_UINT32( ROOT, NAME, FALLBACK ) ( uint32_t ) acm_branch_get_child_uint( ( ROOT ), ( NAME ), ( FALLBACK ) )

PLVector2   acm_get_vector2( AcmBranch *root, const char *name, const PLVector2 *fallback );
PLVector3   acm_get_vector3( AcmBranch *root, const char *name, const PLVector3 *fallback );
PLVector4   acm_get_vector4( AcmBranch *root, const char *name, const PLVector4 *fallback );
PLColourF32 acm_get_colour_f32( AcmBranch *root, const char *name, const PLColourF32 *fallback );

AcmBranch *acm_branch_push_back_branch( AcmBranch *parent, AcmBranch *child );
AcmBranch *acm_branch_push_back_object( AcmBranch *node, const char *name );
AcmBranch *acm_push_string( AcmBranch *parent, const char *name, const char *var, bool conditional );
AcmBranch *acm_branch_push_back_bool( AcmBranch *parent, const char *name, bool var );
AcmBranch *acm_branch_push_back_int8( AcmBranch *parent, const char *name, int8_t var );
AcmBranch *acm_branch_push_back_int16( AcmBranch *parent, const char *name, int16_t var );
AcmBranch *acm_branch_push_back_int32( AcmBranch *parent, const char *name, int32_t var );
AcmBranch *acm_push_uint32( AcmBranch *parent, const char *name, uint32_t var );
AcmBranch *acm_branch_push_back_float32( AcmBranch *parent, const char *name, float var );
AcmBranch *acm_branch_push_back_float64( AcmBranch *parent, const char *name, double var );

AcmBranch *acm_branch_push_back_object_array( AcmBranch *parent, const char *name );
AcmBranch *acm_branch_push_back_string_array( AcmBranch *parent, const char *name, const char **array, uint numElements );
AcmBranch *acm_branch_push_back_int16_array( AcmBranch *root, const char *name, const int16_t *array, uint numElements );
AcmBranch *acm_branch_push_back_int32_array( AcmBranch *parent, const char *name, const int32_t *array, uint numElements );
AcmBranch *acm_branch_push_back_uint32_array( AcmBranch *parent, const char *name, const uint32_t *array, uint numElements );
AcmBranch *acm_push_array_f32( AcmBranch *parent, const char *name, const float *array, uint numElements );

// special
AcmBranch *acm_branch_push_back_vector2( AcmBranch *parent, const char *name, const PLVector2 *vector, bool conditional );
AcmBranch *acm_branch_push_back_vector3( AcmBranch *parent, const char *name, const PLVector3 *vector, bool conditional );
AcmBranch *acm_branch_push_back_vector4( AcmBranch *parent, const char *name, const PLVector4 *vector, bool conditional );

AcmBranch *acm_copy_branch( AcmBranch *node );
void       acm_branch_destroy( AcmBranch *node );

AcmBranch *acm_parse_file( PLFile *file, const char *objectType );
AcmBranch *acm_load_file( const char *path, const char *objectType );
bool       acm_write_file( const char *path, AcmBranch *root, NdFileType fileType );

AcmBranch *acm_parse_buffer( const char *buf, size_t length );

/* debugging */
void acm_branch_print_tree( AcmBranch *self, int index );

PL_EXTERN_C_END
