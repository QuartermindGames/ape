// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_filesystem.h>
#include <plcore/pl_math.h>

PL_EXTERN_C

typedef struct NdBranch NdBranch;

#define ND_DEFAULT_EXTENSION ".n"

typedef enum NdErrorCode
{
	ND_ERROR_SUCCESS,

	ND_ERROR_IO_READ,  /* read failure */
	ND_ERROR_IO_WRITE, /* write failure */

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

/////////////////////////////////////////////////////////////////////////////////////
// Struct Serialization
/////////////////////////////////////////////////////////////////////////////////////

typedef struct NdStructDescriptor NdStructDescriptor;

typedef struct NdStructItemDescriptor
{
	const char *name;
	NdPropertyType type;
	NdPropertyType subType;              // if it's an array
	unsigned int numElements;            // if it's an array
	NdStructDescriptor *structDescriptor;// if it's an object
	size_t offset;                       // offset into the struct
} NdStructItemDescriptor;

typedef struct NdStructDescriptor
{
	const char *name;
	NdStructItemDescriptor items[ 64 ];
	unsigned int numItems;
} NdStructDescriptor;

#define ND_DECLARE_STRUCT( NAME, NUM, ... )         \
	static NdStructDescriptor NAME##_descriptor = { \
	        .name = #NAME,                          \
	        .numItems = ( NUM ),                    \
	        .items = { __VA_ARGS__ } };
#define ND_DECLARE_STRUCT_ITEM( TYPE, VAR, DATATYPE ) \
	{                                                 \
		.name = #VAR,                                 \
		.type = ( DATATYPE ),                         \
		.offset = PL_OFFSETOF( TYPE, VAR ),           \
	}
#define ND_DECLARE_STRUCT_ITEM_ARRAY( TYPE, VAR, SUBTYPE, SIZE ) \
	{                                                            \
		.name = #VAR,                                            \
		.type = ND_PROPERTY_ARRAY,                               \
		.subType = ( SUBTYPE ),                                  \
		.numElements = ( SIZE ),                                 \
		.offset = PL_OFFSETOF( TYPE, VAR ),                      \
	}
#define ND_DECLARE_STRUCT_ITEM_OBJECT( NAME, STRUCT )

NdBranch *nd_serialize_struct( const NdStructDescriptor *descriptor, const void *ptr, NdErrorCode *errorCode );
NdBranch *nd_deserialize_struct( const NdStructDescriptor *descriptor, void *ptr, NdErrorCode *errorCode );

/////////////////////////////////////////////////////////////////////////////////////

void nd_setup_logs( void );

const char *nd_get_error_message( void );
NdErrorCode nd_get_error( void );

unsigned int nd_branch_get_num_of_children( const NdBranch *self ); /* only valid for object/array */
NdBranch *nd_branch_get_first_child( NdBranch *self );
NdBranch *nd_branch_get_child_by_name( NdBranch *self, const char *name ); /* only valid for object */
NdBranch *nd_branch_get_parent( NdBranch *self );

NdBranch *nd_get_next_child( NdBranch *node );

const char *nd_branch_get_name( const NdBranch *self );
NdPropertyType nd_branch_get_type( const NdBranch *self );

NdErrorCode nd_branch_get_bool( const NdBranch *self, bool *dest );
NdErrorCode nd_branch_get_string( const NdBranch *self, char *dest, size_t length );
NdErrorCode nd_branch_get_float32( const NdBranch *self, float *dest );
NdErrorCode nd_branch_get_float64( const NdBranch *self, double *dest );
NdErrorCode nd_branch_get_int8( const NdBranch *self, int8_t *dest );
NdErrorCode nd_branch_get_int16( const NdBranch *self, int16_t *dest );
NdErrorCode nd_branch_get_int32( const NdBranch *self, int32_t *dest );
NdErrorCode nd_branch_get_int64( const NdBranch *self, int64_t *dest );
NdErrorCode nd_branch_get_uint8( const NdBranch *self, uint8_t *dest );
NdErrorCode nd_branch_get_uint16( const NdBranch *self, uint16_t *dest );
NdErrorCode nd_branch_get_uint32( const NdBranch *self, uint32_t *dest );
NdErrorCode nd_branch_get_uint64( const NdBranch *self, uint64_t *dest );

NdErrorCode nd_branch_get_bool_array( NdBranch *self, bool *buf, unsigned int numElements );
NdErrorCode nd_branch_get_string_array( NdBranch *self, char **buf, unsigned int numElements );
NdErrorCode nd_branch_get_int8_array( NdBranch *self, int8_t *buf, unsigned int numElements );
NdErrorCode nd_branch_get_int16_array( NdBranch *self, int16_t *buf, unsigned int numElements );
NdErrorCode nd_branch_get_int32_array( NdBranch *self, int32_t *buf, unsigned int numElements );
NdErrorCode nd_branch_get_uint32_array( NdBranch *self, uint32_t *buf, unsigned int numElements );
NdErrorCode nd_branch_get_float32_array( NdBranch *self, float *buf, unsigned int numElements );
NdErrorCode nd_branch_get_float64_array( NdBranch *self, double *buf, unsigned int numElements );

bool nd_branch_get_child_bool( NdBranch *root, const char *name, bool fallback );
const char *nd_branch_get_child_string( NdBranch *node, const char *name, const char *fallback );
float nd_branch_get_child_float32( NdBranch *node, const char *name, float fallback );
double nd_branch_get_child_float64( NdBranch *node, const char *name, double fallback );

intmax_t nd_branch_get_child_int( NdBranch *root, const char *name, intmax_t fallback );
uintmax_t nd_branch_get_child_uint( NdBranch *root, const char *name, uintmax_t fallback );

#define ND_GET_INT8( ROOT, NAME, FALLBACK )  ( int8_t ) nd_branch_get_child_int( ( ROOT ), ( NAME ), ( FALLBACK ) )
#define ND_GET_INT16( ROOT, NAME, FALLBACK ) ( int16_t ) nd_branch_get_child_int( ( ROOT ), ( NAME ), ( FALLBACK ) )
#define ND_GET_INT32( ROOT, NAME, FALLBACK ) ( int32_t ) nd_branch_get_child_int( ( ROOT ), ( NAME ), ( FALLBACK ) )

#define ND_GET_UINT8( ROOT, NAME, FALLBACK )  ( uint8_t ) nd_branch_get_child_uint( ( ROOT ), ( NAME ), ( FALLBACK ) )
#define ND_GET_UINT16( ROOT, NAME, FALLBACK ) ( uint16_t ) nd_branch_get_child_uint( ( ROOT ), ( NAME ), ( FALLBACK ) )
#define ND_GET_UINT32( ROOT, NAME, FALLBACK ) ( uint32_t ) nd_branch_get_child_uint( ( ROOT ), ( NAME ), ( FALLBACK ) )

PLVector2 nd_get_vector2( NdBranch *root, const char *name, const PLVector2 *fallback );
PLVector3 nd_get_vector3( NdBranch *root, const char *name, const PLVector3 *fallback );
PLVector4 nd_get_vector4( NdBranch *root, const char *name, const PLVector4 *fallback );
PLColourF32 nd_get_colour_f32( NdBranch *root, const char *name, const PLColourF32 *fallback );

NdBranch *nd_branch_push_back_branch( NdBranch *parent, NdBranch *child );
NdBranch *nd_branch_push_back_object( NdBranch *node, const char *name );
NdBranch *nd_branch_push_back_string( NdBranch *parent, const char *name, const char *var );
NdBranch *nd_branch_push_back_bool( NdBranch *parent, const char *name, bool var );
NdBranch *nd_branch_push_back_int8( NdBranch *parent, const char *name, int8_t var );
NdBranch *nd_branch_push_back_int16( NdBranch *parent, const char *name, int16_t var );
NdBranch *nd_branch_push_back_int32( NdBranch *parent, const char *name, int32_t var );
NdBranch *nd_branch_push_back_uint32( NdBranch *parent, const char *name, uint32_t var );
NdBranch *nd_branch_push_back_float32( NdBranch *parent, const char *name, float var );
NdBranch *nd_branch_push_back_float64( NdBranch *parent, const char *name, double var );

NdBranch *nd_branch_push_back_object_array( NdBranch *parent, const char *name );
NdBranch *nd_branch_push_back_string_array( NdBranch *parent, const char *name, const char **array, unsigned int numElements );
NdBranch *nd_branch_push_back_int16_array( NdBranch *root, const char *name, const int16_t *array, unsigned int numElements );
NdBranch *nd_branch_push_back_int32_array( NdBranch *parent, const char *name, const int32_t *array, unsigned int numElements );
NdBranch *nd_branch_push_back_uint32_array( NdBranch *parent, const char *name, const uint32_t *array, unsigned int numElements );
NdBranch *nd_branch_push_back_float32_array( NdBranch *parent, const char *name, const float *array, unsigned int numElements );

NdBranch *nd_copy_branch( NdBranch *node );
void nd_branch_destroy( NdBranch *node );

NdBranch *nd_parse_file( PLFile *file, const char *objectType );
NdBranch *nd_load_file( const char *path, const char *objectType );
bool nd_write_file( const char *path, NdBranch *root, NdFileType fileType );

NdBranch *nd_parse_buffer( const char *buf, size_t length );

/* debugging */
void nd_branch_print_tree( NdBranch *self, int index );

PL_EXTERN_C_END
