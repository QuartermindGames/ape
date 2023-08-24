// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>

#include "node_private.h"

int nd_LogLevelPrint_ = -1;
int nd_LogLevelWarn_ = -1;
void ndSetupLogs( void ) {
	nd_LogLevelPrint_ = PlAddLogLevel( "node", PL_COLOUR_DARK_SLATE_BLUE, true );
	nd_LogLevelWarn_ = PlAddLogLevel( "node/warning", PL_COLOUR_YELLOW, true );
	Message( "Logs are now active for NODE library\n" );
}

#define ND_FORMAT_VERSION       1
#define ND_FORMAT_BINARY_HEADER "node.bin"
#define ND_FORMAT_ASCII_HEADER  "node.ascii" /* obsolete */
#define ND_FORMAT_UTF8_HEADER   "node.utf8"

static const char *StringForPropertyType( NdPropertyType propertyType ) {
	const char *propToStr[ ND_MAX_PROPERTY_TYPES ] = {
	        // Special types
	        [ND_PROPERTY_OBJECT] = "object",
	        [ND_PROPERTY_STRING] = "string",
	        [ND_PROPERTY_BOOL] = "bool",
	        [ND_PROPERTY_ARRAY] = "array",
	        // Generic types
	        [ND_PROPERTY_I8] = "int8",
	        [ND_PROPERTY_I16] = "int16",
	        [ND_PROPERTY_I32] = "int32",
	        [ND_PROPERTY_I64] = "int64",
	        [ND_PROPERTY_UI8] = "uint8",
	        [ND_PROPERTY_UI16] = "uint16",
	        [ND_PROPERTY_UI32] = "uint32",
	        [ND_PROPERTY_UI64] = "uint64",
	        [ND_PROPERTY_F32] = "float",
	        [ND_PROPERTY_F64] = "float64",
	};

	if ( propertyType == ND_PROPERTY_INVALID )
		return "undefined";

	return propToStr[ propertyType ];
}

static char *nlErrorMsg = NULL;
static NdErrorCode nlErrorType = ND_ERROR_SUCCESS;
static void ClearErrorMessage( void ) {
	PlFree( nlErrorMsg );
	nlErrorMsg = NULL;
	nlErrorType = ND_ERROR_SUCCESS;
}

static void SetErrorMessage( NdErrorCode type, const char *msg, ... ) {
	ClearErrorMessage();

	nlErrorType = type;

	va_list args;
	va_start( args, msg );

	int length = pl_vscprintf( msg, args ) + 1;
	if ( length <= 0 )
		return;

	nlErrorMsg = PlCAlloc( 1, length, false );
	if ( nlErrorMsg == NULL ) {
		Warning( "Failed to allocate error message buffer: %d bytes!\n", length );
		return;
	}

	vsnprintf( nlErrorMsg, length, msg, args );
	Warning( "NLERR: %s\n", nlErrorMsg );

	va_end( args );
}

const char *ndGetErrorMessage( void ) { return nlErrorMsg; }
NdErrorCode ndGetError( void ) { return nlErrorType; }

static char *AllocVarString( const char *string, uint16_t *lengthOut ) {
	*lengthOut = ( uint16_t ) strlen( string ) + 1;
	char *buf = PlCAllocA( 1, *lengthOut );
	strcpy( buf, string );
	return buf;
}

unsigned int ndGetNumOfChildren( const NdBranch *parent ) {
	return PlGetNumLinkedListNodes( parent->linkedList );
}

NdBranch *ndGetFirstChild( NdBranch *parent ) {
	PLLinkedListNode *n = PlGetFirstNode( parent->linkedList );
	if ( n == NULL )
		return NULL;

	return PlGetLinkedListNodeUserData( n );
}

NdBranch *ndGetNextChild( NdBranch *node ) {
	PLLinkedListNode *n = PlGetNextLinkedListNode( node->linkedListNode );
	if ( n == NULL )
		return NULL;

	return PlGetLinkedListNodeUserData( n );
}

NdBranch *ndGetChildByName( NdBranch *parent, const char *name ) {
	if ( parent->type != ND_PROPERTY_OBJECT ) {
		SetErrorMessage( ND_ERROR_INVALID_TYPE, "Attempted to get child from an invalid node type!\n" );
		return NULL;
	}

	NdBranch *child = ndGetFirstChild( parent );
	while ( child != NULL ) {
		if ( strcmp( name, child->name.buf ) == 0 )
			return child;

		child = ndGetNextChild( child );
	}

	return NULL;
}

static const NdVarString *GetValueByName( NdBranch *root, const char *name ) {
	const NdBranch *field = ndGetChildByName( root, name );
	if ( field == NULL )
		return NULL;

	return &field->data;
}

NdBranch *ndGetParent( NdBranch *node ) {
	return node->parent;
}

const char *ndGetName( const NdBranch *node ) {
	return node->name.buf;
}

NdPropertyType ndGetType( const NdBranch *node ) {
	return node->type;
}

NdErrorCode ndGetStr( const NdBranch *node, char *dest, size_t length ) {
	if ( node->type != ND_PROPERTY_STRING ) return ND_ERROR_INVALID_TYPE;
	snprintf( dest, length, "%s", node->data.buf );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetBool( const NdBranch *node, bool *dest ) {
	if ( node->type != ND_PROPERTY_BOOL ) return ND_ERROR_INVALID_TYPE;

	if ( ( strcmp( node->data.buf, "true" ) == 0 ) || ( node->data.buf[ 0 ] == '1' && node->data.buf[ 1 ] == '\0' ) ) {
		*dest = true;
		return ND_ERROR_SUCCESS;
	} else if ( ( strcmp( node->data.buf, "false" ) == 0 ) || ( node->data.buf[ 0 ] == '0' && node->data.buf[ 1 ] == '\0' ) ) {
		*dest = false;
		return ND_ERROR_SUCCESS;
	}

	SetErrorMessage( ND_ERROR_INVALID_ARGUMENT, "Invalid data passed from var" );
	return ND_ERROR_INVALID_ARGUMENT;
}

NdErrorCode ndGetF32( const NdBranch *node, float *dest ) {
	if ( node->type != ND_PROPERTY_F32 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtof( node->data.buf, NULL );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetF64( const NdBranch *node, double *dest ) {
	if ( node->type != ND_PROPERTY_F64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtod( node->data.buf, NULL );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetI8( const NdBranch *node, int8_t *dest ) {
	if ( node->type != ND_PROPERTY_I8 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int8_t ) strtol( node->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetI16( const NdBranch *node, int16_t *dest ) {
	if ( node->type != ND_PROPERTY_I16 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int16_t ) strtol( node->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetI32( const NdBranch *node, int32_t *dest ) {
	if ( node->type != ND_PROPERTY_I32 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int32_t ) strtol( node->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetI64( const NdBranch *node, int64_t *dest ) {
	if ( node->type != ND_PROPERTY_I64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoll( node->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetUI8( const NdBranch *node, uint8_t *dest ) {
	if ( node->type != ND_PROPERTY_UI8 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( uint8_t ) strtoul( node->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetUI16( const NdBranch *node, uint16_t *dest ) {
	if ( node->type != ND_PROPERTY_UI16 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( uint16_t ) strtoul( node->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetUI32( const NdBranch *node, uint32_t *dest ) {
	if ( node->type != ND_PROPERTY_UI32 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoul( node->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetUI64( const NdBranch *node, uint64_t *dest ) {
	if ( node->type != ND_PROPERTY_UI64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoull( node->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetStringArray( NdBranch *parent, char **buf, unsigned int numElements ) {
	if ( parent->type != ND_PROPERTY_ARRAY || parent->childType != ND_PROPERTY_STRING ) {
		return ND_ERROR_INVALID_TYPE;
	}

	NdBranch *child = ndGetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i ) {
		if ( child == NULL ) {
			return ND_ERROR_INVALID_ELEMENTS;
		}

		buf[ i ] = PL_NEW_( char, child->data.length + 1 );
		strncpy( buf[ i ], child->data.buf, child->data.length );

		child = ndGetNextChild( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetI8Array( NdBranch *parent, int8_t *buf, unsigned int numElements ) {
	if ( parent->type != ND_PROPERTY_ARRAY || parent->childType != ND_PROPERTY_I8 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = ndGetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i ) {
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = ndGetI8( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = ndGetNextChild( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetI16Array( NdBranch *parent, int16_t *buf, unsigned int numElements ) {
	if ( parent->type != ND_PROPERTY_ARRAY || parent->childType != ND_PROPERTY_I16 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = ndGetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i ) {
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = ndGetI16( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = ndGetNextChild( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetI32Array( NdBranch *parent, int32_t *buf, unsigned int numElements ) {
	if ( parent->type != ND_PROPERTY_ARRAY || parent->childType != ND_PROPERTY_I32 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = ndGetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i ) {
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = ndGetI32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = ndGetNextChild( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetUI32Array( NdBranch *parent, uint32_t *buf, unsigned int numElements ) {
	if ( parent->type != ND_PROPERTY_ARRAY || parent->childType != ND_PROPERTY_UI32 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = ndGetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i ) {
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = ndGetUI32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = ndGetNextChild( child );
	}

	return ND_ERROR_SUCCESS;
}

NdErrorCode ndGetF32Array( NdBranch *parent, float *buf, unsigned int numElements ) {
	if ( parent->type != ND_PROPERTY_ARRAY || parent->childType != ND_PROPERTY_F32 )
		return ND_ERROR_INVALID_TYPE;

	NdBranch *child = ndGetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i ) {
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		NdErrorCode errorCode = ndGetF32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = ndGetNextChild( child );
	}

	return ND_ERROR_SUCCESS;
}

/******************************************/
/** Get: ByName **/

bool ndGetBoolByName( NdBranch *root, const char *name, bool fallback ) {
	const NdBranch *child = ndGetChildByName( root, name );
	if ( child == NULL )
		return fallback;

	bool out;
	if ( ndGetBool( child, &out ) != ND_ERROR_SUCCESS )
		return fallback;

	return out;
}

const char *ndGetStringByName( NdBranch *node, const char *name, const char *fallback ) {
	/* todo: warning on fail */
	const NdVarString *var = GetValueByName( node, name );
	return ( var != NULL ) ? var->buf : fallback;
}

float ndGetF32ByName( NdBranch *node, const char *name, float fallback ) {
	return ( float ) ndGetF64ByName( node, name, fallback );
}

double ndGetF64ByName( NdBranch *node, const char *name, double fallback ) {
	/* todo: warning on fail */
	const NdVarString *var = GetValueByName( node, name );
	return ( var != NULL ) ? strtod( var->buf, NULL ) : fallback;
}

intmax_t ndGetInt( NdBranch *root, const char *name, intmax_t fallback ) {
	const NdVarString *var = GetValueByName( root, name );
	return ( var != NULL ) ? strtoll( var->buf, NULL, 10 ) : fallback;
}

uintmax_t ndGetUInt( NdBranch *root, const char *name, uintmax_t fallback ) {
	const NdVarString *var = GetValueByName( root, name );
	return ( var != NULL ) ? strtoull( var->buf, NULL, 10 ) : fallback;
}

PLVector2 ndGetVector2( NdBranch *root, const char *name, const PLVector2 *fallback ) {
	NdBranch *child = ndGetChildByName( root, name );
	if ( child == NULL ) {
		return *fallback;
	}

	PLVector2 v;
	if ( ndGetF32Array( child, ( float * ) &v, 2 ) != ND_ERROR_SUCCESS ) {
		return *fallback;
	}

	return v;
}

PLVector3 ndGetVector3( NdBranch *root, const char *name, const PLVector3 *fallback ) {
	NdBranch *child = ndGetChildByName( root, name );
	if ( child == NULL )
		return *fallback;

	PLVector3 v;
	if ( ndGetF32Array( child, ( float * ) &v, 3 ) != ND_ERROR_SUCCESS )
		return *fallback;

	return v;
}

PLVector4 ndGetVector4( NdBranch *root, const char *name, const PLVector4 *fallback ) {
	NdBranch *child = ndGetChildByName( root, name );
	if ( child == NULL )
		return *fallback;

	PLVector4 v;
	if ( ndGetF32Array( child, ( float * ) &v, 4 ) != ND_ERROR_SUCCESS )
		return *fallback;

	return v;
}

PLColourF32 ndGetColourF32( NdBranch *root, const char *name, const PLColourF32 *fallback ) {
	PLVector4 v = ndGetVector4( root, name, ( PLVector4 * ) fallback );
	return PlVector4ToColourF32( &v );
}

/******************************************/

NdBranch *ndPushBackNewBranch( NdBranch *parent, const char *name, NdPropertyType propertyType ) {
	/* arrays are special cases */
	if ( parent != NULL && parent->type == ND_PROPERTY_ARRAY && propertyType != parent->childType ) {
		SetErrorMessage( ND_ERROR_INVALID_TYPE, "attempted to add invalid type (%s)", StringForPropertyType( propertyType ) );
		return NULL;
	}

	NdBranch *node = PlCAllocA( 1, sizeof( NdBranch ) );

	/* assign the node name, if provided */
	if ( ( parent == NULL || parent->type != ND_PROPERTY_ARRAY ) && name != NULL )
		node->name.buf = AllocVarString( name, &node->name.length );

	node->type = propertyType;
	node->linkedList = PlCreateLinkedList();

	/* if root is provided, this is treated as a child of that node */
	if ( parent != NULL ) {
		if ( parent->linkedList == NULL )
			parent->linkedList = PlCreateLinkedList();

		node->linkedListNode = PlInsertLinkedListNode( parent->linkedList, node );
		node->parent = parent;
	}

	return node;
}

NdBranch *ndPushBackBranch( NdBranch *parent, NdBranch *child ) {
	NdBranch *childCopy = ndCopyBranch( child );
	childCopy->parent = parent;
	childCopy->linkedListNode = PlInsertLinkedListNode( parent->linkedList, childCopy );
	return childCopy;
}

NdBranch *ndPushBackObject( NdBranch *node, const char *name ) {
	return ndPushBackNewBranch( node, name, ND_PROPERTY_OBJECT );
}

NdBranch *ndPushBackString( NdBranch *parent, const char *name, const char *var ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_STRING );
	if ( node != NULL )
		node->data.buf = AllocVarString( var, &node->data.length );

	return node;
}

NdBranch *ndPushBackStringArray( NdBranch *parent, const char *name, const char **array, unsigned int numElements ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_ARRAY );
	if ( node != NULL ) {
		node->childType = ND_PROPERTY_STRING;
		for ( unsigned int i = 0; i < numElements; ++i )
			ndPushBackString( node, NULL, array[ i ] );
	}
	return node;
}

NdBranch *ndPushBackBool( NdBranch *parent, const char *name, bool var ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_BOOL );
	if ( node != NULL )
		node->data.buf = AllocVarString( var ? "true" : "false", &node->data.length );

	return node;
}

NdBranch *ndPushBackI8( NdBranch *parent, const char *name, int8_t var ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_I8 );
	if ( node != NULL ) {
		char buf[ 4 ];
		pl_itoa( var, buf, sizeof( buf ), 10 );
		node->data.buf = AllocVarString( buf, &node->data.length );
	}
	return node;
}

NdBranch *ndPushBackI16( NdBranch *parent, const char *name, int16_t var ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_I16 );
	if ( node != NULL ) {
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_int16, var );
		node->data.buf = AllocVarString( buf, &node->data.length );
	}
	return node;
}

NdBranch *ndPushBackI32( NdBranch *parent, const char *name, int32_t var ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_I32 );
	if ( node != NULL ) {
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_int32, var );
		node->data.buf = AllocVarString( buf, &node->data.length );
	}
	return node;
}

NdBranch *ndPushBackUI32( NdBranch *parent, const char *name, uint32_t var ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_UI32 );
	if ( node != NULL ) {
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_uint32, var );
		node->data.buf = AllocVarString( buf, &node->data.length );
	}
	return node;
}

NdBranch *ndPushBackF32( NdBranch *parent, const char *name, float var ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_F32 );
	if ( node != NULL ) {
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_float, var );
		node->data.buf = AllocVarString( buf, &node->data.length );
	}
	return node;
}

NdBranch *ndPushBackF64( NdBranch *parent, const char *name, double var ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_F64 );
	if ( node != NULL ) {
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), PL_FMT_double, var );
		node->data.buf = AllocVarString( buf, &node->data.length );
	}
	return node;
}

NdBranch *ndPushBackI32Array( NdBranch *parent, const char *name, const int32_t *array, unsigned int numElements ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_ARRAY );
	if ( node != NULL ) {
		node->childType = ND_PROPERTY_I32;
		for ( unsigned int i = 0; i < numElements; ++i )
			ndPushBackI32( node, NULL, array[ i ] );
	}
	return node;
}

NdBranch *ndPushBackF32Array( NdBranch *parent, const char *name, const float *array, unsigned int numElements ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_ARRAY );
	if ( node != NULL ) {
		node->childType = ND_PROPERTY_F32;
		for ( unsigned int i = 0; i < numElements; ++i )
			ndPushBackF32( node, NULL, array[ i ] );
	}
	return node;
}

NdBranch *ndPushBackObjectArray( NdBranch *parent, const char *name ) {
	NdBranch *node = ndPushBackNewBranch( parent, name, ND_PROPERTY_ARRAY );
	if ( node != NULL )
		node->childType = ND_PROPERTY_OBJECT;

	return node;
}

static char *CopyVarString( const NdVarString *varString, uint16_t *length ) {
	*length = varString->length;
	char *buf = PL_NEW_( char, *length + 1 );
	strncpy( buf, varString->buf, *length );
	return buf;
}

/**
 * Copies the given node list.
 */
NdBranch *ndCopyBranch( NdBranch *node ) {
	NdBranch *newNode = PL_NEW( NdBranch );
	newNode->type = node->type;
	newNode->childType = node->childType;
	newNode->data.buf = CopyVarString( &node->data, &newNode->data.length );
	newNode->name.buf = CopyVarString( &node->name, &newNode->name.length );
	// Not setting the parent is intentional here, since we likely don't want that link

	NdBranch *child = ndGetFirstChild( node );
	while ( child != NULL ) {
		if ( newNode->linkedList == NULL )
			newNode->linkedList = PlCreateLinkedList();

		NdBranch *newChild = ndCopyBranch( child );
		newChild->linkedListNode = PlInsertLinkedListNode( newNode->linkedList, newChild );
		newChild->parent = newNode;

		child = ndGetNextChild( child );
	}

	return newNode;
}

void ndDestroyBranch( NdBranch *node ) {
	if ( node == NULL ) {
		return;
	}

	PlFree( node->name.buf );
	PlFree( node->data.buf );

	/* if it's an object/array, we'll need to clean up all it's children */
	if ( node->type == ND_PROPERTY_OBJECT || node->type == ND_PROPERTY_ARRAY ) {
		NdBranch *child = ndGetFirstChild( node );
		while ( child != NULL ) {
			NdBranch *nextChild = ndGetNextChild( child );
			ndDestroyBranch( child );
			child = nextChild;
		}
	}

	PlDestroyLinkedList( node->linkedList );
	if ( node->parent != NULL )
		PlDestroyLinkedListNode( node->linkedListNode );

	PlFree( node );
}

/******************************************/
/** Deserialisation **/

static char *DeserializeStringVar( PLFile *file, uint16_t *length ) {
	*length = PlReadInt16( file, false, NULL );
	if ( *length > 0 ) {
		char *buf = PlMAlloc( *length, true );
		PlReadFile( file, buf, sizeof( char ), *length );
		return buf;
	}

	return NULL;
}

static NdBranch *DeserializeBinaryNode( PLFile *file, NdBranch *parent ) {
	/* try to fetch the name, not all nodes necessarily have a name... */
	NdVarString name;
	name.buf = DeserializeStringVar( file, &name.length );
	const char *dname = ( name.buf != NULL ) ? name.buf : "unknown";

	bool status;
	NdPropertyType type = ( NdPropertyType ) PlReadInt8( file, &status );
	if ( !status ) {
		Warning( "Failed to read property type for \"%s\"!\n", dname );
		PlFree( name.buf );
		return NULL;
	}

	/* binary implementation is pretty damn straight forward */
	NdBranch *node = ndPushBackNewBranch( parent, NULL, type );
	if ( node == NULL ) {
		PlFree( name.buf );
		return NULL;
	}

	/* node now takes ownership of name */
	node->name = name;

	switch ( node->type ) {
		default:
			Warning( "Encountered unhandled node type: %d!\n", node->type );
			ndDestroyBranch( node );
			node = NULL;
			break;
		case ND_PROPERTY_ARRAY:
			/* only extra component we get here is the child type */
			node->childType = ( NdPropertyType ) PlReadInt8( file, NULL );
		case ND_PROPERTY_OBJECT: {
			unsigned int numChildren = PlReadInt32( file, false, NULL );
			for ( unsigned int i = 0; i < numChildren; ++i )
				DeserializeBinaryNode( file, node );
			break;
		}
		case ND_PROPERTY_STRING: {
			node->data.buf = DeserializeStringVar( file, &node->data.length );
			break;
		}
		case ND_PROPERTY_BOOL: {
			bool v = PlReadInt8( file, NULL );
			node->data.buf = AllocVarString( v ? "true" : "false", &node->data.length );
			break;
		}
		case ND_PROPERTY_F32: {
			float v = PlReadFloat32( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_float, v );
			node->data.buf = AllocVarString( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_F64: {
			double v = PlReadFloat64( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_double, v );
			node->data.buf = AllocVarString( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_I8: {
			int8_t v = PlReadInt8( file, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_int32, v );
			node->data.buf = AllocVarString( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_I32: {
			int32_t v = PlReadInt32( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_int32, v );
			node->data.buf = AllocVarString( str, &node->data.length );
			break;
		}
		case ND_PROPERTY_I64: {
			int64_t v = PlReadInt64( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), PL_FMT_int64, v );
			node->data.buf = AllocVarString( str, &node->data.length );
			break;
		}
	}

	return node;
}

static NdFileType ParseNodeFileType( PLFile *file ) {
	char token[ 32 ];
	if ( PlReadString( file, token, sizeof( token ) ) == NULL ) {
		SetErrorMessage( ND_ERROR_IO_READ, "Failed to read in file type: %s", PlGetError() );
		return ND_FILE_INVALID;
	}

	if ( strncmp( token, ND_FORMAT_BINARY_HEADER, strlen( ND_FORMAT_BINARY_HEADER ) ) == 0 )
		return ND_FILE_BINARY;
	/* we still check for 'ascii' here, just for backwards compat, but they're handled the
	 * same either way */
	else if ( strncmp( token, ND_FORMAT_ASCII_HEADER, strlen( ND_FORMAT_ASCII_HEADER ) ) == 0 ||
	          strncmp( token, ND_FORMAT_UTF8_HEADER, strlen( ND_FORMAT_UTF8_HEADER ) ) == 0 )
		return ND_FILE_UTF8;

	SetErrorMessage( ND_ERROR_INVALID_ARGUMENT, "Unknown file type \"%s\"", token );
	return ND_FILE_INVALID;
}

NdBranch *ndParseFile( PLFile *file, const char *objectType ) {
	NdBranch *root = NULL;

	NdFileType fileType = ParseNodeFileType( file );
	if ( fileType == ND_FILE_BINARY )
		root = DeserializeBinaryNode( file, NULL );
	else if ( fileType == ND_FILE_UTF8 ) {
		/* first need to run the pre-processor on it */
		size_t length = PlGetFileSize( file );
		if ( length <= strlen( ND_FORMAT_ASCII_HEADER ) )
			Warning( "Unexpected file size, possibly not a valid node file?\n" );
		else {
			const char *data = ( const char * ) ( ( uint8_t * ) PlGetFileData( file ) + strlen( ND_FORMAT_ASCII_HEADER ) );
			char *buf = PL_NEW_( char, length + 1 );
			memcpy( buf, data, length );
			buf = ndPreProcessScript( buf, &length, true );
			root = ndParseBuffer( buf, length );
			PL_DELETE( buf );
		}
	} else
		Warning( "Invalid node file type: %d\n", fileType );

	if ( root != NULL && objectType != NULL ) {
		const char *rootName = ndGetName( root );
		if ( strcmp( rootName, objectType ) != 0 ) {
			/* destroy the tree */
			ndDestroyBranch( root );

			Warning( "Invalid \"%s\" file, expected \"%s\" but got \"%s\"!\n", objectType, objectType, rootName );
			return NULL;
		}
	}

	return root;
}

NdBranch *ndLoadFile( const char *path, const char *objectType ) {
	ClearErrorMessage();

	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL ) {
		Warning( "Failed to open \"%s\": %s\n", path, PlGetError() );
		return NULL;
	}

	NdBranch *root = ndParseFile( file, objectType );

	PlCloseFile( file );

	return root;
}

/******************************************/
/** Serialisation **/

static unsigned int sDepth; /* serialisation depth */

static void WriteLine( FILE *file, const char *string, bool tabify ) {
	if ( tabify ) {
		for ( unsigned int i = 0; i < sDepth; ++i )
			fputc( '\t', file );
	}

	if ( string == NULL )
		return;

	fprintf( file, "%s", string );
}

static void SerializeStringVar( const NdVarString *string, NdFileType fileType, FILE *file ) {
	if ( fileType == ND_FILE_BINARY ) {
		fwrite( &string->length, sizeof( uint16_t ), 1, file );
		/* slightly paranoid here, because strBuf is probably null if length is 0
		 * which is totally valid, but eh */
		if ( string->length > 0 )
			fwrite( string->buf, sizeof( char ), string->length, file );

		return;
	}

	/* allow nameless nodes, used for arrays */
	if ( string->length == 0 )
		return;

	bool encloseString = false;
	const char *c = string->buf;
	if ( *c == '\0' )
		/* enclose an empty string!!! */
		encloseString = true;
	else {
		/* otherwise, check if there are any spaces */
		while ( *c != '\0' ) {
			if ( *c == ' ' ) {
				encloseString = true;
				break;
			}

			c++;
		}
	}

	if ( encloseString )
		fprintf( file, "\"%s\" ", string->buf );
	else
		fprintf( file, "%s ", string->buf );
}

static void SerializeNodeTree( FILE *file, NdBranch *root, NdFileType fileType );
static void SerializeNode( FILE *file, NdBranch *node, NdFileType fileType ) {
	if ( fileType == ND_FILE_UTF8 ) {
		/* write out the line identifying this node */
		WriteLine( file, NULL, true );
		NdBranch *parent = ndGetParent( node );
		if ( parent == NULL || parent->type != ND_PROPERTY_ARRAY ) {
			fprintf( file, "%s ", StringForPropertyType( node->type ) );
			if ( node->type == ND_PROPERTY_ARRAY )
				fprintf( file, "%s ", StringForPropertyType( node->childType ) );

			SerializeStringVar( &node->name, fileType, file );
		}

		/* if this node has children, serialize all those */
		if ( node->type == ND_PROPERTY_OBJECT || node->type == ND_PROPERTY_ARRAY ) {
			WriteLine( file, "{\n", ( parent != NULL && parent->type == ND_PROPERTY_ARRAY ) );
			sDepth++;
			SerializeNodeTree( file, node, fileType );
			sDepth--;
			WriteLine( file, "}\n", true );
		} else {
			SerializeStringVar( &node->data, fileType, file );
			fprintf( file, "\n" );
		}

		return;
	}

	SerializeStringVar( &node->name, fileType, file );
	fwrite( &node->type, sizeof( int8_t ), 1, file );
	switch ( node->type ) {
		default:
			Warning( "Invalid node type: " PL_FMT_uint32 "/n", node->type );
			abort();
		case ND_PROPERTY_F32: {
			float v;
			ndGetF32( node, &v );
			fwrite( &v, sizeof( float ), 1, file );
			break;
		}
		case ND_PROPERTY_F64: {
			double v;
			ndGetF64( node, &v );
			fwrite( &v, sizeof( double ), 1, file );
			break;
		}
		case ND_PROPERTY_I8: {
			int8_t v;
			ndGetI8( node, &v );
			fwrite( &v, sizeof( int8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_I16: {
			int16_t v;
			ndGetI16( node, &v );
			fwrite( &v, sizeof( int16_t ), 1, file );
		}
		case ND_PROPERTY_I32: {
			int32_t v;
			ndGetI32( node, &v );
			fwrite( &v, sizeof( int32_t ), 1, file );
			break;
		}
		case ND_PROPERTY_I64: {
			int64_t v;
			ndGetI64( node, &v );
			fwrite( &v, sizeof( int64_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI8: {
			uint8_t v;
			ndGetUI8( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI16: {
			uint16_t v;
			ndGetUI16( node, &v );
			fwrite( &v, sizeof( uint16_t ), 1, file );
		}
		case ND_PROPERTY_UI32: {
			uint32_t v;
			ndGetUI32( node, &v );
			fwrite( &v, sizeof( uint32_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI64: {
			uint64_t v;
			ndGetUI64( node, &v );
			fwrite( &v, sizeof( uint64_t ), 1, file );
			break;
		}
		case ND_PROPERTY_STRING: {
			SerializeStringVar( &node->data, fileType, file );
			break;
		}
		case ND_PROPERTY_BOOL: {
			bool v;
			ndGetBool( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_ARRAY:
			/* only extra component here is the child type */
			fwrite( &node->childType, sizeof( uint8_t ), 1, file );
		case ND_PROPERTY_OBJECT: {
			uint32_t i = PlGetNumLinkedListNodes( node->linkedList );
			fwrite( &i, sizeof( uint32_t ), 1, file );
			SerializeNodeTree( file, node, fileType );
			break;
		}
	}
}

static void SerializeNodeTree( FILE *file, NdBranch *root, NdFileType fileType ) {
	PLLinkedListNode *i = PlGetFirstNode( root->linkedList );
	while ( i != NULL ) {
		NdBranch *node = PlGetLinkedListNodeUserData( i );
		SerializeNode( file, node, fileType );
		i = PlGetNextLinkedListNode( i );
	}
}

/**
 * Serialize the given node set.
 */
bool ndWriteFile( const char *path, NdBranch *root, NdFileType fileType ) {
	FILE *file = fopen( path, "wb" );
	if ( file == NULL ) {
		SetErrorMessage( ND_ERROR_IO_WRITE, "Failed to open path \"%s\"", path );
		return false;
	}

	if ( fileType == ND_FILE_BINARY )
		fprintf( file, ND_FORMAT_BINARY_HEADER "\n" );
	else {
		sDepth = 0;
		fprintf( file, ND_FORMAT_UTF8_HEADER "\n; this node file has been auto-generated!\n" );
	}

	SerializeNode( file, root, fileType );

	fclose( file );

	return true;
}

/******************************************/
/** API Testing **/

void ndPrintTree( NdBranch *node, int index ) {
	for ( int i = 0; i < index; ++i ) printf( "\t" );
	if ( node->type == ND_PROPERTY_OBJECT || node->type == ND_PROPERTY_ARRAY ) {
		index++;

		const char *name = ( node->name.buf != NULL ) ? node->name.buf : "";
		if ( node->type == ND_PROPERTY_OBJECT )
			Message( "%s (%s)\n", name, StringForPropertyType( node->type ) );
		else
			Message( "%s (%s %s)\n", name, StringForPropertyType( node->type ), StringForPropertyType( node->childType ) );

		NdBranch *child = ndGetFirstChild( node );
		while ( child != NULL ) {
			ndPrintTree( child, index );
			child = ndGetNextChild( child );
		}
	} else {
		NdBranch *parent = ndGetParent( node );
		if ( parent != NULL && parent->type == ND_PROPERTY_ARRAY )
			Message( "%s %s\n", StringForPropertyType( node->type ), node->data.buf );
		else
			Message( "%s %s %s\n", StringForPropertyType( node->type ), node->name.buf, node->data.buf );
	}
}
