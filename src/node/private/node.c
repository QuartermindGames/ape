// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_filesystem.h>

#include "node_private.h"

int nd_LogLevelPrint_ = -1;
int nd_LogLevelWarn_ = -1;
void nd_setup_logs( void )
{
	nd_LogLevelPrint_ = PlAddLogLevel( "node", PL_COLOUR_DARK_SLATE_BLUE, true );
	nd_LogLevelWarn_ = PlAddLogLevel( "node/warning", PL_COLOUR_YELLOW, true );
	Message( "Logs are now active for NODE library\n" );
}

#define ND_FORMAT_UTF8_HEADER "node.utf8"

#define ND_FORMAT_BINARY_HEADER   "node.bin" // original format w/ no versioning support (defaults to 1)
#define ND_FORMAT_BINARY_HEADER_2 "node.binx"// new format w/ versioning support
#define ND_FORMAT_BINARY_VERSION  1

static const char *string_for_property_type( NdPropertyType propertyType )
{
	const char *propToStr[ ND_MAX_PROPERTY_TYPES ] = {
	        // Special types
	        [ND_PROPERTY_OBJECT] = "object",
	        [ND_PROPERTY_STRING] = "string",
	        [ND_PROPERTY_BOOL] = "bool",
	        [ND_PROPERTY_ARRAY] = "array",
	        // Generic types
	        [ND_PROPERTY_INT8] = "int8",
	        [ND_PROPERTY_INT16] = "int16",
	        [ND_PROPERTY_INT32] = "int32",
	        [ND_PROPERTY_INT64] = "int64",
	        [ND_PROPERTY_UI8] = "uint8",
	        [ND_PROPERTY_UI16] = "uint16",
	        [ND_PROPERTY_UI32] = "uint32",
	        [ND_PROPERTY_UI64] = "uint64",
	        [ND_PROPERTY_FLOAT32] = "float",
	        [ND_PROPERTY_FLOAT64] = "float64",
	};

	if ( propertyType == ND_PROPERTY_INVALID )
	{
		return "undefined";
	}

	return propToStr[ propertyType ];
}

static char *nlErrorMsg = NULL;
static NdErrorCode nlErrorType = ND_ERROR_SUCCESS;
static void clear_error_message( void )
{
	PlFree( nlErrorMsg );
	nlErrorMsg = NULL;
	nlErrorType = ND_ERROR_SUCCESS;
}

static void set_error_message( NdErrorCode type, const char *msg, ... )
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

const char *nd_get_error_message( void ) { return nlErrorMsg; }
NdErrorCode nd_get_error( void ) { return nlErrorType; }

static char *alloc_var_string( const char *string, uint16_t *lengthOut )
{
	*lengthOut = ( uint16_t ) strlen( string ) + 1;
	char *buf = PlCAllocA( 1, *lengthOut );
	strcpy( buf, string );
	return buf;
}

unsigned int nd_branch_get_num_of_children( const NdBranch *self )
{
	return PlGetNumLinkedListNodes( self->linkedList );
}

NdBranch *nd_branch_get_first_child( NdBranch *self )
{
	PLLinkedListNode *n = PlGetFirstNode( self->linkedList );
	if ( n == NULL )
	{
		return NULL;
	}

	return PlGetLinkedListNodeUserData( n );
}

NdBranch *nd_get_next_child( NdBranch *node )
{
	PLLinkedListNode *n = PlGetNextLinkedListNode( node->linkedListNode );
	if ( n == NULL )
		return NULL;

	return PlGetLinkedListNodeUserData( n );
}

NdBranch *nd_branch_get_child_by_name( NdBranch *self, const char *name )
{
	if ( self->type != ND_PROPERTY_OBJECT )
	{
		set_error_message( ND_ERROR_INVALID_TYPE, "Attempted to get child from an invalid node type!\n" );
		return NULL;
	}

	NdBranch *child = nd_branch_get_first_child( self );
	while ( child != NULL )
	{
		if ( strcmp( name, child->name.buf ) == 0 )
		{
			return child;
		}

		child = nd_get_next_child( child );
	}

	return NULL;
}

static const NdVarString *get_value_by_name( NdBranch *root, const char *name )
{
	const NdBranch *field = nd_branch_get_child_by_name( root, name );
	if ( field == NULL )
	{
		return NULL;
	}

	return &field->data;
}

NdBranch *nd_branch_get_parent( NdBranch *self )
{
	return self->parent;
}

const char *nd_branch_get_name( const NdBranch *self )
{
	return self->name.buf;
}

NdPropertyType nd_branch_get_type( const NdBranch *self )
{
	return self->type;
}

NdErrorCode nd_branch_get_string( const NdBranch *self, char *dest, size_t length )
{
	if ( self->type != ND_PROPERTY_STRING ) return ND_ERROR_INVALID_TYPE;
	snprintf( dest, length, "%s", self->data.buf );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_bool( const NdBranch *self, bool *dest )
{
	if ( self->type != ND_PROPERTY_BOOL ) return ND_ERROR_INVALID_TYPE;

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

NdErrorCode nd_branch_get_float32( const NdBranch *self, float *dest )
{
	if ( self->type != ND_PROPERTY_FLOAT32 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtof( self->data.buf, NULL );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_float64( const NdBranch *self, double *dest )
{
	if ( self->type != ND_PROPERTY_FLOAT64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtod( self->data.buf, NULL );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_int8( const NdBranch *self, int8_t *dest )
{
	if ( self->type != ND_PROPERTY_INT8 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int8_t ) strtol( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_int16( const NdBranch *self, int16_t *dest )
{
	if ( self->type != ND_PROPERTY_INT16 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int16_t ) strtol( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_int32( const NdBranch *self, int32_t *dest )
{
	if ( self->type != ND_PROPERTY_INT32 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int32_t ) strtol( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_int64( const NdBranch *self, int64_t *dest )
{
	if ( self->type != ND_PROPERTY_INT64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoll( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_uint8( const NdBranch *self, uint8_t *dest )
{
	if ( self->type != ND_PROPERTY_UI8 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( uint8_t ) strtoul( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_uint16( const NdBranch *self, uint16_t *dest )
{
	if ( self->type != ND_PROPERTY_UI16 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( uint16_t ) strtoul( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_uint32( const NdBranch *self, uint32_t *dest )
{
	if ( self->type != ND_PROPERTY_UI32 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoul( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_uint64( const NdBranch *self, uint64_t *dest )
{
	if ( self->type != ND_PROPERTY_UI64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoull( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_string_array( NdBranch *self, char **buf, unsigned int numElements )
{
	if ( self->type != ND_PROPERTY_ARRAY || self->childType != ND_PROPERTY_STRING )
	{
		return ND_ERROR_INVALID_TYPE;
	}

	NdBranch *child = nd_branch_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
		{
			return ND_ERROR_INVALID_ELEMENTS;
		}

		buf[ i ] = PL_NEW_( char, child->data.length + 1 );
		strncpy( buf[ i ], child->data.buf, child->data.length );

		child = nd_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_bool_array( NdBranch *self, bool *buf, unsigned int numElements )
{
	if ( self->type != ND_PROPERTY_ARRAY || self->childType != ND_PROPERTY_FLOAT64 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = nd_branch_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = nd_branch_get_bool( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = nd_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_int8_array( NdBranch *self, int8_t *buf, unsigned int numElements )
{
	if ( self->type != ND_PROPERTY_ARRAY || self->childType != ND_PROPERTY_INT8 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = nd_branch_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = nd_branch_get_int8( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = nd_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_int16_array( NdBranch *self, int16_t *buf, unsigned int numElements )
{
	if ( self->type != ND_PROPERTY_ARRAY || self->childType != ND_PROPERTY_INT16 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = nd_branch_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = nd_branch_get_int16( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = nd_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_int32_array( NdBranch *self, int32_t *buf, unsigned int numElements )
{
	if ( self->type != ND_PROPERTY_ARRAY || self->childType != ND_PROPERTY_INT32 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = nd_branch_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = nd_branch_get_int32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = nd_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_uint32_array( NdBranch *self, uint32_t *buf, unsigned int numElements )
{
	if ( self->type != ND_PROPERTY_ARRAY || self->childType != ND_PROPERTY_UI32 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = nd_branch_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = nd_branch_get_uint32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = nd_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_float32_array( NdBranch *self, float *buf, unsigned int numElements )
{
	if ( self->type != ND_PROPERTY_ARRAY || self->childType != ND_PROPERTY_FLOAT32 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = nd_branch_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = nd_branch_get_float32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = nd_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode nd_branch_get_float64_array( NdBranch *self, double *buf, unsigned int numElements )
{
	if ( self->type != ND_PROPERTY_ARRAY || self->childType != ND_PROPERTY_FLOAT64 )
	{
		return ND_ERROR_INVALID_TYPE;
	}

	NdBranch *child = nd_branch_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
		{
			return ND_ERROR_INVALID_ELEMENTS;
		}

		NdErrorCode errorCode = nd_branch_get_float64( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = nd_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

/******************************************/
/** Get: ByName **/

bool nd_branch_get_child_bool( NdBranch *root, const char *name, bool fallback )
{
	const NdBranch *child = nd_branch_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return fallback;
	}

	bool out;
	if ( nd_branch_get_bool( child, &out ) != ND_ERROR_SUCCESS )
	{
		return fallback;
	}

	return out;
}

const char *nd_branch_get_child_string( NdBranch *node, const char *name, const char *fallback )
{
	/* todo: warning on fail */
	const NdVarString *var = get_value_by_name( node, name );
	return ( var != NULL ) ? var->buf : fallback;
}

float nd_branch_get_child_float32( NdBranch *node, const char *name, float fallback )
{
	return ( float ) nd_branch_get_child_float64( node, name, fallback );
}

double nd_branch_get_child_float64( NdBranch *node, const char *name, double fallback )
{
	/* todo: warning on fail */
	const NdVarString *var = get_value_by_name( node, name );
	return ( var != NULL ) ? strtod( var->buf, NULL ) : fallback;
}

intmax_t nd_branch_get_child_int( NdBranch *root, const char *name, intmax_t fallback )
{
	const NdVarString *var = get_value_by_name( root, name );
	return ( var != NULL ) ? strtoll( var->buf, NULL, 10 ) : fallback;
}

uintmax_t nd_branch_get_child_uint( NdBranch *root, const char *name, uintmax_t fallback )
{
	const NdVarString *var = get_value_by_name( root, name );
	return ( var != NULL ) ? strtoull( var->buf, NULL, 10 ) : fallback;
}

PLVector2 nd_get_vector2( NdBranch *root, const char *name, const PLVector2 *fallback )
{
	NdBranch *child = nd_branch_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return *fallback;
	}

	PLVector2 v;
	if ( nd_branch_get_float32_array( child, ( float * ) &v, 2 ) != ND_ERROR_SUCCESS )
	{
		return *fallback;
	}

	return v;
}

PLVector3 nd_get_vector3( NdBranch *root, const char *name, const PLVector3 *fallback )
{
	NdBranch *child = nd_branch_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return *fallback;
	}

	PLVector3 v;
	if ( nd_branch_get_float32_array( child, ( float * ) &v, 3 ) != ND_ERROR_SUCCESS )
	{
		return *fallback;
	}

	return v;
}

PLVector4 nd_get_vector4( NdBranch *root, const char *name, const PLVector4 *fallback )
{
	NdBranch *child = nd_branch_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return *fallback;
	}

	PLVector4 v;
	if ( nd_branch_get_float32_array( child, ( float * ) &v, 4 ) != ND_ERROR_SUCCESS )
	{
		return *fallback;
	}

	return v;
}

PLColourF32 nd_get_colour_f32( NdBranch *root, const char *name, const PLColourF32 *fallback )
{
	PLVector4 v = nd_get_vector4( root, name, ( PLVector4 * ) fallback );
	return PlVector4ToColourF32( &v );
}

/******************************************/

NdBranch *nd_push_back_new_branch( NdBranch *parent, const char *name, NdPropertyType propertyType )
{
	/* arrays are special cases */
	if ( parent != NULL && parent->type == ND_PROPERTY_ARRAY && propertyType != parent->childType )
	{
		set_error_message( ND_ERROR_INVALID_TYPE, "attempted to add invalid type (%s)", string_for_property_type( propertyType ) );
		return NULL;
	}

	NdBranch *node = PL_NEW( NdBranch );

	/* assign the node name, if provided */
	if ( ( parent == NULL || parent->type != ND_PROPERTY_ARRAY ) && name != NULL )
	{
		node->name.buf = alloc_var_string( name, &node->name.length );
	}

	node->type = propertyType;
	node->linkedList = PlCreateLinkedList();

	/* if root is provided, this is treated as a child of that node */
	if ( parent != NULL )
	{
		if ( parent->linkedList == NULL )
		{
			parent->linkedList = PlCreateLinkedList();
		}

		node->linkedListNode = PlInsertLinkedListNode( parent->linkedList, node );
		node->parent = parent;
	}

	return node;
}

NdBranch *nd_branch_push_back_branch( NdBranch *parent, NdBranch *child )
{
	NdBranch *childCopy = nd_copy_branch( child );
	childCopy->parent = parent;
	childCopy->linkedListNode = PlInsertLinkedListNode( parent->linkedList, childCopy );
	return childCopy;
}

NdBranch *nd_branch_push_back_object( NdBranch *node, const char *name )
{
	return nd_push_back_new_branch( node, name, ND_PROPERTY_OBJECT );
}

NdBranch *nd_branch_push_back_string( NdBranch *parent, const char *name, const char *var )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_STRING );
	if ( node != NULL )
		node->data.buf = alloc_var_string( var, &node->data.length );

	return node;
}

NdBranch *nd_branch_push_back_string_array( NdBranch *parent, const char *name, const char **array, unsigned int numElements )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_ARRAY );
	if ( node != NULL )
	{
		node->childType = ND_PROPERTY_STRING;
		for ( unsigned int i = 0; i < numElements; ++i )
			nd_branch_push_back_string( node, NULL, array[ i ] );
	}
	return node;
}

NdBranch *nd_branch_push_back_bool( NdBranch *parent, const char *name, bool var )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_BOOL );
	if ( node != NULL )
		node->data.buf = alloc_var_string( var ? "true" : "false", &node->data.length );

	return node;
}

NdBranch *nd_branch_push_back_int8( NdBranch *parent, const char *name, int8_t var )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_INT8 );
	if ( node != NULL )
	{
		char buf[ 4 ];
		pl_itoa( var, buf, sizeof( buf ), 10 );
		node->data.buf = alloc_var_string( buf, &node->data.length );
	}
	return node;
}

NdBranch *nd_branch_push_back_int16( NdBranch *parent, const char *name, int16_t var )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_INT16 );
	if ( node != NULL )
	{
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_int16, var );
		node->data.buf = alloc_var_string( buf, &node->data.length );
	}
	return node;
}

NdBranch *nd_branch_push_back_int32( NdBranch *parent, const char *name, int32_t var )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_INT32 );
	if ( node != NULL )
	{
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_int32, var );
		node->data.buf = alloc_var_string( buf, &node->data.length );
	}
	return node;
}

NdBranch *nd_branch_push_back_uint32( NdBranch *parent, const char *name, uint32_t var )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_UI32 );
	if ( node != NULL )
	{
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_uint32, var );
		node->data.buf = alloc_var_string( buf, &node->data.length );
	}
	return node;
}

NdBranch *nd_branch_push_back_float32( NdBranch *parent, const char *name, float var )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_FLOAT32 );
	if ( node != NULL )
	{
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_float, var );
		node->data.buf = alloc_var_string( buf, &node->data.length );
	}
	return node;
}

NdBranch *nd_branch_push_back_float64( NdBranch *parent, const char *name, double var )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_FLOAT64 );
	if ( node != NULL )
	{
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_double, var );
		node->data.buf = alloc_var_string( buf, &node->data.length );
	}
	return node;
}

NdBranch *nd_branch_push_back_int16_array( NdBranch *root, const char *name, const int16_t *array, unsigned int numElements )
{
	NdBranch *node = nd_push_back_new_branch( root, name, ND_PROPERTY_ARRAY );
	if ( node != NULL )
	{
		node->childType = ND_PROPERTY_INT16;
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			nd_branch_push_back_int16( node, NULL, array[ i ] );
		}
	}
	return node;
}

NdBranch *nd_branch_push_back_int32_array( NdBranch *parent, const char *name, const int32_t *array, unsigned int numElements )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_ARRAY );
	if ( node != NULL )
	{
		node->childType = ND_PROPERTY_INT32;
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			nd_branch_push_back_int32( node, NULL, array[ i ] );
		}
	}
	return node;
}

NdBranch *nd_branch_push_back_uint32_array( NdBranch *parent, const char *name, const uint32_t *array, unsigned int numElements )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_ARRAY );
	if ( node != NULL )
	{
		node->childType = ND_PROPERTY_UI32;
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			nd_branch_push_back_uint32( node, NULL, array[ i ] );
		}
	}
	return node;
}

NdBranch *nd_branch_push_back_float32_array( NdBranch *parent, const char *name, const float *array, unsigned int numElements )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_ARRAY );
	if ( node != NULL )
	{
		node->childType = ND_PROPERTY_FLOAT32;
		for ( unsigned int i = 0; i < numElements; ++i )
			nd_branch_push_back_float32( node, NULL, array[ i ] );
	}
	return node;
}

NdBranch *nd_branch_push_back_object_array( NdBranch *parent, const char *name )
{
	NdBranch *node = nd_push_back_new_branch( parent, name, ND_PROPERTY_ARRAY );
	if ( node != NULL )
		node->childType = ND_PROPERTY_OBJECT;

	return node;
}

static char *copy_var_string( const NdVarString *varString, uint16_t *length )
{
	*length = varString->length;
	char *buf = PL_NEW_( char, *length + 1 );
	strncpy( buf, varString->buf, *length );
	return buf;
}

/**
 * Copies the given node list.
 */
NdBranch *nd_copy_branch( NdBranch *node )
{
	NdBranch *newNode = PL_NEW( NdBranch );
	newNode->type = node->type;
	newNode->childType = node->childType;
	newNode->data.buf = copy_var_string( &node->data, &newNode->data.length );
	newNode->name.buf = copy_var_string( &node->name, &newNode->name.length );
	// Not setting the parent is intentional here, since we likely don't want that link

	NdBranch *child = nd_branch_get_first_child( node );
	while ( child != NULL )
	{
		if ( newNode->linkedList == NULL )
			newNode->linkedList = PlCreateLinkedList();

		NdBranch *newChild = nd_copy_branch( child );
		newChild->linkedListNode = PlInsertLinkedListNode( newNode->linkedList, newChild );
		newChild->parent = newNode;

		child = nd_get_next_child( child );
	}

	return newNode;
}

void nd_branch_destroy( NdBranch *node )
{
	if ( node == NULL )
	{
		return;
	}

	PL_DELETE( node->name.buf );
	PL_DELETE( node->data.buf );

	/* if it's an object/array, we'll need to clean up all it's children */
	if ( node->type == ND_PROPERTY_OBJECT || node->type == ND_PROPERTY_ARRAY )
	{
		NdBranch *child = nd_branch_get_first_child( node );
		while ( child != NULL )
		{
			NdBranch *nextChild = nd_get_next_child( child );
			nd_branch_destroy( child );
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

static NdBranch *deserialize_binary_node( PLFile *file, NdBranch *parent )
{
	/* try to fetch the name, not all nodes necessarily have a name... */
	NdVarString name;
	name.buf = deserialize_string_var( file, &name.length );
	const char *dname = ( name.buf != NULL ) ? name.buf : "unknown";

	bool status;
	NdPropertyType type = ( NdPropertyType ) PlReadInt8( file, &status );
	if ( !status )
	{
		Warning( "Failed to read property type for \"%s\"!\n", dname );
		PlFree( name.buf );
		return NULL;
	}

	/* binary implementation is pretty damn straight forward */
	NdBranch *node = nd_push_back_new_branch( parent, NULL, type );
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
			nd_branch_destroy( node );
			node = NULL;
			break;
		case ND_PROPERTY_ARRAY:
			/* only extra component we get here is the child type */
			node->childType = ( NdPropertyType ) PlReadInt8( file, NULL );
		case ND_PROPERTY_OBJECT:
		{
			unsigned int numChildren = PlReadInt32( file, false, NULL );
			for ( unsigned int i = 0; i < numChildren; ++i )
				deserialize_binary_node( file, node );
			break;
		}
		case ND_PROPERTY_STRING:
		{
			node->data.buf = deserialize_string_var( file, &node->data.length );
			break;
		}
		case ND_PROPERTY_BOOL:
		{
			bool v = PlReadInt8( file, NULL );
			node->data.buf = alloc_var_string( v ? "true" : "false", &node->data.length );
			break;
		}
		case ND_PROPERTY_FLOAT32:
		{
			float v = PlReadFloat32( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_float, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_FLOAT64:
		{
			double v = PlReadFloat64( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_double, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_INT8:
		{
			int8_t v = PlReadInt8( file, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_int32, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_UI32:
		case ND_PROPERTY_INT32:
		{
			int32_t v = PlReadInt32( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), node->type == ND_PROPERTY_INT32 ? PL_FMT_int32 : PL_FMT_uint32, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_UI64:
		case ND_PROPERTY_INT64:
		{
			int64_t v = PlReadInt64( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), node->type == ND_PROPERTY_INT64 ? PL_FMT_int64 : PL_FMT_uint64, v );
			node->data.buf = alloc_var_string( str, &node->data.length );
			break;
		}
	}

	return node;
}

static NdFileType parse_node_file_type( PLFile *file, uint32_t *dstVersion )
{
	char token[ 32 ];
	if ( PlReadString( file, token, sizeof( token ) ) == NULL )
	{
		set_error_message( ND_ERROR_IO_READ, "Failed to read in file type: %s", PlGetError() );
		return ND_FILE_INVALID;
	}

	if ( strncmp( token, ND_FORMAT_BINARY_HEADER_2, strlen( ND_FORMAT_BINARY_HEADER_2 ) ) == 0 )
	{
		uint32_t version = PL_READUINT32( file, false, NULL );
		if ( version == 0 || version > ND_FORMAT_BINARY_VERSION )
		{
			set_error_message( ND_ERROR_IO_READ, "invalid binary node format (%u == 0 || %u > %u)", version, version, ND_FORMAT_BINARY_VERSION );
			return ND_FILE_INVALID;
		}

		*dstVersion = version;
		return ND_FILE_BINARY;
	}
	else if ( strncmp( token, ND_FORMAT_BINARY_HEADER, strlen( ND_FORMAT_BINARY_HEADER ) ) == 0 )
	{
		*dstVersion = 1;
		return ND_FILE_BINARY;
	}
	/* we still check for 'ascii' here, just for backwards compat, but they're handled the
	 * same either way */
	else if ( strncmp( token, ND_FORMAT_UTF8_HEADER, strlen( ND_FORMAT_UTF8_HEADER ) ) == 0 )
	{
		*dstVersion = 1;
		return ND_FILE_UTF8;
	}

	set_error_message( ND_ERROR_INVALID_ARGUMENT, "Unknown file type \"%s\"", token );
	return ND_FILE_INVALID;
}

NdBranch *nd_parse_file( PLFile *file, const char *objectType )
{
	NdBranch *root = NULL;

	uint32_t version;
	NdFileType fileType = parse_node_file_type( file, &version );
	if ( fileType == ND_FILE_BINARY )
	{
		root = deserialize_binary_node( file, NULL );
	}
	else if ( fileType == ND_FILE_UTF8 )
	{
		/* first need to run the pre-processor on it */
		size_t length = PlGetFileSize( file );
		if ( length <= strlen( ND_FORMAT_UTF8_HEADER ) )
		{
			Warning( "Unexpected file size, possibly not a valid node file?\n" );
		}
		else
		{
			size_t headerSize = strlen( ND_FORMAT_UTF8_HEADER );
			const char *data = ( const char * ) ( ( uint8_t * ) PlGetFileData( file ) + headerSize );

			length -= headerSize;
			char *buf = PL_NEW_( char, length + 1 );
			memcpy( buf, data, length );

			buf = ndPreProcessScript( buf, &length, true );
			root = nd_parse_buffer( buf, length );

			PL_DELETE( buf );
		}
	}
	else
	{
		Warning( "Invalid node file type: %d\n", fileType );
	}

	if ( root != NULL && objectType != NULL )
	{
		const char *rootName = nd_branch_get_name( root );
		if ( strcmp( rootName, objectType ) != 0 )
		{
			/* destroy the tree */
			nd_branch_destroy( root );

			Warning( "Invalid \"%s\" file, expected \"%s\" but got \"%s\"!\n", objectType, objectType, rootName );
			return NULL;
		}
	}

	return root;
}

NdBranch *nd_load_file( const char *path, const char *objectType )
{
	clear_error_message();

	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		Warning( "Failed to open \"%s\": %s\n", path, PlGetError() );
		return NULL;
	}

	NdBranch *root = nd_parse_file( file, objectType );

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

static void serialize_string_var( const NdVarString *string, NdFileType fileType, FILE *file )
{
	if ( fileType == ND_FILE_BINARY )
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

	bool encloseString = false;
	const char *c = string->buf;
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

static void serialize_node_tree( FILE *file, NdBranch *root, NdFileType fileType );
static void serialize_node( FILE *file, NdBranch *node, NdFileType fileType )
{
	if ( fileType == ND_FILE_UTF8 )
	{
		/* write out the line identifying this node */
		write_line( file, NULL, true );
		NdBranch *parent = nd_branch_get_parent( node );
		if ( parent == NULL || parent->type != ND_PROPERTY_ARRAY )
		{
			fprintf( file, "%s ", string_for_property_type( node->type ) );
			if ( node->type == ND_PROPERTY_ARRAY )
			{
				fprintf( file, "%s ", string_for_property_type( node->childType ) );
			}

			serialize_string_var( &node->name, fileType, file );
		}

		/* if this node has children, serialize all those */
		if ( node->type == ND_PROPERTY_OBJECT || node->type == ND_PROPERTY_ARRAY )
		{
			write_line( file, "{\n", ( parent != NULL && parent->type == ND_PROPERTY_ARRAY ) );
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
		case ND_PROPERTY_FLOAT32:
		{
			float v;
			nd_branch_get_float32( node, &v );
			fwrite( &v, sizeof( float ), 1, file );
			break;
		}
		case ND_PROPERTY_FLOAT64:
		{
			double v;
			nd_branch_get_float64( node, &v );
			fwrite( &v, sizeof( double ), 1, file );
			break;
		}
		case ND_PROPERTY_INT8:
		{
			int8_t v;
			nd_branch_get_int8( node, &v );
			fwrite( &v, sizeof( int8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_INT16:
		{
			int16_t v;
			nd_branch_get_int16( node, &v );
			fwrite( &v, sizeof( int16_t ), 1, file );
		}
		case ND_PROPERTY_INT32:
		{
			int32_t v;
			nd_branch_get_int32( node, &v );
			fwrite( &v, sizeof( int32_t ), 1, file );
			break;
		}
		case ND_PROPERTY_INT64:
		{
			int64_t v;
			nd_branch_get_int64( node, &v );
			fwrite( &v, sizeof( int64_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI8:
		{
			uint8_t v;
			nd_branch_get_uint8( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI16:
		{
			uint16_t v;
			nd_branch_get_uint16( node, &v );
			fwrite( &v, sizeof( uint16_t ), 1, file );
		}
		case ND_PROPERTY_UI32:
		{
			uint32_t v;
			nd_branch_get_uint32( node, &v );
			fwrite( &v, sizeof( uint32_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI64:
		{
			uint64_t v;
			nd_branch_get_uint64( node, &v );
			fwrite( &v, sizeof( uint64_t ), 1, file );
			break;
		}
		case ND_PROPERTY_STRING:
		{
			serialize_string_var( &node->data, fileType, file );
			break;
		}
		case ND_PROPERTY_BOOL:
		{
			bool v;
			nd_branch_get_bool( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_ARRAY:
		{
			/* only extra component here is the child type */
			fwrite( &node->childType, sizeof( uint8_t ), 1, file );
		}
		case ND_PROPERTY_OBJECT:
		{
			uint32_t i = PlGetNumLinkedListNodes( node->linkedList );
			fwrite( &i, sizeof( uint32_t ), 1, file );
			serialize_node_tree( file, node, fileType );
			break;
		}
	}
}

static void serialize_node_tree( FILE *file, NdBranch *root, NdFileType fileType )
{
	PLLinkedListNode *i = PlGetFirstNode( root->linkedList );
	while ( i != NULL )
	{
		NdBranch *node = PlGetLinkedListNodeUserData( i );
		serialize_node( file, node, fileType );
		i = PlGetNextLinkedListNode( i );
	}
}

/**
 * Serialize the given node set.
 */
bool nd_write_file( const char *path, NdBranch *root, NdFileType fileType )
{
	FILE *file = fopen( path, "wb" );
	if ( file == NULL )
	{
		set_error_message( ND_ERROR_IO_WRITE, "Failed to open path \"%s\"", path );
		return false;
	}

	if ( fileType == ND_FILE_BINARY )
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

NdBranch *nd_serialize_struct( const NdStructDescriptor *descriptor, const void *ptr, NdErrorCode *errorCode )
{
	//TODO: unfinished
	return NULL;

	*errorCode = ND_ERROR_SUCCESS;

	NdBranch *branch = nd_push_back_new_branch( NULL, descriptor->name, ND_PROPERTY_OBJECT );
	if ( branch == NULL )
	{
		*errorCode = nd_get_error();
		return NULL;
	}

	for ( unsigned int i = 0; i < descriptor->numItems; ++i )
	{
		const NdStructItemDescriptor *item = &descriptor->items[ i ];

		void *data = GET_DATA_PTR( ptr, item->offset );
		switch ( descriptor->items[ i ].type )
		{
			default:
			{
				*errorCode = ND_ERROR_INVALID_TYPE;
				return branch;
			}
			case ND_PROPERTY_OBJECT: break;
			case ND_PROPERTY_LINK: break;
			case ND_PROPERTY_ARRAY: break;
			case ND_PROPERTY_STRING: break;
			case ND_PROPERTY_BOOL:
			{
				nd_branch_push_back_bool( branch, item->name, *( bool * ) data );
				break;
			}
			case ND_PROPERTY_FLOAT32:
			{
				nd_branch_push_back_float32( branch, item->name, *( float * ) data );
				break;
			}
			case ND_PROPERTY_FLOAT64:
			{
				nd_branch_push_back_float64( branch, item->name, *( double * ) data );
				break;
			}
			case ND_PROPERTY_INT8:
			{
				nd_branch_push_back_int8( branch, item->name, *( int8_t * ) data );
				break;
			}
			case ND_PROPERTY_INT16:
			{
				nd_branch_push_back_int16( branch, item->name, *( int16_t * ) data );
				break;
			}
			case ND_PROPERTY_INT32:
			{
				nd_branch_push_back_int32( branch, item->name, *( int32_t * ) data );
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

void nd_branch_print_tree( NdBranch *self, int index )
{
	for ( int i = 0; i < index; ++i ) printf( "\t" );
	if ( self->type == ND_PROPERTY_OBJECT || self->type == ND_PROPERTY_ARRAY )
	{
		index++;

		const char *name = ( self->name.buf != NULL ) ? self->name.buf : "";
		if ( self->type == ND_PROPERTY_OBJECT )
			Message( "%s (%s)\n", name, string_for_property_type( self->type ) );
		else
			Message( "%s (%s %s)\n", name, string_for_property_type( self->type ), string_for_property_type( self->childType ) );

		NdBranch *child = nd_branch_get_first_child( self );
		while ( child != NULL )
		{
			nd_branch_print_tree( child, index );
			child = nd_get_next_child( child );
		}
	}
	else
	{
		NdBranch *parent = nd_branch_get_parent( self );
		if ( parent != NULL && parent->type == ND_PROPERTY_ARRAY )
			Message( "%s %s\n", string_for_property_type( self->type ), self->data.buf );
		else
			Message( "%s %s %s\n", string_for_property_type( self->type ), self->name.buf, self->data.buf );
	}
}
