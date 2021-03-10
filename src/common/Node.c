/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>
#include <PL/pl_parse.h>
#include <PL/platform_filesystem.h>

#include "NodePrivate.h"

#define NL_VERSION 1
#define NL_BINARY_HEADER "node.bin"
#define NL_ASCII_HEADER "node.ascii"
#define NL_UTF8_HEADER "node.utf8"

static const char *StringForPropertyType( NLPropertyType propertyType ) {
	switch ( propertyType ) {
		case NODE_PROPERTY_INTEGER:
			return "integer";
		case NODE_PROPERTY_FLOAT:
			return "float";
		case NODE_PROPERTY_STRING:
			return "string";
		case NODE_PROPERTY_BOOLEAN:
			return "bool";
		case NODE_PROPERTY_OBJECT:
			return "object";
		case NODE_PROPERTY_ARRAY:
			return "array";
		default:
			return "invalid";
	}
}

static char *nlErrorMsg = NULL;
static NLErrorCode nlErrorType = NL_ERROR_SUCCESS;
static void NL_ClearErrorMessage( void ) {
	free( nlErrorMsg );
	nlErrorMsg = NULL;
	nlErrorType = NL_ERROR_SUCCESS;
}
static void NL_SetErrorMessage( NLErrorCode type, const char *msg, ... ) {
	nlErrorType = type;

	va_list args;
	va_start( args, msg );

	int length = pl_vscprintf( msg, args ) + 1;
	if ( length <= 0 ) {
		return;
	}

	nlErrorMsg = calloc( 1, length );
	if ( nlErrorMsg == NULL ) {
		Warning( "Failed to allocate error message buffer: %d bytes!\n", length );
		return;
	}

	vsnprintf( nlErrorMsg, length, msg, args );
	Warning( "NLERR: %s\n", nlErrorMsg );

	va_end( args );
}

const char *NL_GetErrorMessage( void ) { return nlErrorMsg; }
NLErrorCode NL_GetError( void ) { return nlErrorType; }

static char *AllocVarString( const char *string, unsigned int *lengthOut ) {
	*lengthOut = strlen( string ) + 1;
	char *buf = calloc( 1, *lengthOut );
	strcpy( buf, string );
	return buf;
}

unsigned int NL_GetNumOfChildren( const NLNode *parent ) {
	return plGetNumLinkedListNodes( parent->linkedList );
}

NLNode *NL_GetFirstChild( NLNode *parent ) {
	PLLinkedListNode *n = plGetFirstNode( parent->linkedList );
	if ( n == NULL ) {
		return NULL;
	}

	return plGetLinkedListNodeUserData( n );
}

NLNode *NL_GetNextChild( NLNode *node ) {
	PLLinkedListNode *n = plGetNextLinkedListNode( node->linkedListNode );
	if ( n == NULL ) {
		return NULL;
	}

	return plGetLinkedListNodeUserData( n );
}

NLNode *NL_GetChildByName( NLNode *parent, const char *name ) {
	if ( parent->type != NODE_PROPERTY_OBJECT ) {
		NL_SetErrorMessage( NL_ERROR_INVALID_TYPE, "Attempted to get child from an invalid node type!\n" );
		return NULL;
	}

	NLNode *child = NL_GetFirstChild( parent );
	while ( child != NULL ) {
		if ( strcmp( name, child->name.strBuf ) == 0 ) {
			return child;
		}

		child = NL_GetNextChild( parent );
	}

	return NULL;
}

NLNode *NL_GetChildByIndex( NLNode *parent, unsigned int i ) {
	if ( parent->type != NODE_PROPERTY_ARRAY ) {
		NL_SetErrorMessage( NL_ERROR_INVALID_TYPE, "Attempted to get child from an invalid node type!\n" );
		return NULL;
	}

	/* todo: optimise this... */

	unsigned int curPos = 0;

	NLNode *child = NL_GetFirstChild( parent );
	while ( child != NULL ) {
		if ( curPos == i ) {
			return child;
		}

		child = NL_GetNextChild( parent );
		curPos++;
	}

	return NULL;
}

NLNode *NL_GetParent( NLNode *node ) {
	return node->parent;
}

const char *NL_GetName( const NLNode *node ) {
	return node->name.strBuf;
}

NLPropertyType NL_GetType( const NLNode *node ) {
	return node->type;
}

const char *NL_GetString( const NLNode *node ) {
	return node->data.strBuf;
}

bool NL_GetBoolean( const NLNode *node ) {
	if ( ( strcmp( node->data.strBuf, "true" ) == 0 ) || ( node->data.strBuf[ 0 ] == '1' && node->data.strBuf[ 1 ] == '\0' ) ) {
		return true;
	} else if ( ( strcmp( node->data.strBuf, "false" ) == 0 ) || ( node->data.strBuf[ 0 ] == '0' && node->data.strBuf[ 1 ] == '\0' ) ) {
		return false;
	}

	NL_SetErrorMessage( NL_ERROR_INVALID_ARGUMENT, "Invalid data passed from var" );
	return false;
}

float NL_GetFloat( const NLNode *node ) {
	return strtof( node->data.strBuf, NULL );
}

PLVector2 NL_GetVec2( NLNode *node ) {
	if ( node->type != NODE_PROPERTY_ARRAY && node->childType != NODE_PROPERTY_FLOAT ) {
		NL_SetErrorMessage( NL_ERROR_INVALID_TYPE, "Attempted to get a \"vec2\" from an invalid property type!\n" );
		return pl_vecOrigin2;
	} else if ( NL_GetNumOfChildren( node ) != 2 ) {
		NL_SetErrorMessage( NL_ERROR_INVALID_ELEMENTS, "Invalid number of elements for \"vec2\" under node!\n" );
		return pl_vecOrigin2;
	}

	NLNode *xn = NL_GetFirstChild( node );
	assert( xn != NULL );
	NLNode *yn = NL_GetNextChild( xn );
	assert( yn != NULL );

	return PLVector2(
	        NL_GetFloat( xn ),
	        NL_GetFloat( yn ) );
}

PLVector3 NL_GetVec3( NLNode *node ) {
	if ( node->type != NODE_PROPERTY_ARRAY && node->childType != NODE_PROPERTY_FLOAT ) {
		NL_SetErrorMessage( NL_ERROR_INVALID_TYPE, "Attempted to get a \"vec3\" from an invalid property type!\n" );
		return pl_vecOrigin3;
	} else if ( NL_GetNumOfChildren( node ) != 3 ) {
		NL_SetErrorMessage( NL_ERROR_INVALID_ELEMENTS, "Invalid number of elements for \"vec3\" under node!\n" );
		return pl_vecOrigin3;
	}

	NLNode *xn = NL_GetFirstChild( node );
	assert( xn != NULL );
	NLNode *yn = NL_GetNextChild( xn );
	assert( yn != NULL );
	NLNode *zn = NL_GetNextChild( yn );
	assert( zn != NULL );

	return PLVector3(
	        NL_GetFloat( xn ),
	        NL_GetFloat( yn ),
	        NL_GetFloat( zn ) );
}

PLVector4 NL_GetVec4( NLNode *node ) {
	if ( node->type != NODE_PROPERTY_ARRAY && node->childType != NODE_PROPERTY_FLOAT ) {
		NL_SetErrorMessage( NL_ERROR_INVALID_TYPE, "Attempted to get a \"vec4\" from an invalid property type!\n" );
		return pl_vecOrigin4;
	} else if ( NL_GetNumOfChildren( node ) != 3 ) {
		NL_SetErrorMessage( NL_ERROR_INVALID_ELEMENTS, "Invalid number of elements for \"vec4\" under node!\n" );
		return pl_vecOrigin4;
	}

	NLNode *xn = NL_GetFirstChild( node );
	assert( xn != NULL );
	NLNode *yn = NL_GetNextChild( xn );
	assert( yn != NULL );
	NLNode *zn = NL_GetNextChild( yn );
	assert( zn != NULL );
	NLNode *wn = NL_GetNextChild( zn );

	return PLVector4(
	        NL_GetFloat( xn ),
	        NL_GetFloat( yn ),
	        NL_GetFloat( zn ),
	        NL_GetFloat( wn ) );
}

NLNode *xNL_PushBackNode( NLNode *parent, const char *name, NLPropertyType propertyType ) {
	/* arrays are special cases */
	if ( parent != NULL && parent->type == NODE_PROPERTY_ARRAY && propertyType != parent->childType ) {
		NL_SetErrorMessage( NL_ERROR_INVALID_TYPE, "Attempted to add " );
		return NULL;
	}

	NLNode *node = calloc( 1, sizeof( NLNode ) );
	if ( node == NULL ) {
		NL_SetErrorMessage( NL_ERROR_MEM_ALLOC, "failed to allocate node" );
		return NULL;
	}

	/* assign the node name, if provided */
	if ( name != NULL ) {
		node->name.strBuf = AllocVarString( name, &node->name.strBufLength );
	}

	node->type = propertyType;
	node->linkedList = plCreateLinkedList();

	/* if root is provided, this is treated as a child of that node */
	if ( parent != NULL ) {
		if ( parent->linkedList == NULL ) {
			parent->linkedList = plCreateLinkedList();
		}

		node->linkedListNode = plInsertLinkedListNode( parent->linkedList, node );
		node->parent = parent;
	}

	return node;
}

NLNode *NL_PushBackObj( NLNode *node, const char *name ) {
	return xNL_PushBackNode( node, name, NODE_PROPERTY_OBJECT );
}

NLNode *NL_PushBackString( NLNode *parent, const char *name, const char *var ) {
	NLNode *node = xNL_PushBackNode( parent, name, NODE_PROPERTY_STRING );
	if ( node != NULL ) {
		node->data.strBuf = AllocVarString( var, &node->data.strBufLength );
	}
	return node;
}

NLNode *NL_PushBackBool( NLNode *parent, const char *name, bool var ) {
	NLNode *node = xNL_PushBackNode( parent, name, NODE_PROPERTY_BOOLEAN );
	if ( node != NULL ) {
		node->data.strBuf = AllocVarString( var ? "true" : "false", &node->data.strBufLength );
	}
	return node;
}

NLNode *NL_PushBackInt( NLNode *parent, const char *name, int var ) {
	NLNode *node = xNL_PushBackNode( parent, name, NODE_PROPERTY_INTEGER );
	if ( node != NULL ) {
        char buf[ 32 ];
        snprintf( buf, sizeof( buf ), "%d", var );
        node->data.strBuf = AllocVarString( buf, &node->data.strBufLength );
	}
	return node;
}

NLNode *NL_PushBackFloat( NLNode *parent, const char *name, float var ) {
	NLNode *node = xNL_PushBackNode( parent, name, NODE_PROPERTY_FLOAT );
	if ( node != NULL ) {
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), "%f", var );
		node->data.strBuf = AllocVarString( buf, &node->data.strBufLength );
	}
	return node;
}

NLNode *NL_PushBackIntArray( NLNode *parent, const char *name, const int *array, unsigned int numElements ) {
    NLNode *node = xNL_PushBackNode( parent, name, NODE_PROPERTY_ARRAY );
    if ( node != NULL ) {
		node->childType = NODE_PROPERTY_INTEGER;
        for ( unsigned int i = 0; i < numElements; ++i ) {
            NL_PushBackInt( node, NULL, array[ i ] );
        }
    }
    return node;
}

NLNode *NL_PushBackFloatArray( NLNode *parent, const char *name, const float *array, unsigned int numElements ) {
	NLNode *node = xNL_PushBackNode( parent, name, NODE_PROPERTY_ARRAY );
	if ( node != NULL ) {
        node->childType = NODE_PROPERTY_FLOAT;
		for ( unsigned int i = 0; i < numElements; ++i ) {
			NL_PushBackFloat( node, NULL, array[ i ] );
		}
	}
	return node;
}

NLNode *NL_PushBackVec2( NLNode *parent, const char *name, const PLVector2 *var ) {
	return NL_PushBackFloatArray( parent, name, ( float* ) var, 2 );
}

NLNode *NL_PushBackVec3( NLNode *parent, const char *name, const PLVector3 *var ) {
	return NL_PushBackFloatArray( parent, name, ( float* ) var, 3 );
}

NLNode *NL_PushBackVec4( NLNode *parent, const char *name, const PLVector4 *var ) {
	return NL_PushBackFloatArray( parent, name, ( float* ) var, 4 );
}

void NL_DestroyNode( NLNode *node ) {
	pl_free( node->name.strBuf );
	pl_free( node->data.strBuf );

	/* if it's an object/array, we'll need to clean up all it's children */
	if ( node->type == NODE_PROPERTY_OBJECT || node->type == NODE_PROPERTY_ARRAY ) {
		NLNode *child = NL_GetFirstChild( node );
		while ( child != NULL ) {
			NLNode *nextChild = NL_GetNextChild( child );
			NL_DestroyNode( child );
			child = nextChild;
		}
	}

	plDestroyLinkedList( node->linkedList );
	if ( node->parent != NULL ) {
		plDestroyLinkedListNode( node->parent->linkedList, node->linkedListNode );
	}

	pl_free( node );
}

/******************************************/
/** Deserialisation **/

char *DeserializeStringVar( PLFile *file, unsigned int *length ) {
	*length = plReadInt32( file, false, NULL );
	if ( *length > 0 ) {
		char *buf = malloc( *length );
		plReadFile( file, buf, sizeof( char ), *length );
		return buf;
	}

	return NULL;
}

static NLNode *DeserializeBinaryNode( PLFile *file, NLNode *parent ) {
	/* binary implementation is pretty damn straight forward */
	NLNode *node = xNL_PushBackNode( parent, NULL, NODE_PROPERTY_INVALID );
	node->name.strBuf = DeserializeStringVar( file, &node->name.strBufLength );

	node->type = ( NLPropertyType ) plReadInt8( file, NULL );
	if ( node->type == NODE_PROPERTY_INVALID ) {
		NL_DestroyNode( node );
		Warning( "Encountered invalid node in \"%s\"!\n", plGetFilePath( file ) );
		return NULL;
	}

	switch ( node->type ) {
		case NODE_PROPERTY_ARRAY:
			/* only extra component we get here is the child type */
			node->childType = ( NLPropertyType ) plReadInt8( file, NULL );
		case NODE_PROPERTY_OBJECT: {
			unsigned int numChildren = plReadInt32( file, false, NULL );
			for ( unsigned int i = 0; i < numChildren; ++i ) {
				DeserializeBinaryNode( file, node );
			}
			break;
		}
		case NODE_PROPERTY_STRING: {
			node->data.strBuf = DeserializeStringVar( file, &node->data.strBufLength );
			break;
		}
		case NODE_PROPERTY_BOOLEAN: {
			bool v = plReadInt8( file, NULL );
			node->data.strBuf = AllocVarString( v ? "true" : "false", &node->data.strBufLength );
			break;
		}
		case NODE_PROPERTY_FLOAT: {
			float v = ( float ) plReadInt32( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%f", v );
			node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
			break;
		}
		case NODE_PROPERTY_INTEGER: {
			int v = plReadInt32( file, false, NULL );
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%d", v );
			node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
			break;
		}
	}

	return node;
}

static NLFileType ParseNodeFileType( PLFile *file ) {
	char token[ 32 ];
	if ( plReadString( file, token, sizeof( token ) ) == NULL ) {
		NL_SetErrorMessage( NL_ERROR_IO_READ, "Failed to read in file type: %s", plGetError() );
		return NL_FILE_INVALID;
	}

	if ( strncmp( token, NL_BINARY_HEADER, strlen( NL_BINARY_HEADER ) ) == 0 ) {
		return NL_FILE_BINARY;
	} else if ( strncmp( token, NL_ASCII_HEADER, strlen( NL_ASCII_HEADER ) ) == 0 ) {
		return NL_FILE_ASCII;
	}

	NL_SetErrorMessage( NL_ERROR_INVALID_ARGUMENT, "Unknown file type \"%s\"", token );
	return NL_FILE_INVALID;
}

NLNode *NL_LoadFile( const char *path, const char *objectType ) {
	NL_ClearErrorMessage();

	PLFile *file = plOpenFile( path, true );
	if ( file == NULL ) {
		Warning( "Failed to open \"%s\": %s\n", path, plGetError() );
		return NULL;
	}

	NLNode *root = NULL;

	NLFileType fileType = ParseNodeFileType( file );
	if ( fileType == NL_FILE_INVALID ) {
		Warning( "Invalid node file type: %d\n", fileType );
	} else if ( fileType == NL_FILE_ASCII ) {
		/* first need to run the pre-processor on it */
		size_t length = plGetFileSize( file );
        const char *data = ( const char* ) plGetFileData( file ) + strlen( NL_ASCII_HEADER );
		char *buf = pl_malloc( length );
		memcpy( buf, data, length );

		Message( "Preprocessing \"%s\"\n", path );
		buf = xNL_PreProcessScript( buf, &length, true );
		Message( "Done\n%s", buf );

		root = NL_ParseBuffer( buf, length );
	} else {
		/* assumes binary format */
		root = DeserializeBinaryNode( file, NULL );
	}

	plCloseFile( file );

	if ( root != NULL && objectType != NULL ) {
		const char *rootName = NL_GetName( root );
		if ( strcmp( rootName, objectType ) != 0 ) {
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

void SerializeStringVar( const NLVarString *string, NLFileType fileType, FILE *file ) {
	if ( fileType == NL_FILE_BINARY ) {
		fwrite( &string->strBufLength, sizeof( uint32_t ), 1, file );
		/* slightly paranoid here, because strBuf is probably null if length is 0
		 * which is totally valid, but eh */
		if ( string->strBufLength > 0 ) {
			fwrite( string->strBuf, sizeof( char ), string->strBufLength, file );
		}
		return;
	}

	/* allow nameless nodes, used for arrays */
	if ( string->strBufLength == 0 ) {
		return;
	}

	bool encloseString = false;
	const char *c = string->strBuf;
	if ( *c == '\0' ) {
		/* enclose an empty string!!! */
		encloseString = true;
	} else {
		/* otherwise, check if there are any spaces */
		while ( *c != '\0' ) {
			if ( *c == ' ' ) {
				encloseString = true;
				break;
			}

			c++;
		}
	}

	if ( encloseString ) {
		fprintf( file, "\"%s\"", string->strBuf );
	} else {
		fprintf( file, "%s", string->strBuf );
	}
}

static void SerializeNodeTree( FILE *file, NLNode *root, NLFileType fileType );
static void SerializeNode( FILE *file, NLNode *node, NLFileType fileType ) {
	if ( fileType == NL_FILE_ASCII ) {
		/* write out the line identifying this node */
		fprintf( file, "%s ", StringForPropertyType( node->type ) );
		SerializeStringVar( &node->name, fileType, file );
		fprintf( file, " " );
		/* if this node has children, serialize all those */
		if ( node->type == NODE_PROPERTY_OBJECT || node->type == NODE_PROPERTY_ARRAY ) {
			fprintf( file, "{\n" );
			SerializeNodeTree( file, node, fileType );
			fprintf( file, "}\n\n" );
		} else {
			SerializeStringVar( &node->data, fileType, file );
			fprintf( file, "\n" );
		}
	} else {
		SerializeStringVar( &node->name, fileType, file );
		fwrite( &node->type, sizeof( int8_t ), 1, file );
		switch ( node->type ) {
			case NODE_PROPERTY_FLOAT: {
				float v = NL_GetFloat( node );
				fwrite( &v, sizeof( float ), 1, file );
				break;
			}
			case NODE_PROPERTY_INTEGER: {
				int v = ( int ) NL_GetFloat( node );
				fwrite( &v, sizeof( uint32_t ), 1, file );
				break;
			}
			case NODE_PROPERTY_STRING: {
				SerializeStringVar( &node->data, fileType, file );
				break;
			}
			case NODE_PROPERTY_BOOLEAN: {
				bool v = NL_GetBoolean( node );
				fwrite( &v, sizeof( uint8_t ), 1, file );
				break;
			}
			case NODE_PROPERTY_ARRAY:
				/* only extra component here is the child type */
				fwrite( &node->childType, sizeof( uint8_t ), 1, file );
			case NODE_PROPERTY_OBJECT: {
				uint32_t i = plGetNumLinkedListNodes( node->linkedList );
				fwrite( &i, sizeof( uint32_t ), 1, file );
				SerializeNodeTree( file, node, fileType );
				break;
			}
		}
	}
}

static void SerializeNodeTree( FILE *file, NLNode *root, NLFileType fileType ) {
	PLLinkedListNode *i = plGetFirstNode( root->linkedList );
	while ( i != NULL ) {
		NLNode *node = plGetLinkedListNodeUserData( i );
		SerializeNode( file, node, fileType );
		i = plGetNextLinkedListNode( i );
	}
}

/**
 * Serialize the given node set.
 */
void NL_WriteFile( const char *path, NLNode *root, NLFileType fileType ) {
	FILE *file = fopen( path, "wb" );
	if ( file == NULL ) {
		NL_SetErrorMessage( NL_ERROR_IO_WRITE, "Failed to open path \"%s\"", path );
		return;
	}

	if ( fileType == NL_FILE_BINARY ) {
		fprintf( file, NL_BINARY_HEADER "\n" );
	} else {
		fprintf( file, NL_ASCII_HEADER "\n" );
	}

	SerializeNode( file, root, fileType );

	fclose( file );
}

/******************************************/
/** API Testing **/

void NL_PrintNodeTree( NLNode *node, int index ) {
	for ( unsigned int i = 0; i < index; ++i ) printf( "\t" );
	if ( node->type == NODE_PROPERTY_OBJECT || node->type == NODE_PROPERTY_ARRAY ) {
		index++;

		if ( node->type == NODE_PROPERTY_OBJECT ) {
			Message( "%s %s\n", node->name.strBuf, StringForPropertyType( node->type ) );
		} else {
			Message( "%s %s\n", StringForPropertyType( node->type ), StringForPropertyType( node->childType ) );
		}

		NLNode *child = NL_GetFirstChild( node );
		while ( child != NULL ) {
			NL_PrintNodeTree( child, index );
			child = NL_GetNextChild( child );
		}
	} else {
		NLNode *parent = NL_GetParent( node );
		if ( parent != NULL && parent->type == NODE_PROPERTY_ARRAY ) {
            Message( "%s %s\n", StringForPropertyType( node->type ), node->data.strBuf );
		} else {
			Message( "%s %s %s\n", StringForPropertyType( node->type ), node->name.strBuf, node->data.strBuf );
		}
	}
}
