// SPDX-License-Identifier: MIT
// Ape Config Markup
// Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>

#include "acm_private.h"

int  nd_LogLevelPrint_ = -1;
int  nd_LogLevelWarn_  = -1;
void acm_setup_logs( void )
{
	nd_LogLevelPrint_ = PlAddLogLevel( "node", PL_COLOUR_DARK_SLATE_BLUE, true );
	nd_LogLevelWarn_  = PlAddLogLevel( "node/warning", PL_COLOUR_YELLOW, true );
	Message( "Logs are now active for NODE library\n" );
}

#define ND_FORMAT_UTF8_HEADER "node.utf8"

#define ND_FORMAT_BINARY_HEADER   "node.bin" // original format w/ no versioning support (defaults to 1)
#define ND_FORMAT_BINARY_HEADER_2 "node.binx"// new format w/ versioning support
#define ND_FORMAT_BINARY_VERSION  1

static const char *string_for_property_type( AcmPropertyType propertyType )
{
	const char *propToStr[ ACM_MAX_PROPERTY_TYPES ] = {
	        // Special types
	        [ACM_PROPERTY_TYPE_OBJECT] = "object",
	        [ACM_PROPERTY_TYPE_STRING] = "string",
	        [ACM_PROPERTY_TYPE_BOOL]   = "bool",
	        [ACM_PROPERTY_TYPE_ARRAY]  = "array",
	        // Generic types
	        [ND_PROPERTY_INT8]          = "int8",
	        [ND_PROPERTY_INT16]         = "int16",
	        [ND_PROPERTY_INT32]         = "int32",
	        [ND_PROPERTY_INT64]         = "int64",
	        [ND_PROPERTY_UI8]           = "uint8",
	        [ND_PROPERTY_UI16]          = "uint16",
	        [ND_PROPERTY_UI32]          = "uint32",
	        [ND_PROPERTY_UI64]          = "uint64",
	        [ACM_PROPERTY_TYPE_FLOAT32] = "float",
	        [ACM_PROPERTY_TYPE_FLOAT64] = "float64",
	};

	if ( propertyType == ACM_PROPERTY_TYPE_INVALID )
	{
		return "invalid";
	}

	return propToStr[ propertyType ];
}

static char        *nlErrorMsg  = NULL;
static AcmErrorCode nlErrorType = ND_ERROR_SUCCESS;
static void         clear_error_message( void )
{
	PlFree( nlErrorMsg );
	nlErrorMsg  = NULL;
	nlErrorType = ND_ERROR_SUCCESS;
}

static void set_error_message( AcmErrorCode type, const char *msg, ... )
{
	clear_error_message();

	nlErrorType = type;

	va_list args;
	va_start( args, msg );

	int length = pl_vscprintf( msg, args ) + 1;
	if ( length <= 0 )
		return;

	nlErrorMsg = PlCAlloc( 1, length, false );
	if ( nlErrorMsg == NULL )
	{
		Warning( "Failed to allocate error message buffer: %d bytes!\n", length );
		return;
	}

	vsnprintf( nlErrorMsg, length, msg, args );
	Warning( "NLERR: %s\n", nlErrorMsg );

	va_end( args );
}

const char  *acm_get_error_message( void ) { return nlErrorMsg; }
AcmErrorCode acm_get_error( void ) { return nlErrorType; }

static char *alloc_var_string( const char *string, uint16_t *lengthOut )
{
	*lengthOut = ( uint16_t ) strlen( string ) + 1;
	char *buf  = PlCAllocA( 1, *lengthOut );
	strcpy( buf, string );
	return buf;
}

unsigned int acm_get_num_of_children( const AcmBranch *self )
{
	return PlGetNumLinkedListNodes( self->linkedList );
}

AcmBranch *acm_get_first_child( AcmBranch *self )
{
	PLLinkedListNode *n = PlGetFirstNode( self->linkedList );
	if ( n == NULL )
	{
		return NULL;
	}

	return PlGetLinkedListNodeUserData( n );
}

AcmBranch *acm_get_next_child( AcmBranch *node )
{
	PLLinkedListNode *n = PlGetNextLinkedListNode( node->linkedListNode );
	if ( n == NULL )
		return NULL;

	return PlGetLinkedListNodeUserData( n );
}

AcmBranch *acm_get_child_by_name( AcmBranch *self, const char *name )
{
	if ( self->type != ACM_PROPERTY_TYPE_OBJECT )
	{
		set_error_message( ND_ERROR_INVALID_TYPE, "Attempted to get child from an invalid node type!\n" );
		return NULL;
	}

	AcmBranch *child = acm_get_first_child( self );
	while ( child != NULL )
	{
		if ( strcmp( name, child->name.buf ) == 0 )
		{
			return child;
		}

		child = acm_get_next_child( child );
	}

	return NULL;
}

static const NdVarString *get_value_by_name( AcmBranch *root, const char *name )
{
	const AcmBranch *field = acm_get_child_by_name( root, name );
	if ( field == NULL )
	{
		return NULL;
	}

	return &field->data;
}

AcmBranch *acm_get_parent( AcmBranch *self )
{
	return self->parent;
}

const char *acm_branch_get_name( const AcmBranch *self )
{
	return self->name.buf;
}

AcmPropertyType acm_branch_get_type( const AcmBranch *self )
{
	return self->type;
}

AcmErrorCode acm_branch_get_string( const AcmBranch *self, char *dest, size_t length )
{
	if ( self->type != ACM_PROPERTY_TYPE_STRING ) return ND_ERROR_INVALID_TYPE;
	snprintf( dest, length, "%s", self->data.buf );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_bool( const AcmBranch *self, bool *dest )
{
	if ( self->type != ACM_PROPERTY_TYPE_BOOL ) return ND_ERROR_INVALID_TYPE;

	if ( ( strcmp( self->data.buf, "true" ) == 0 ) || ( self->data.buf[ 0 ] == '1' && self->data.buf[ 1 ] == '\0' ) )
	{
		*dest = true;
		return ND_ERROR_SUCCESS;
	}
	else if ( ( strcmp( self->data.buf, "false" ) == 0 ) || ( self->data.buf[ 0 ] == '0' && self->data.buf[ 1 ] == '\0' ) )
	{
		*dest = false;
		return ND_ERROR_SUCCESS;
	}

	set_error_message( ND_ERROR_INVALID_ARGUMENT, "Invalid data passed from var" );
	return ND_ERROR_INVALID_ARGUMENT;
}

AcmErrorCode acm_branch_get_float32( const AcmBranch *self, float *dest )
{
	if ( self->type != ACM_PROPERTY_TYPE_FLOAT32 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtof( self->data.buf, NULL );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_float64( const AcmBranch *self, double *dest )
{
	if ( self->type != ACM_PROPERTY_TYPE_FLOAT64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtod( self->data.buf, NULL );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int8( const AcmBranch *self, int8_t *dest )
{
	if ( self->type != ND_PROPERTY_INT8 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int8_t ) strtol( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int16( const AcmBranch *self, int16_t *dest )
{
	if ( self->type != ND_PROPERTY_INT16 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int16_t ) strtol( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int32( const AcmBranch *self, int32_t *dest )
{
	if ( self->type != ND_PROPERTY_INT32 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int32_t ) strtol( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int64( const AcmBranch *self, int64_t *dest )
{
	if ( self->type != ND_PROPERTY_INT64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoll( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint8( const AcmBranch *self, uint8_t *dest )
{
	if ( self->type != ND_PROPERTY_UI8 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( uint8_t ) strtoul( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint16( const AcmBranch *self, uint16_t *dest )
{
	if ( self->type != ND_PROPERTY_UI16 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( uint16_t ) strtoul( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint32( const AcmBranch *self, uint32_t *dest )
{
	if ( self->type != ND_PROPERTY_UI32 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoul( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint64( const AcmBranch *self, uint64_t *dest )
{
	if ( self->type != ND_PROPERTY_UI64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoull( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_string_array( AcmBranch *self, char **buf, uint numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ACM_PROPERTY_TYPE_STRING )
	{
		return ND_ERROR_INVALID_TYPE;
	}

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
		{
			return ND_ERROR_INVALID_ELEMENTS;
		}

		buf[ i ] = PL_NEW_( char, child->data.length + 1 );
		strncpy( buf[ i ], child->data.buf, child->data.length );

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_bool_array( AcmBranch *self, bool *buf, uint numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ACM_PROPERTY_TYPE_FLOAT64 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_bool( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int8_array( AcmBranch *self, int8_t *buf, uint numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ND_PROPERTY_INT8 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_int8( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int16_array( AcmBranch *self, int16_t *buf, uint numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ND_PROPERTY_INT16 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_int16( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int32_array( AcmBranch *self, int32_t *buf, uint numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ND_PROPERTY_INT32 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_int32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint32_array( AcmBranch *self, uint32_t *buf, uint numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ND_PROPERTY_UI32 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_uint32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_float32_array( AcmBranch *self, float *buf, uint numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ACM_PROPERTY_TYPE_FLOAT32 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_float32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
		{
			return errorCode;
		}

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_float64_array( AcmBranch *self, double *buf, uint numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ACM_PROPERTY_TYPE_FLOAT64 )
	{
		return ND_ERROR_INVALID_TYPE;
	}

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
		{
			return ND_ERROR_INVALID_ELEMENTS;
		}

		AcmErrorCode errorCode = acm_branch_get_float64( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

/******************************************/
/** Get: ByName **/

bool acm_get_bool( AcmBranch *root, const char *name, bool fallback )
{
	const AcmBranch *child = acm_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return fallback;
	}

	bool out;
	if ( acm_branch_get_bool( child, &out ) != ND_ERROR_SUCCESS )
	{
		return fallback;
	}

	return out;
}

const char *acm_get_string( AcmBranch *node, const char *name, const char *fallback )
{
	/* todo: warning on fail */
	const NdVarString *var = get_value_by_name( node, name );
	return ( var != NULL ) ? var->buf : fallback;
}

float acm_get_f32( AcmBranch *node, const char *name, float fallback )
{
	return ( float ) acm_get_f64( node, name, fallback );
}

float *acm_get_array_f32( AcmBranch *branch, const char *name, float *destination, uint32_t numElements )
{
	AcmBranch *child = acm_get_child_by_name( branch, name );
	if ( child == NULL )
	{
		return NULL;
	}

	if ( acm_branch_get_float32_array( child, destination, numElements ) != ND_ERROR_SUCCESS )
	{
		return NULL;
	}

	return destination;
}

double acm_get_f64( AcmBranch *node, const char *name, double fallback )
{
	/* todo: warning on fail */
	const NdVarString *var = get_value_by_name( node, name );
	return ( var != NULL ) ? strtod( var->buf, NULL ) : fallback;
}

intmax_t acm_branch_get_child_int( AcmBranch *root, const char *name, intmax_t fallback )
{
	const NdVarString *var = get_value_by_name( root, name );
	return ( var != NULL ) ? strtoll( var->buf, NULL, 10 ) : fallback;
}

uintmax_t acm_get_uint( AcmBranch *root, const char *name, uintmax_t fallback )
{
	const NdVarString *var = get_value_by_name( root, name );
	return ( var != NULL ) ? strtoull( var->buf, NULL, 10 ) : fallback;
}

int16_t *acm_get_array_i16( AcmBranch *branch, const char *name, int16_t *destination, uint numElements )
{
	AcmBranch *child = acm_get_child_by_name( branch, name );
	if ( child == NULL )
	{
		return NULL;
	}

	if ( acm_branch_get_int16_array( child, destination, numElements ) != ND_ERROR_SUCCESS )
	{
		return NULL;
	}

	return destination;
}

PLVector2 acm_get_vector2( AcmBranch *root, const char *name, const PLVector2 *fallback )
{
	AcmBranch *child = acm_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return *fallback;
	}

	PLVector2 v;
	if ( acm_branch_get_float32_array( child, ( float * ) &v, 2 ) != ND_ERROR_SUCCESS )
	{
		return *fallback;
	}

	return v;
}

PLVector3 acm_get_vector3( AcmBranch *root, const char *name, const PLVector3 *fallback )
{
	AcmBranch *child = acm_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return *fallback;
	}

	PLVector3 v;
	if ( acm_branch_get_float32_array( child, ( float * ) &v, 3 ) != ND_ERROR_SUCCESS )
	{
		return *fallback;
	}

	return v;
}

PLVector4 acm_get_vector4( AcmBranch *root, const char *name, const PLVector4 *fallback )
{
	AcmBranch *child = acm_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return *fallback;
	}

	PLVector4 v;
	if ( acm_branch_get_float32_array( child, ( float * ) &v, 4 ) != ND_ERROR_SUCCESS )
	{
		return *fallback;
	}

	return v;
}

PLColourF32 acm_get_colour_f32( AcmBranch *root, const char *name, const PLColourF32 *fallback )
{
	PLVector4 v = acm_get_vector4( root, name, ( PLVector4 * ) fallback );
	return PlVector4ToColourF32( &v );
}

AcmBranch *acm_linear_lookup( AcmBranch *root, const char *name )
{
	if ( root->name.buf != NULL && ( pl_strcasecmp( root->name.buf, name ) == 0 ) )
	{
		return root;
	}

	AcmBranch *child;
	PL_ITERATE_LINKED_LIST( child, AcmBranch, root->linkedList, i )
	{
		AcmBranch *c = acm_linear_lookup( child, name );
		if ( c != NULL )
		{
			return c;
		}
	}

	return NULL;
}

/******************************************/

AcmBranch *acm_push_new_branch( AcmBranch *parent, const char *name, AcmPropertyType propertyType, AcmPropertyType childType )
{
	/* arrays are special cases */
	if ( parent != NULL && parent->type == ACM_PROPERTY_TYPE_ARRAY && propertyType != parent->childType )
	{
		set_error_message( ND_ERROR_INVALID_TYPE, "attempted to add invalid type (%s)", string_for_property_type( propertyType ) );
		return NULL;
	}

	AcmBranch *node = PL_NEW( AcmBranch );

	/* assign the node name, if provided */
	if ( ( parent == NULL || parent->type != ACM_PROPERTY_TYPE_ARRAY ) && name != NULL )
	{
		node->name.buf = alloc_var_string( name, &node->name.length );
	}

	node->type       = propertyType;
	node->childType  = childType;// only matters for array
	node->linkedList = PlCreateLinkedList();

	/* if root is provided, this is treated as a child of that node */
	if ( parent != NULL )
	{
		if ( parent->linkedList == NULL )
		{
			parent->linkedList = PlCreateLinkedList();
		}

		node->linkedListNode = PlInsertLinkedListNode( parent->linkedList, node );
		node->parent         = parent;
	}

	return node;
}

AcmBranch *acm_push_variable_( AcmBranch *parent, const char *name, const char *value, AcmPropertyType type )
{
	AcmBranch *branch = acm_push_new_branch( parent, name, type, ACM_PROPERTY_TYPE_INVALID );
	if ( branch == NULL )
	{
		return NULL;
	}

	branch->data.buf = alloc_var_string( value, &branch->data.length );
	return branch;
}

AcmBranch *acm_push_branch( AcmBranch *parent, AcmBranch *child )
{
	AcmBranch *childCopy      = acm_copy_branch( child );
	childCopy->parent         = parent;
	childCopy->linkedListNode = PlInsertLinkedListNode( parent->linkedList, childCopy );
	return childCopy;
}

AcmBranch *acm_push_object( AcmBranch *node, const char *name )
{
	return acm_push_new_branch( node, name, ACM_PROPERTY_TYPE_OBJECT, ACM_PROPERTY_TYPE_INVALID );
}

AcmBranch *acm_push_string( AcmBranch *parent, const char *name, const char *var, bool conditional )
{
	if ( conditional && *var == '\0' )
	{
		return NULL;
	}

	return acm_push_variable_( parent, name, var, ACM_PROPERTY_TYPE_STRING );
}

AcmBranch *acm_push_array_string( AcmBranch *parent, const char *name, const char **array, uint numElements )
{
	AcmBranch *node = acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ACM_PROPERTY_TYPE_STRING );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_string( node, NULL, array[ i ], false );
		}
	}
	return node;
}

AcmBranch *acm_push_bool( AcmBranch *parent, const char *name, bool var )
{
	return acm_push_variable_( parent, name, var ? "true" : "false", ACM_PROPERTY_TYPE_BOOL );
}

AcmBranch *acm_push_i8( AcmBranch *parent, const char *name, int8_t var )
{
	char buf[ 4 ];
	pl_itoa( var, buf, sizeof( buf ), 10 );
	return acm_push_variable_( parent, name, buf, ND_PROPERTY_INT8 );
}

AcmBranch *acm_push_i16( AcmBranch *parent, const char *name, int16_t var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), PL_FMT_int16, var );
	return acm_push_variable_( parent, name, buf, ND_PROPERTY_INT16 );
}

AcmBranch *acm_push_i32( AcmBranch *parent, const char *name, int32_t var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), PL_FMT_int32, var );
	return acm_push_variable_( parent, name, buf, ND_PROPERTY_INT32 );
}

AcmBranch *acm_push_ui32( AcmBranch *parent, const char *name, uint32_t var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), PL_FMT_uint32, var );
	return acm_push_variable_( parent, name, buf, ND_PROPERTY_UI32 );
}

AcmBranch *acm_push_f32( AcmBranch *parent, const char *name, float var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), PL_FMT_float, var );
	return acm_push_variable_( parent, name, buf, ACM_PROPERTY_TYPE_FLOAT32 );
}

AcmBranch *acm_push_f64( AcmBranch *parent, const char *name, double var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), PL_FMT_double, var );
	return acm_push_variable_( parent, name, buf, ACM_PROPERTY_TYPE_FLOAT64 );
}

AcmBranch *acm_push_array_i16( AcmBranch *root, const char *name, const int16_t *array, uint numElements )
{
	AcmBranch *node = acm_push_new_branch( root, name, ACM_PROPERTY_TYPE_ARRAY, ND_PROPERTY_INT16 );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_i16( node, NULL, array[ i ] );
		}
	}
	return node;
}

AcmBranch *acm_push_array_i32( AcmBranch *parent, const char *name, const int32_t *array, uint numElements )
{
	AcmBranch *node = acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ND_PROPERTY_INT32 );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_i32( node, NULL, array[ i ] );
		}
	}
	return node;
}

AcmBranch *acm_push_array_ui32( AcmBranch *parent, const char *name, const uint32_t *array, uint numElements )
{
	AcmBranch *node = acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ND_PROPERTY_UI32 );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_ui32( node, NULL, array[ i ] );
		}
	}
	return node;
}

AcmBranch *acm_push_array_f32( AcmBranch *parent, const char *name, const float *array, uint numElements )
{
	AcmBranch *node = acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ACM_PROPERTY_TYPE_FLOAT32 );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_f32( node, NULL, array[ i ] );
		}
	}
	return node;
}

AcmBranch *acm_push_vector2( AcmBranch *parent, const char *name, const PLVector2 *vector, bool conditional )
{
	if ( conditional && PlCompareVector2( vector, &pl_vecOrigin2 ) )
	{
		return NULL;
	}

	return acm_push_array_f32( parent, name, ( float * ) vector, 2 );
}

AcmBranch *acm_push_vector3( AcmBranch *parent, const char *name, const PLVector3 *vector, bool conditional )
{
	if ( conditional && PlCompareVector3( vector, &pl_vecOrigin3 ) )
	{
		return NULL;
	}

	return acm_push_array_f32( parent, name, ( float * ) vector, 3 );
}

AcmBranch *acm_push_vector4( AcmBranch *parent, const char *name, const PLVector4 *vector, bool conditional )
{
	if ( conditional && PlCompareVector4( vector, &pl_vecOrigin4 ) )
	{
		return NULL;
	}

	return acm_push_array_f32( parent, name, ( float * ) vector, 4 );
}

AcmBranch *acm_push_colour4f( AcmBranch *parent, const char *name, const PLColourF32 *colour, bool conditional )
{
	if ( conditional && PlColour4fCompare( colour, &PL_COLOURF32( 0.0f, 0.0f, 0.0f, 0.0f ) ) )
	{
		return NULL;
	}

	return acm_push_array_f32( parent, name, ( float * ) colour, 4 );
}

AcmBranch *acm_push_array_object( AcmBranch *parent, const char *name )
{
	return acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ACM_PROPERTY_TYPE_OBJECT );
}

static char *copy_var_string( const NdVarString *varString, uint16_t *length )
{
	*length   = varString->length;
	char *buf = PL_NEW_( char, *length + 1 );
	strncpy( buf, varString->buf, *length );
	return buf;
}

/**
 * Copies the given node list.
 */
AcmBranch *acm_copy_branch( AcmBranch *node )
{
	AcmBranch *newNode = PL_NEW( AcmBranch );
	newNode->type      = node->type;
	newNode->childType = node->childType;
	newNode->data.buf  = copy_var_string( &node->data, &newNode->data.length );
	newNode->name.buf  = copy_var_string( &node->name, &newNode->name.length );
	// Not setting the parent is intentional here, since we likely don't want that link

	AcmBranch *child = acm_get_first_child( node );
	while ( child != NULL )
	{
		if ( newNode->linkedList == NULL )
			newNode->linkedList = PlCreateLinkedList();

		AcmBranch *newChild      = acm_copy_branch( child );
		newChild->linkedListNode = PlInsertLinkedListNode( newNode->linkedList, newChild );
		newChild->parent         = newNode;

		child = acm_get_next_child( child );
	}

	return newNode;
}

void acm_branch_destroy( AcmBranch *node )
{
	if ( node == NULL )
	{
		return;
	}

	PL_DELETE( node->name.buf );
	PL_DELETE( node->data.buf );

	/* if it's an object/array, we'll need to clean up all it's children */
	if ( node->type == ACM_PROPERTY_TYPE_OBJECT || node->type == ACM_PROPERTY_TYPE_ARRAY )
	{
		AcmBranch *child = acm_get_first_child( node );
		while ( child != NULL )
		{
			AcmBranch *nextChild = acm_get_next_child( child );
			acm_branch_destroy( child );
			child = nextChild;
		}
	}

	PlDestroyLinkedList( node->linkedList );
	if ( node->parent != NULL )
	{
		PlDestroyLinkedListNode( node->linkedListNode );
	}

	PL_DELETE( node );
}

/******************************************/
/** Deserialisation **/

static char *deserialize_string_var( PLFile *file, uint16_t *length )
{
	*length = PlReadInt16( file, false, NULL );
	if ( *length > 0 )
	{
		char *buf = PlMAlloc( *length, true );
		PlReadFile( file, buf, sizeof( char ), *length );
		return buf;
	}

	return NULL;
}

static AcmBranch *deserialize_binary_node( PLFile *file, AcmBranch *parent )
{
	/* try to fetch the name, not all nodes necessarily have a name... */
	NdVarString name;
	name.buf          = deserialize_string_var( file, &name.length );
	const char *dname = ( name.buf != NULL ) ? name.buf : "unknown";

	bool            status;
	AcmPropertyType type = ( AcmPropertyType ) PlReadInt8( file, &status );
	if ( !status )
	{
		Warning( "Failed to read property type for \"%s\"!\n", dname );
		PlFree( name.buf );
		return NULL;
	}

	/* binary implementation is pretty damn straight forward */
	AcmBranch *node = acm_push_new_branch( parent, NULL, type, ACM_PROPERTY_TYPE_INVALID );
	if ( node == NULL )
	{
		PlFree( name.buf );
		return NULL;
	}

	/* node now takes ownership of name */
	node->name = name;

	switch ( node->type )
	{
		default:
			Warning( "Encountered unhandled node type: %d!\n", node->type );
			acm_branch_destroy( node );
			node = NULL;
			break;
		case ACM_PROPERTY_TYPE_ARRAY:
			/* only extra component we get here is the child type */
			node->childType = ( AcmPropertyType ) PlReadInt8( file, NULL );
		case ACM_PROPERTY_TYPE_OBJECT:
		{
			unsigned int numChildren = PlReadInt32( file, false, NULL );
			for ( unsigned int i = 0; i < numChildren; ++i )
			{
				deserialize_binary_node( file, node );
			}
			break;
		}
		case ACM_PROPERTY_TYPE_STRING:
		{
			node->data.buf = deserialize_string_var( file, &node->data.length );
			break;
		}
		case ACM_PROPERTY_TYPE_BOOL:
		{
			bool v         = PlReadInt8( file, NULL );
			node->data.buf = alloc_var_string( v ? "true" : "false", &node->data.length );
			break;
		}
		case ACM_PROPERTY_TYPE_FLOAT32:
		{
			float v = PlReadFloat32( file, false, NULL );
			char  str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_float, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
		case ACM_PROPERTY_TYPE_FLOAT64:
		{
			double v = PlReadFloat64( file, false, NULL );
			char   str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_double, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_INT8:
		{
			int8_t v = PlReadInt8( file, NULL );
			char   str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_int32, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_UI16:
		case ND_PROPERTY_INT16:
		{
			int16_t v = PlReadInt16( file, false, NULL );
			char    str[ 32 ];
			snprintf( str, sizeof( str ), node->type == ND_PROPERTY_INT16 ? PL_FMT_int16 : PL_FMT_uint16, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
		}
		case ND_PROPERTY_UI32:
		case ND_PROPERTY_INT32:
		{
			int32_t v = PlReadInt32( file, false, NULL );
			char    str[ 32 ];
			snprintf( str, sizeof( str ), node->type == ND_PROPERTY_INT32 ? PL_FMT_int32 : PL_FMT_uint32, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_UI64:
		case ND_PROPERTY_INT64:
		{
			int64_t v = PlReadInt64( file, false, NULL );
			char    str[ 32 ];
			snprintf( str, sizeof( str ), node->type == ND_PROPERTY_INT64 ? PL_FMT_int64 : PL_FMT_uint64, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
	}

	return node;
}

static AcmFileType parse_node_file_type( PLFile *file, uint32_t *dstVersion )
{
	char token[ 32 ];
	if ( PlReadString( file, token, sizeof( token ) ) == NULL )
	{
		set_error_message( ND_ERROR_IO_READ, "Failed to read in file type: %s", PlGetError() );
		return ACM_FILE_TYPE_INVALID;
	}

	if ( strncmp( token, ND_FORMAT_BINARY_HEADER_2, strlen( ND_FORMAT_BINARY_HEADER_2 ) ) == 0 )
	{
		uint32_t version = PL_READUINT32( file, false, NULL );
		if ( version == 0 || version > ND_FORMAT_BINARY_VERSION )
		{
			set_error_message( ND_ERROR_IO_READ, "invalid binary node format (%u == 0 || %u > %u)", version, version, ND_FORMAT_BINARY_VERSION );
			return ACM_FILE_TYPE_INVALID;
		}

		*dstVersion = version;
		return ACM_FILE_TYPE_BINARY;
	}
	else if ( strncmp( token, ND_FORMAT_BINARY_HEADER, strlen( ND_FORMAT_BINARY_HEADER ) ) == 0 )
	{
		*dstVersion = 1;
		return ACM_FILE_TYPE_BINARY;
	}
	/* we still check for 'ascii' here, just for backwards compat, but they're handled the
	 * same either way */
	else if ( strncmp( token, ND_FORMAT_UTF8_HEADER, strlen( ND_FORMAT_UTF8_HEADER ) ) == 0 )
	{
		*dstVersion = 1;
		return ACM_FILE_TYPE_UTF8;
	}

	set_error_message( ND_ERROR_INVALID_ARGUMENT, "Unknown file type \"%s\"", token );
	return ACM_FILE_TYPE_INVALID;
}

AcmBranch *acm_parse_file( PLFile *file, const char *objectType )
{
	AcmBranch *root = NULL;

	uint32_t    version;
	AcmFileType fileType = parse_node_file_type( file, &version );
	if ( fileType == ACM_FILE_TYPE_BINARY )
	{
		root = deserialize_binary_node( file, NULL );
	}
	else if ( fileType == ACM_FILE_TYPE_UTF8 )
	{
		size_t length = PlGetFileSize( file );
		if ( length <= strlen( ND_FORMAT_UTF8_HEADER ) )
		{
			Warning( "Unexpected file size, possibly not a valid node file?\n" );
		}
		else
		{
			size_t      headerSize = strlen( ND_FORMAT_UTF8_HEADER );
			const char *data       = ( const char * ) ( ( uint8_t * ) PlGetFileData( file ) + headerSize );

			length -= headerSize;
			char *buf = PL_NEW_( char, length + 1 );
			memcpy( buf, data, length );

			root = acm_parse_buffer( buf, PlGetFilePath( file ) );

			PL_DELETE( buf );
		}
	}
	else
	{
		Warning( "Invalid node file type: %d\n", fileType );
	}

	if ( root != NULL && objectType != NULL )
	{
		const char *rootName = acm_branch_get_name( root );
		if ( strcmp( rootName, objectType ) != 0 )
		{
			/* destroy the tree */
			acm_branch_destroy( root );

			Warning( "Invalid \"%s\" file, expected \"%s\" but got \"%s\"!\n", objectType, objectType, rootName );
			return NULL;
		}
	}

	return root;
}

AcmBranch *acm_load_file( const char *path, const char *objectType )
{
	clear_error_message();

	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		Warning( "Failed to open \"%s\": %s\n", path, PlGetError() );
		return NULL;
	}

	AcmBranch *root = acm_parse_file( file, objectType );

	PlCloseFile( file );

	return root;
}

/******************************************/
/** Serialisation **/

static unsigned int sDepth; /* serialisation depth */

static void write_line( FILE *file, const char *string, bool tabify )
{
	if ( tabify )
	{
		for ( unsigned int i = 0; i < sDepth; ++i )
		{
			fputc( '\t', file );
		}
	}

	if ( string == NULL )
	{
		return;
	}

	fprintf( file, "%s", string );
}

static void serialize_string_var( const NdVarString *string, AcmFileType fileType, FILE *file )
{
	if ( fileType == ACM_FILE_TYPE_BINARY )
	{
		fwrite( &string->length, sizeof( uint16_t ), 1, file );
		/* slightly paranoid here, because strBuf is probably null if length is 0
		 * which is totally valid, but eh */
		if ( string->length > 0 )
		{
			fwrite( string->buf, sizeof( char ), string->length, file );
		}

		return;
	}

	/* allow nameless nodes, used for arrays */
	if ( string->length == 0 )
	{
		return;
	}

	bool        encloseString = false;
	const char *c             = string->buf;
	if ( *c == '\0' )
	{
		/* enclose an empty string!!! */
		encloseString = true;
	}
	else
	{
		/* otherwise, check if there are any spaces */
		while ( *c != '\0' )
		{
			if ( *c == ' ' )
			{
				encloseString = true;
				break;
			}

			c++;
		}
	}

	if ( encloseString )
	{
		fprintf( file, "\"%s\" ", string->buf );
	}
	else
	{
		fprintf( file, "%s ", string->buf );
	}
}

static void serialize_node_tree( FILE *file, AcmBranch *root, AcmFileType fileType );
static void serialize_node( FILE *file, AcmBranch *node, AcmFileType fileType )
{
	if ( fileType == ACM_FILE_TYPE_UTF8 )
	{
		/* write out the line identifying this node */
		write_line( file, NULL, true );
		AcmBranch *parent = acm_get_parent( node );
		if ( parent == NULL || parent->type != ACM_PROPERTY_TYPE_ARRAY )
		{
			fprintf( file, "%s ", string_for_property_type( node->type ) );
			if ( node->type == ACM_PROPERTY_TYPE_ARRAY )
			{
				fprintf( file, "%s ", string_for_property_type( node->childType ) );
			}

			serialize_string_var( &node->name, fileType, file );
		}

		/* if this node has children, serialize all those */
		if ( node->type == ACM_PROPERTY_TYPE_OBJECT || node->type == ACM_PROPERTY_TYPE_ARRAY )
		{
			write_line( file, "{\n", ( parent != NULL && parent->type == ACM_PROPERTY_TYPE_ARRAY ) );
			sDepth++;
			serialize_node_tree( file, node, fileType );
			sDepth--;
			write_line( file, "}\n", true );
		}
		else
		{
			serialize_string_var( &node->data, fileType, file );
			fprintf( file, "\n" );
		}

		return;
	}

	serialize_string_var( &node->name, fileType, file );
	fwrite( &node->type, sizeof( int8_t ), 1, file );
	switch ( node->type )
	{
		default:
		{
			Warning( "Invalid node type: " PL_FMT_uint32 "/n", node->type );
			abort();
		}
		case ACM_PROPERTY_TYPE_FLOAT32:
		{
			float v;
			acm_branch_get_float32( node, &v );
			fwrite( &v, sizeof( float ), 1, file );
			break;
		}
		case ACM_PROPERTY_TYPE_FLOAT64:
		{
			double v;
			acm_branch_get_float64( node, &v );
			fwrite( &v, sizeof( double ), 1, file );
			break;
		}
		case ND_PROPERTY_INT8:
		{
			int8_t v;
			acm_branch_get_int8( node, &v );
			fwrite( &v, sizeof( int8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_INT16:
		{
			int16_t v;
			acm_branch_get_int16( node, &v );
			fwrite( &v, sizeof( int16_t ), 1, file );
		}
		case ND_PROPERTY_INT32:
		{
			int32_t v;
			acm_branch_get_int32( node, &v );
			fwrite( &v, sizeof( int32_t ), 1, file );
			break;
		}
		case ND_PROPERTY_INT64:
		{
			int64_t v;
			acm_branch_get_int64( node, &v );
			fwrite( &v, sizeof( int64_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI8:
		{
			uint8_t v;
			acm_branch_get_uint8( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI16:
		{
			uint16_t v;
			acm_branch_get_uint16( node, &v );
			fwrite( &v, sizeof( uint16_t ), 1, file );
		}
		case ND_PROPERTY_UI32:
		{
			uint32_t v;
			acm_branch_get_uint32( node, &v );
			fwrite( &v, sizeof( uint32_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI64:
		{
			uint64_t v;
			acm_branch_get_uint64( node, &v );
			fwrite( &v, sizeof( uint64_t ), 1, file );
			break;
		}
		case ACM_PROPERTY_TYPE_STRING:
		{
			serialize_string_var( &node->data, fileType, file );
			break;
		}
		case ACM_PROPERTY_TYPE_BOOL:
		{
			bool v;
			acm_branch_get_bool( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case ACM_PROPERTY_TYPE_ARRAY:
		{
			/* only extra component here is the child type */
			fwrite( &node->childType, sizeof( uint8_t ), 1, file );
		}
		case ACM_PROPERTY_TYPE_OBJECT:
		{
			uint32_t i = PlGetNumLinkedListNodes( node->linkedList );
			fwrite( &i, sizeof( uint32_t ), 1, file );
			serialize_node_tree( file, node, fileType );
			break;
		}
	}
}

static void serialize_node_tree( FILE *file, AcmBranch *root, AcmFileType fileType )
{
	PLLinkedListNode *i = PlGetFirstNode( root->linkedList );
	while ( i != NULL )
	{
		AcmBranch *node = PlGetLinkedListNodeUserData( i );
		serialize_node( file, node, fileType );
		i = PlGetNextLinkedListNode( i );
	}
}

/**
 * Serialize the given node set.
 */
bool acm_write_file( const char *path, AcmBranch *root, AcmFileType fileType )
{
	FILE *file = fopen( path, "wb" );
	if ( file == NULL )
	{
		set_error_message( ND_ERROR_IO_WRITE, "Failed to open path \"%s\"", path );
		return false;
	}

	if ( fileType == ACM_FILE_TYPE_BINARY )
	{
		fprintf( file, ND_FORMAT_BINARY_HEADER_2 "\n" );
		static const unsigned int version = ND_FORMAT_BINARY_VERSION;
		fwrite( &version, sizeof( uint32_t ), 1, file );
	}
	else
	{
		sDepth = 0;
		fprintf( file, ND_FORMAT_UTF8_HEADER "\n; this node file has been auto-generated!\n" );
	}

	serialize_node( file, root, fileType );

	fclose( file );

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Struct Serialization
/////////////////////////////////////////////////////////////////////////////////////

#define GET_DATA_PTR( PTR, OFFS ) ( ( void * ) ( PTR ) + ( OFFS ) )

AcmBranch *acm_serialize_struct( const AcmStructDescriptor *descriptor, const void *ptr, AcmErrorCode *errorCode )
{
	//TODO: unfinished
	return NULL;

	*errorCode = ND_ERROR_SUCCESS;

	AcmBranch *branch = acm_push_new_branch( NULL, descriptor->name, ACM_PROPERTY_TYPE_OBJECT, ACM_PROPERTY_TYPE_INVALID );
	if ( branch == NULL )
	{
		*errorCode = acm_get_error();
		return NULL;
	}

	for ( unsigned int i = 0; i < descriptor->numItems; ++i )
	{
		const AcmStructItemDescriptor *item = &descriptor->items[ i ];

		void *data = GET_DATA_PTR( ptr, item->offset );
		switch ( descriptor->items[ i ].type )
		{
			default:
			{
				*errorCode = ND_ERROR_INVALID_TYPE;
				return branch;
			}
			case ACM_PROPERTY_TYPE_OBJECT: break;
			case ACM_PROPERTY_TYPE_ARRAY: break;
			case ACM_PROPERTY_TYPE_STRING: break;
			case ACM_PROPERTY_TYPE_BOOL:
			{
				acm_push_bool( branch, item->name, *( bool * ) data );
				break;
			}
			case ACM_PROPERTY_TYPE_FLOAT32:
			{
				acm_push_f32( branch, item->name, *( float * ) data );
				break;
			}
			case ACM_PROPERTY_TYPE_FLOAT64:
			{
				acm_push_f64( branch, item->name, *( double * ) data );
				break;
			}
			case ND_PROPERTY_INT8:
			{
				acm_push_i8( branch, item->name, *( int8_t * ) data );
				break;
			}
			case ND_PROPERTY_INT16:
			{
				acm_push_i16( branch, item->name, *( int16_t * ) data );
				break;
			}
			case ND_PROPERTY_INT32:
			{
				acm_push_i32( branch, item->name, *( int32_t * ) data );
				break;
			}
			case ND_PROPERTY_INT64: break;
			case ND_PROPERTY_UI8: break;
			case ND_PROPERTY_UI16: break;
			case ND_PROPERTY_UI32: break;
			case ND_PROPERTY_UI64: break;
		}
	}

	return branch;
}

/////////////////////////////////////////////////////////////////////////////////////

/******************************************/
/** API Testing **/

void acm_print_tree( AcmBranch *self, int index )
{
	for ( int i = 0; i < index; ++i ) printf( "\t" );
	if ( self->type == ACM_PROPERTY_TYPE_OBJECT || self->type == ACM_PROPERTY_TYPE_ARRAY )
	{
		index++;

		const char *name = ( self->name.buf != NULL ) ? self->name.buf : "";
		if ( self->type == ACM_PROPERTY_TYPE_OBJECT )
			Message( "%s (%s)\n", name, string_for_property_type( self->type ) );
		else
			Message( "%s (%s %s)\n", name, string_for_property_type( self->type ), string_for_property_type( self->childType ) );

		AcmBranch *child = acm_get_first_child( self );
		while ( child != NULL )
		{
			acm_print_tree( child, index );
			child = acm_get_next_child( child );
		}
	}
	else
	{
		AcmBranch *parent = acm_get_parent( self );
		if ( parent != NULL && parent->type == ACM_PROPERTY_TYPE_ARRAY )
			Message( "%s %s\n", string_for_property_type( self->type ), self->data.buf );
		else
			Message( "%s %s %s\n", string_for_property_type( self->type ), self->name.buf, self->data.buf );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Testing
/////////////////////////////////////////////////////////////////////////////////////

#if defined( ACM_TEST )

int main( PL_UNUSED int argc, PL_UNUSED char **argv )
{
	PlInitialize( argc, argv );

	acm_setup_logs();

	AcmBranch *branch = acm_load_file( "projects/base/base.prj.n", NULL );
	if ( branch == NULL )
	{
		return EXIT_FAILURE;
	}

	acm_print_tree( branch, 0 );

	return EXIT_SUCCESS;
}

#endif
