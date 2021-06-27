/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plcore/pl_linkedlist.h>
#include <plcore/pl_filesystem.h>

#include "node_private.h"

#define NL_VERSION       1
#define NL_BINARY_HEADER "node.bin"
#define NL_ASCII_HEADER  "node.ascii"
#define NL_UTF8_HEADER   "node.utf8"

static const char *StringForPropertyType( NLPropertyType propertyType )
{
	const char *propToStr[ NL_MAX_PROPERTYTYPES ] = {
	        // Special types
	        [NL_PROP_OBJ]   = "object",
	        [NL_PROP_STR]   = "string",
	        [NL_PROP_BOOL]  = "bool",
	        [NL_PROP_ARRAY] = "array",
	        // Generic types
	        [NL_PROP_I8]   = "int8",
	        [NL_PROP_I16]  = "int16",
	        [NL_PROP_I32]  = "integer",
	        [NL_PROP_I64]  = "int64",
	        [NL_PROP_UI8]  = "uint8",
	        [NL_PROP_UI16] = "uint16",
	        [NL_PROP_UI32] = "uint32",
	        [NL_PROP_UI64] = "uint64",
	        [NL_PROP_F32]  = "float",
	        [NL_PROP_F64]  = "float64",
	};

	if ( propertyType == NL_PROP_UNDEFINED )
		return "undefined";

	return propToStr[ propertyType ];
}

static char *      nlErrorMsg  = NULL;
static NLErrorCode nlErrorType = NL_ERROR_SUCCESS;
static void        NL_ClearErrorMessage( void )
{
	free( nlErrorMsg );
	nlErrorMsg  = NULL;
	nlErrorType = NL_ERROR_SUCCESS;
}

static void NL_SetErrorMessage( NLErrorCode type, const char *msg, ... )
{
	NL_ClearErrorMessage();

	nlErrorType = type;

	va_list args;
	va_start( args, msg );

	int length = pl_vscprintf( msg, args ) + 1;
	if ( length <= 0 )
		return;

	nlErrorMsg = calloc( 1, length );
	if ( nlErrorMsg == NULL )
	{
		Warning( "Failed to allocate error message buffer: %d bytes!\n", length );
		return;
	}

	vsnprintf( nlErrorMsg, length, msg, args );
	Warning( "NLERR: %s\n", nlErrorMsg );

	va_end( args );
}

const char *NL_GetErrorMessage( void ) { return nlErrorMsg; }
NLErrorCode NL_GetError( void ) { return nlErrorType; }

static char *AllocVarString( const char *string, unsigned int *lengthOut )
{
	*lengthOut = ( unsigned int ) strlen( string ) + 1;
	char *buf  = calloc( 1, *lengthOut );
	if ( buf == NULL )
		abort();
	strcpy( buf, string );
	return buf;
}

unsigned int NL_GetNumOfChildren( const NLNode *parent )
{
	return PlGetNumLinkedListNodes( parent->linkedList );
}

NLNode *NL_GetFirstChild( NLNode *parent )
{
	PLLinkedListNode *n = PlGetFirstNode( parent->linkedList );
	if ( n == NULL )
		return NULL;

	return PlGetLinkedListNodeUserData( n );
}

NLNode *NL_GetNextChild( NLNode *node )
{
	PLLinkedListNode *n = PlGetNextLinkedListNode( node->linkedListNode );
	if ( n == NULL )
		return NULL;

	return PlGetLinkedListNodeUserData( n );
}

NLNode *NL_GetChildByName( NLNode *parent, const char *name )
{
	if ( parent->type != NL_PROP_OBJ )
	{
		NL_SetErrorMessage( NL_ERROR_INVALID_TYPE, "Attempted to get child from an invalid node type!\n" );
		return NULL;
	}

	NLNode *child = NL_GetFirstChild( parent );
	while ( child != NULL )
	{
		if ( strcmp( name, child->name.strBuf ) == 0 )
			return child;

		child = NL_GetNextChild( child );
	}

	return NULL;
}

NLNode *NL_GetChildByIndex( NLNode *parent, unsigned int i )
{
	if ( parent->type != NL_PROP_ARRAY )
	{
		NL_SetErrorMessage( NL_ERROR_INVALID_TYPE, "Attempted to get child from an invalid node type!\n" );
		return NULL;
	}

	/* todo: optimise this... */

	unsigned int curPos = 0;

	NLNode *child = NL_GetFirstChild( parent );
	while ( child != NULL )
	{
		if ( curPos == i )
			return child;

		child = NL_GetNextChild( parent );
		curPos++;
	}

	return NULL;
}

static const NLVarString *GetValueByName( NLNode *root, const char *name )
{
	const NLNode *field = NL_GetChildByName( root, name );
	if ( field == NULL )
		return NULL;

	return &field->data;
}

NLNode *NL_GetParent( NLNode *node )
{
	return node->parent;
}

const char *NL_GetName( const NLNode *node )
{
	return node->name.strBuf;
}

NLPropertyType NL_GetType( const NLNode *node )
{
	return node->type;
}

NLErrorCode NL_GetStr( const NLNode *node, char *dest, size_t length )
{
	if ( node->type != NL_PROP_STR ) return NL_ERROR_INVALID_TYPE;
	snprintf( dest, length, "%s", node->data.strBuf );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetBool( const NLNode *node, bool *dest )
{
	if ( node->type != NL_PROP_BOOL ) return NL_ERROR_INVALID_TYPE;

	if ( ( strcmp( node->data.strBuf, "true" ) == 0 ) || ( node->data.strBuf[ 0 ] == '1' && node->data.strBuf[ 1 ] == '\0' ) )
	{
		*dest = true;
		return NL_ERROR_SUCCESS;
	}
	else if ( ( strcmp( node->data.strBuf, "false" ) == 0 ) || ( node->data.strBuf[ 0 ] == '0' && node->data.strBuf[ 1 ] == '\0' ) )
	{
		*dest = false;
		return NL_ERROR_SUCCESS;
	}

	NL_SetErrorMessage( NL_ERROR_INVALID_ARGUMENT, "Invalid data passed from var" );
	return NL_ERROR_INVALID_ARGUMENT;
}

NLErrorCode NL_GetF32( const NLNode *node, float *dest )
{
	if ( node->type != NL_PROP_F32 ) return NL_ERROR_INVALID_TYPE;
	*dest = strtof( node->data.strBuf, NULL );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetF64( const NLNode *node, double *dest )
{
	if ( node->type != NL_PROP_F64 ) return NL_ERROR_INVALID_TYPE;
	*dest = strtod( node->data.strBuf, NULL );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetI8( const NLNode *node, int8_t *dest )
{
	if ( node->type != NL_PROP_I8 ) return NL_ERROR_INVALID_TYPE;
	*dest = ( int8_t ) strtol( node->data.strBuf, NULL, 10 );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetI16( const NLNode *node, int16_t *dest )
{
	if ( node->type != NL_PROP_I16 ) return NL_ERROR_INVALID_TYPE;
	*dest = ( int16_t ) strtol( node->data.strBuf, NULL, 10 );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetI32( const NLNode *node, int32_t *dest )
{
	if ( node->type != NL_PROP_I32 ) return NL_ERROR_INVALID_TYPE;
	*dest = strtol( node->data.strBuf, NULL, 10 );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetI64( const NLNode *node, int64_t *dest )
{
	if ( node->type != NL_PROP_I64 ) return NL_ERROR_INVALID_TYPE;
	*dest = strtoll( node->data.strBuf, NULL, 10 );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetUI8( const NLNode *node, uint8_t *dest )
{
	if ( node->type != NL_PROP_UI8 ) return NL_ERROR_INVALID_TYPE;
	*dest = ( uint8_t ) strtoul( node->data.strBuf, NULL, 10 );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetUI16( const NLNode *node, uint16_t *dest )
{
	if ( node->type != NL_PROP_UI16 ) return NL_ERROR_INVALID_TYPE;
	*dest = ( uint16_t ) strtoul( node->data.strBuf, NULL, 10 );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetUI32( const NLNode *node, uint32_t *dest )
{
	if ( node->type != NL_PROP_UI32 ) return NL_ERROR_INVALID_TYPE;
	*dest = strtoul( node->data.strBuf, NULL, 10 );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetUI64( const NLNode *node, uint64_t *dest )
{
	if ( node->type != NL_PROP_UI64 ) return NL_ERROR_INVALID_TYPE;
	*dest = strtoull( node->data.strBuf, NULL, 10 );
	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetI8Array( NLNode *parent, int8_t *buf, unsigned int numElements )
{
	if ( parent->type != NL_PROP_ARRAY || parent->childType != NL_PROP_I8 )
		return NL_ERROR_INVALID_TYPE;

	NLNode *child = NL_GetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return NL_ERROR_INVALID_ELEMENTS;

		NLErrorCode errorCode = NL_GetI8( child, &buf[ i ] );
		if ( errorCode != NL_ERROR_SUCCESS )
			return errorCode;

		child = NL_GetNextChild( child );
	}

	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetI16Array( NLNode *parent, int16_t *buf, unsigned int numElements )
{
	if ( parent->type != NL_PROP_ARRAY || parent->childType != NL_PROP_I16 )
		return NL_ERROR_INVALID_TYPE;

	NLNode *child = NL_GetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return NL_ERROR_INVALID_ELEMENTS;

		NLErrorCode errorCode = NL_GetI16( child, &buf[ i ] );
		if ( errorCode != NL_ERROR_SUCCESS )
			return errorCode;

		child = NL_GetNextChild( child );
	}

	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetI32Array( NLNode *parent, int32_t *buf, unsigned int numElements )
{
	if ( parent->type != NL_PROP_ARRAY || parent->childType != NL_PROP_I32 )
		return NL_ERROR_INVALID_TYPE;

	NLNode *child = NL_GetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return NL_ERROR_INVALID_ELEMENTS;

		NLErrorCode errorCode = NL_GetI32( child, &buf[ i ] );
		if ( errorCode != NL_ERROR_SUCCESS )
			return errorCode;

		child = NL_GetNextChild( child );
	}

	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetUI32Array( NLNode *parent, uint32_t *buf, unsigned int numElements )
{
	if ( parent->type != NL_PROP_ARRAY || parent->childType != NL_PROP_UI32 )
		return NL_ERROR_INVALID_TYPE;

	NLNode *child = NL_GetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return NL_ERROR_INVALID_ELEMENTS;

		NLErrorCode errorCode = NL_GetUI32( child, &buf[ i ] );
		if ( errorCode != NL_ERROR_SUCCESS )
			return errorCode;

		child = NL_GetNextChild( child );
	}

	return NL_ERROR_SUCCESS;
}

NLErrorCode NL_GetF32Array( NLNode *parent, float *buf, unsigned int numElements )
{
	if ( parent->type != NL_PROP_ARRAY || parent->childType != NL_PROP_F32 )
		return NL_ERROR_INVALID_TYPE;

	NLNode *child = NL_GetFirstChild( parent );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return NL_ERROR_INVALID_ELEMENTS;

		NLErrorCode errorCode = NL_GetF32( child, &buf[ i ] );
		if ( errorCode != NL_ERROR_SUCCESS )
			return errorCode;

		child = NL_GetNextChild( child );
	}

	return NL_ERROR_SUCCESS;
}

/******************************************/
/** Get: ByName **/

const char *NL_GetStrByName( NLNode *node, const char *name, const char *fallback )
{
	const NLVarString *var = GetValueByName( node, name );
	return ( var != NULL ) ? var->strBuf : fallback;
}

float NL_GetF32ByName( NLNode *node, const char *name, float fallback )
{
	const NLVarString *var = GetValueByName( node, name );
	return ( var != NULL ) ? strtof( var->strBuf, NULL ) : fallback;
}

int32_t NL_GetI32ByName( NLNode *node, const char *name, int32_t fallback )
{
	const NLVarString *var = GetValueByName( node, name );
	return ( var != NULL ) ? strtol( var->strBuf, NULL, 10 ) : fallback;
}

/******************************************/

NLNode *xNL_PushBackNode( NLNode *parent, const char *name, NLPropertyType propertyType )
{
	/* arrays are special cases */
	if ( parent != NULL && parent->type == NL_PROP_ARRAY && propertyType != parent->childType )
	{
		NL_SetErrorMessage( NL_ERROR_INVALID_TYPE, "Attempted to add " );
		return NULL;
	}

	NLNode *node = calloc( 1, sizeof( NLNode ) );
	if ( node == NULL )
	{
		NL_SetErrorMessage( NL_ERROR_MEM_ALLOC, "failed to allocate node" );
		return NULL;
	}

	/* assign the node name, if provided */
	if ( ( parent == NULL || parent->type != NL_PROP_ARRAY ) && name != NULL )
		node->name.strBuf = AllocVarString( name, &node->name.strBufLength );

	node->type       = propertyType;
	node->linkedList = PlCreateLinkedList();

	/* if root is provided, this is treated as a child of that node */
	if ( parent != NULL )
	{
		if ( parent->linkedList == NULL )
			parent->linkedList = PlCreateLinkedList();

		node->linkedListNode = PlInsertLinkedListNode( parent->linkedList, node );
		node->parent         = parent;
	}

	return node;
}

NLNode *NL_PushBackObj( NLNode *node, const char *name )
{
	return xNL_PushBackNode( node, name, NL_PROP_OBJ );
}

NLNode *NL_PushBackStr( NLNode *parent, const char *name, const char *var )
{
	NLNode *node = xNL_PushBackNode( parent, name, NL_PROP_STR );
	if ( node != NULL )
		node->data.strBuf = AllocVarString( var, &node->data.strBufLength );

	return node;
}

NLNode *NL_PushBackStrArray( NLNode *parent, const char *name, const char **array, unsigned int numElements )
{
	NLNode *node = xNL_PushBackNode( parent, name, NL_PROP_ARRAY );
	if ( node != NULL )
	{
		node->childType = NL_PROP_STR;
		for ( unsigned int i = 0; i < numElements; ++i )
			NL_PushBackStr( node, NULL, array[ i ] );
	}
	return node;
}

NLNode *NL_PushBackBool( NLNode *parent, const char *name, bool var )
{
	NLNode *node = xNL_PushBackNode( parent, name, NL_PROP_BOOL );
	if ( node != NULL )
		node->data.strBuf = AllocVarString( var ? "true" : "false", &node->data.strBufLength );

	return node;
}

NLNode *NL_PushBackI32( NLNode *parent, const char *name, int32_t var )
{
	NLNode *node = xNL_PushBackNode( parent, name, NL_PROP_I32 );
	if ( node != NULL )
	{
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), "%d", var );
		node->data.strBuf = AllocVarString( buf, &node->data.strBufLength );
	}
	return node;
}

NLNode *NL_PushBackF32( NLNode *parent, const char *name, float var )
{
	NLNode *node = xNL_PushBackNode( parent, name, NL_PROP_F32 );
	if ( node != NULL )
	{
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), "%f", var );
		node->data.strBuf = AllocVarString( buf, &node->data.strBufLength );
	}
	return node;
}

NLNode *NL_PushBackI32Array( NLNode *parent, const char *name, const int *array, unsigned int numElements )
{
	NLNode *node = xNL_PushBackNode( parent, name, NL_PROP_ARRAY );
	if ( node != NULL )
	{
		node->childType = NL_PROP_I32;
		for ( unsigned int i = 0; i < numElements; ++i )
			NL_PushBackI32( node, NULL, array[ i ] );
	}
	return node;
}

NLNode *NL_PushBackF32Array( NLNode *parent, const char *name, const float *array, unsigned int numElements )
{
	NLNode *node = xNL_PushBackNode( parent, name, NL_PROP_ARRAY );
	if ( node != NULL )
	{
		node->childType = NL_PROP_F32;
		for ( unsigned int i = 0; i < numElements; ++i )
			NL_PushBackF32( node, NULL, array[ i ] );
	}
	return node;
}

NLNode *NL_PushBackObjArray( NLNode *parent, const char *name )
{
	NLNode *node = xNL_PushBackNode( parent, name, NL_PROP_ARRAY );
	if ( node != NULL )
		node->childType = NL_PROP_OBJ;

	return node;
}

static char *CopyVarString( const NLVarString *varString, unsigned int *length )
{
	*length   = varString->strBufLength;
	char *buf = malloc( *length );
	strncpy( buf, varString->strBuf, *length );
	return buf;
}

/**
 * Copies the given node list.
 */
NLNode *NL_CopyNode( NLNode *node )
{
	NLNode *newNode      = malloc( sizeof( NLNode ) );
	newNode->type        = node->type;
	newNode->childType   = node->childType;
	newNode->data.strBuf = CopyVarString( &node->data, &newNode->data.strBufLength );
	newNode->name.strBuf = CopyVarString( &node->name, &newNode->name.strBufLength );
	// Not setting the parent is intentional here, since we likely don't want that link

	NLNode *child = NL_GetFirstChild( node );
	while ( child != NULL )
	{
		if ( newNode->linkedList == NULL )
			newNode->linkedList = PlCreateLinkedList();

		NLNode *newChild         = NL_CopyNode( child );
		newChild->linkedListNode = PlInsertLinkedListNode( newNode->linkedList, newChild );
		newChild->parent         = newNode;

		child = NL_GetNextChild( child );
	}

	return newNode;
}

void NL_DestroyNode( NLNode *node )
{
	pl_free( node->name.strBuf );
	pl_free( node->data.strBuf );

	/* if it's an object/array, we'll need to clean up all it's children */
	if ( node->type == NL_PROP_OBJ || node->type == NL_PROP_ARRAY )
	{
		NLNode *child = NL_GetFirstChild( node );
		while ( child != NULL )
		{
			NLNode *nextChild = NL_GetNextChild( child );
			NL_DestroyNode( child );
			child = nextChild;
		}
	}

	PlDestroyLinkedList( node->linkedList );
	if ( node->parent != NULL )
		PlDestroyLinkedListNode( node->parent->linkedList, node->linkedListNode );

	pl_free( node );
}

/******************************************/
/** Deserialisation **/

char *DeserializeStringVar( PLFile *file, unsigned int *length )
{
	*length = PlReadInt32( file, false, NULL );
	if ( *length > 0 )
	{
		char *buf = malloc( *length );
		PlReadFile( file, buf, sizeof( char ), *length );
		return buf;
	}

	return NULL;
}

static NLNode *DeserializeBinaryNode( PLFile *file, NLNode *parent )
{
	/* binary implementation is pretty damn straight forward */
	NLNode *node      = xNL_PushBackNode( parent, NULL, NL_PROP_UNDEFINED );
	node->name.strBuf = DeserializeStringVar( file, &node->name.strBufLength );

	node->type = ( NLPropertyType ) PlReadInt8( file, NULL );
	switch ( node->type )
	{
		default:
			Warning( "Encountered unhandled node type: %d!\n", node->type );
			NL_DestroyNode( node );
			node = NULL;
			break;
		case NL_PROP_ARRAY:
			/* only extra component we get here is the child type */
			node->childType = ( NLPropertyType ) PlReadInt8( file, NULL );
		case NL_PROP_OBJ:
		{
			unsigned int numChildren = PlReadInt32( file, false, NULL );
			for ( unsigned int i = 0; i < numChildren; ++i )
				DeserializeBinaryNode( file, node );
			break;
		}
		case NL_PROP_STR:
		{
			node->data.strBuf = DeserializeStringVar( file, &node->data.strBufLength );
			break;
		}
		case NL_PROP_BOOL:
		{
			bool v            = PlReadInt8( file, NULL );
			node->data.strBuf = AllocVarString( v ? "true" : "false", &node->data.strBufLength );
			break;
		}
		case NL_PROP_F32:
		{
			float v = ( float ) PlReadInt32( file, false, NULL );
			char  str[ 32 ];
			snprintf( str, sizeof( str ), COM_FMT_float, v );
			node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
			break;
		}
		case NL_PROP_F64:
		{
			double v = ( double ) PlReadInt64( file, false, NULL );
			char   str[ 32 ];
			snprintf( str, sizeof( str ), COM_FMT_double, v );
			node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
			break;
		}
		case NL_PROP_I8:
		{
			int8_t v = PlReadInt8( file, NULL );
			char   str[ 32 ];
			snprintf( str, sizeof( str ), COM_FMT_int32, v );
			node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
			break;
		}
		case NL_PROP_I32:
		{
			int32_t v = PlReadInt32( file, false, NULL );
			char    str[ 32 ];
			snprintf( str, sizeof( str ), COM_FMT_int32, v );
			node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
			break;
		}
		case NL_PROP_I64:
		{
			int64_t v = PlReadInt64( file, false, NULL );
			char    str[ 32 ];
			snprintf( str, sizeof( str ), COM_FMT_int64, v );
			node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
			break;
		}
	}

	return node;
}

static NLFileType ParseNodeFileType( PLFile *file )
{
	char token[ 32 ];
	if ( PlReadString( file, token, sizeof( token ) ) == NULL )
	{
		NL_SetErrorMessage( NL_ERROR_IO_READ, "Failed to read in file type: %s", PlGetError() );
		return NL_FILE_INVALID;
	}

	if ( strncmp( token, NL_BINARY_HEADER, strlen( NL_BINARY_HEADER ) ) == 0 )
		return NL_FILE_BINARY;
	else if ( strncmp( token, NL_ASCII_HEADER, strlen( NL_ASCII_HEADER ) ) == 0 )
		return NL_FILE_ASCII;

	NL_SetErrorMessage( NL_ERROR_INVALID_ARGUMENT, "Unknown file type \"%s\"", token );
	return NL_FILE_INVALID;
}

NLNode *NL_LoadFile( const char *path, const char *objectType )
{
	NL_ClearErrorMessage();

	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		Warning( "Failed to open \"%s\": %s\n", path, PlGetError() );
		return NULL;
	}

	NLNode *root = NULL;

	NLFileType fileType = ParseNodeFileType( file );
	if ( fileType == NL_FILE_BINARY )
		root = DeserializeBinaryNode( file, NULL );
	else if ( fileType == NL_FILE_ASCII )
	{
		/* first need to run the pre-processor on it */
		size_t      length = PlGetFileSize( file );
		const char *data   = ( const char * ) PlGetFileData( file ) + strlen( NL_ASCII_HEADER );
		char *      buf    = pl_malloc( length );
		memcpy( buf, data, length );
		buf  = xNL_PreProcessScript( buf, &length, true );
		root = NL_ParseBuffer( buf, length );
		pl_free( buf );
	}
	else
		Warning( "Invalid node file type: %d\n", fileType );

	PlCloseFile( file );

	if ( root != NULL && objectType != NULL )
	{
		const char *rootName = NL_GetName( root );
		if ( strcmp( rootName, objectType ) != 0 )
		{
			/* destroy the tree */
			NL_DestroyNode( root );

			Warning( "Invalid \"%s\" file, expected \"%s\" but got \"%s\"!\n", objectType, objectType, rootName );
			return NULL;
		}
	}

	return root;
}

/******************************************/
/** Serialisation **/

static unsigned int sDepth; /* serialisation depth */

static void WriteLine( FILE *file, const char *string, bool tabify )
{
	if ( tabify )
	{
		for ( unsigned int i = 0; i < sDepth; ++i )
			fputc( '\t', file );
	}

	if ( string == NULL )
		return;

	fprintf( file, "%s", string );
}

static void SerializeStringVar( const NLVarString *string, NLFileType fileType, FILE *file )
{
	if ( fileType == NL_FILE_BINARY )
	{
		fwrite( &string->strBufLength, sizeof( uint32_t ), 1, file );
		/* slightly paranoid here, because strBuf is probably null if length is 0
		 * which is totally valid, but eh */
		if ( string->strBufLength > 0 )
			fwrite( string->strBuf, sizeof( char ), string->strBufLength, file );

		return;
	}

	/* allow nameless nodes, used for arrays */
	if ( string->strBufLength == 0 )
		return;

	bool        encloseString = false;
	const char *c             = string->strBuf;
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
		fprintf( file, "\"%s\" ", string->strBuf );
	else
		fprintf( file, "%s ", string->strBuf );
}

static void SerializeNodeTree( FILE *file, NLNode *root, NLFileType fileType );
static void SerializeNode( FILE *file, NLNode *node, NLFileType fileType )
{
	if ( fileType == NL_FILE_ASCII )
	{
		/* write out the line identifying this node */
		WriteLine( file, NULL, true );
		NLNode *parent = NL_GetParent( node );
		if ( parent == NULL || parent->type != NL_PROP_ARRAY )
		{
			fprintf( file, "%s ", StringForPropertyType( node->type ) );
			if ( node->type == NL_PROP_ARRAY )
			{
				fprintf( file, "%s ", StringForPropertyType( node->childType ) );
			}
			SerializeStringVar( &node->name, fileType, file );
		}

		/* if this node has children, serialize all those */
		if ( node->type == NL_PROP_OBJ || node->type == NL_PROP_ARRAY )
		{
			WriteLine( file, "{\n", ( parent != NULL && parent->type == NL_PROP_ARRAY ) );
			sDepth++;
			SerializeNodeTree( file, node, fileType );
			sDepth--;
			WriteLine( file, "}\n", true );
		}
		else
		{
			SerializeStringVar( &node->data, fileType, file );
			fprintf( file, "\n" );
		}

		return;
	}

	SerializeStringVar( &node->name, fileType, file );
	fwrite( &node->type, sizeof( int8_t ), 1, file );
	switch ( node->type )
	{
		case NL_PROP_F32:
		{
			float v;
			NL_GetF32( node, &v );
			fwrite( &v, sizeof( float ), 1, file );
			break;
		}
		case NL_PROP_F64:
		{
			double v;
			NL_GetF64( node, &v );
			fwrite( &v, sizeof( double ), 1, file );
			break;
		}
		case NL_PROP_I8:
		{
			int8_t v;
			NL_GetI8( node, &v );
			fwrite( &v, sizeof( int8_t ), 1, file );
			break;
		}
		case NL_PROP_I16:
		{
			int16_t v;
			NL_GetI16( node, &v );
			fwrite( &v, sizeof( int16_t ), 1, file );
		}
		case NL_PROP_I32:
		{
			int32_t v;
			NL_GetI32( node, &v );
			fwrite( &v, sizeof( int32_t ), 1, file );
			break;
		}
		case NL_PROP_I64:
		{
			int64_t v;
			NL_GetI64( node, &v );
			fwrite( &v, sizeof( int64_t ), 1, file );
			break;
		}
		case NL_PROP_UI8:
		{
			uint8_t v;
			NL_GetUI8( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case NL_PROP_UI16:
		{
			uint16_t v;
			NL_GetUI16( node, &v );
			fwrite( &v, sizeof( uint16_t ), 1, file );
		}
		case NL_PROP_UI32:
		{
			uint32_t v;
			NL_GetUI32( node, &v );
			fwrite( &v, sizeof( uint32_t ), 1, file );
			break;
		}
		case NL_PROP_UI64:
		{
			uint64_t v;
			NL_GetUI64( node, &v );
			fwrite( &v, sizeof( uint64_t ), 1, file );
			break;
		}
		case NL_PROP_STR:
		{
			SerializeStringVar( &node->data, fileType, file );
			break;
		}
		case NL_PROP_BOOL:
		{
			bool v;
			NL_GetBool( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case NL_PROP_ARRAY:
			/* only extra component here is the child type */
			fwrite( &node->childType, sizeof( uint8_t ), 1, file );
		case NL_PROP_OBJ:
		{
			uint32_t i = PlGetNumLinkedListNodes( node->linkedList );
			fwrite( &i, sizeof( uint32_t ), 1, file );
			SerializeNodeTree( file, node, fileType );
			break;
		}
	}
}

static void SerializeNodeTree( FILE *file, NLNode *root, NLFileType fileType )
{
	PLLinkedListNode *i = PlGetFirstNode( root->linkedList );
	while ( i != NULL )
	{
		NLNode *node = PlGetLinkedListNodeUserData( i );
		SerializeNode( file, node, fileType );
		i = PlGetNextLinkedListNode( i );
	}
}

/**
 * Serialize the given node set.
 */
void NL_WriteFile( const char *path, NLNode *root, NLFileType fileType )
{
	FILE *file = fopen( path, "wb" );
	if ( file == NULL )
	{
		NL_SetErrorMessage( NL_ERROR_IO_WRITE, "Failed to open path \"%s\"", path );
		return;
	}

	if ( fileType == NL_FILE_BINARY )
		fprintf( file, NL_BINARY_HEADER "\n" );
	else
	{
		sDepth = 0;
		fprintf( file, NL_ASCII_HEADER "\n; this node file has been auto-generated!\n" );
	}

	SerializeNode( file, root, fileType );

	fclose( file );
}

/******************************************/
/** API Testing **/

void NL_PrintNodeTree( NLNode *node, int index )
{
	for ( int i = 0; i < index; ++i ) printf( "\t" );
	if ( node->type == NL_PROP_OBJ || node->type == NL_PROP_ARRAY )
	{
		index++;

		const char *name = ( node->name.strBuf != NULL ) ? node->name.strBuf : "";
		if ( node->type == NL_PROP_OBJ )
			Message( "%s (%s)\n", name, StringForPropertyType( node->type ) );
		else
			Message( "%s (%s %s)\n", name, StringForPropertyType( node->type ), StringForPropertyType( node->childType ) );

		NLNode *child = NL_GetFirstChild( node );
		while ( child != NULL )
		{
			NL_PrintNodeTree( child, index );
			child = NL_GetNextChild( child );
		}
	}
	else
	{
		NLNode *parent = NL_GetParent( node );
		if ( parent != NULL && parent->type == NL_PROP_ARRAY )
			Message( "%s %s\n", StringForPropertyType( node->type ), node->data.strBuf );
		else
			Message( "%s %s %s\n", StringForPropertyType( node->type ), node->name.strBuf, node->data.strBuf );
	}
}
