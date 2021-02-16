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

static char *nlErrorMsg = NULL;
static void NL_ClearErrorMessage( void ) {
	free( nlErrorMsg );
	nlErrorMsg = NULL;
}
static void NL_SetErrorMessage( const char *msg, ... ) {
	va_list args;
	va_start( args, msg );

	int length = pl_vscprintf( msg, args ) + 1;
	if ( length <= 0 ) {
		return;
	}

	nlErrorMsg = calloc( 1, length );
	if ( nlErrorMsg == NULL ) {
		printf( "Failed to allocate error message buffer: %d bytes!\n", length );
		return;
	}

	vsnprintf( nlErrorMsg, length, msg, args );
	printf( "error: %s\n", nlErrorMsg );

	va_end( args );
}

const char *NL_GetError( void ) { return nlErrorMsg; }

static char *AllocVarString( const char *string, unsigned int *lengthOut ) {
	*lengthOut = strlen( string ) + 1;
	char *buf = calloc( 1, *lengthOut );
	strcpy( buf, string );
	return buf;
}

unsigned int NL_GetNumOfChildren( const NLNode *root ) {
	return plGetNumLinkedListNodes( root->linkedList );
}

NLNode *NL_GetFirstChild( NLNode *root ) {
	PLLinkedListNode *n = plGetFirstNode( root->linkedList );
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

NLNode *NL_GetChildByName( NLNode *root, const char *name ) {
	NLNode *child = NL_GetFirstChild( root );
	while ( child != NULL ) {
		if ( strcmp( name, child->name.strBuf ) == 0 ) {
			return child;
		}

		child = NL_GetNextChild( root );
	}

	return NULL;
}

NLNode *NL_GetParent( NLNode *node ) {
	return node->parent;
}

const char *Node_GetName( const NLNode *node ) {
	return node->name.strBuf;
}

NLPropertyType Node_GetType( const NLNode *node ) {
	return node->type;
}

const char *Node_GetString( const NLNode *node ) {
	return node->data.strBuf;
}

bool Node_GetBoolean( const NLNode *node ) {
	if ( ( strcmp( node->data.strBuf, "true" ) == 0 ) || ( node->data.strBuf[ 0 ] == '1' && node->data.strBuf[ 1 ] == '\0' ) ) {
		return true;
	} else if ( ( strcmp( node->data.strBuf, "false" ) == 0 ) || ( node->data.strBuf[ 0 ] == '0' && node->data.strBuf[ 1 ] == '\0' ) ) {
		return false;
	}

	NL_SetErrorMessage( "invalid data passed from var" );
	return false;
}

float Node_GetFloat( const NLNode *node ) {
	return strtof( node->data.strBuf, NULL );
}

PLVector2 Node_GetVec2( const NLNode *node ) {
    PLVector2 v;
    char *p = node->data.strBuf;
    v.x = strtof( p, &p );
    v.y = strtof( p, &p );
    return v;
}

PLVector3 Node_GetVec3( const NLNode *node ) {
	PLVector3 v;
	char *p = node->data.strBuf;
	v.x = strtof( p, &p );
	v.y = strtof( p, &p );
	v.z = strtof( p, &p );
	return v;
}

PLVector4 Node_GetVec4( const NLNode *node ) {
    PLVector4 v;
    char *p = node->data.strBuf;
    v.x = strtof( p, &p );
    v.y = strtof( p, &p );
    v.z = strtof( p, &p );
	v.w = strtof( p, &p );
    return v;
}

static NLNode *InsertNode( NLNode *root, const char *name, NLPropertyType propertyType ) {
	NLNode *node = calloc( 1, sizeof( NLNode ) );
	if ( node == NULL ) {
		NL_SetErrorMessage( "failed to allocate node" );
		return NULL;
	}

	/* assign the node name, if provided */
	if ( name != NULL ) {
		node->name.strBuf = AllocVarString( name, &node->name.strBufLength );
	}

	node->type = propertyType;
	node->linkedList = plCreateLinkedList();

	/* if root is provided, this is treated as a child of that node */
	if ( root != NULL ) {
		if ( root->linkedList == NULL ) {
			root->linkedList = plCreateLinkedList();
		}

		node->linkedListNode = plInsertLinkedListNode( root->linkedList, node );
		node->parent = root;
	}

	return node;
}

NLNode *NL_AddObject( NLNode *root, const char *name ) {
	return InsertNode( root, name, NODE_PROPERTY_OBJECT );
}

NLNode *NL_AddStringVar( NLNode *root, const char *name, const char *var ) {
	NLNode *node = InsertNode( root, name, NODE_PROPERTY_STRING );
	node->data.strBuf = AllocVarString( var, &node->data.strBufLength );
	return node;
}

NLNode *NL_AddBooleanVar( NLNode *root, const char *name, bool var ) {
	NLNode *node = InsertNode( root, name, NODE_PROPERTY_BOOLEAN );
	node->data.strBuf = AllocVarString( var ? "true" : "false", &node->data.strBufLength );
	return node;
}

NLNode *NL_AddNumericVar( NLNode *root, const char *name, float var ) {
	NLNode *node = InsertNode( root, name, NODE_PROPERTY_FLOAT );
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), "%f", var );
	node->data.strBuf = AllocVarString( buf, &node->data.strBufLength );
	return node;
}

void NL_DestroyNode( NLNode *node ) {
    pl_free( node->name.strBuf );
    pl_free( node->data.strBuf );

	/* if it's an object, we'll need to clean up all it's children */
	if ( node->type == NODE_PROPERTY_OBJECT ) {
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
	char *buf = malloc( *length );
	plReadFile( file, buf, sizeof( char ), *length );
	return buf;
}

static NLNode *DeserializeBinaryNode( PLFile *file, NLNode *parent ) {
	/* binary implementation is pretty damn straight forward */
	NLNode *node = InsertNode( parent, NULL, NODE_PROPERTY_INVALID );
	node->name.strBuf = DeserializeStringVar( file, &node->name.strBufLength );

	node->type = ( NLPropertyType ) plReadInt8( file, NULL );
	if ( node->type == NODE_PROPERTY_INVALID ) {
		NL_DestroyNode( node );
		Warning( "Encountered invalid node in \"%s\"!\n", plGetFilePath( file ) );
		return NULL;
	}

	switch( node->type ) {
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
		case NODE_PROPERTY_VEC2: {
			float x = ( float ) plReadInt32( file, false, NULL );
			float y = ( float ) plReadInt32( file, false, NULL );
			char str[ 64 ];
			snprintf( str, sizeof( str ), "%f,%f", x, y );
			node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
			break;
		}
		case NODE_PROPERTY_VEC3: {
            float x = ( float ) plReadInt32( file, false, NULL );
            float y = ( float ) plReadInt32( file, false, NULL );
			float z = ( float ) plReadInt32( file, false, NULL );
            char str[ 128 ];
            snprintf( str, sizeof( str ), "%f,%f,%f", x, y, z );
            node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
            break;
		}
		case NODE_PROPERTY_VEC4: {
            float x = ( float ) plReadInt32( file, false, NULL );
            float y = ( float ) plReadInt32( file, false, NULL );
            float z = ( float ) plReadInt32( file, false, NULL );
			float w = ( float ) plReadInt32( file, false, NULL );
            char str[ 256 ];
            snprintf( str, sizeof( str ), "%f,%f,%f,%f", x, y, z, w );
            node->data.strBuf = AllocVarString( str, &node->data.strBufLength );
            break;
		}
	}

	return node;
}

static NLFileType ParseNodeFileType( PLFile *file ) {
	char token[ 32 ];
	if ( plReadString( file, token, sizeof( token ) ) == NULL ) {
		NL_SetErrorMessage( "failed to read in file type: %s", plGetError() );
		return NL_FILE_INVALID;
	}

	if ( strncmp( token, NL_BINARY_HEADER, strlen( NL_BINARY_HEADER ) ) == 0 ) {
		return NL_FILE_BINARY;
	} else if ( strncmp( token, NL_ASCII_HEADER, strlen( NL_ASCII_HEADER ) ) == 0 ) {
		return NL_FILE_ASCII;
	}

	NL_SetErrorMessage( "unknown file type \"%s\"", token );
	return NL_FILE_INVALID;
}

NLNode *NL_LoadFile( const char *path ) {
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
		char *buf = pl_malloc( length );
		memcpy( buf, plGetFileData( file ), length );

		Message( "Preprocessing \"%s\"\n", path );
		buf = Node_PreProcessScript( buf, &length, true );
		Message( "Done\n" );
	} else {
		/* assumes binary format */
		root = DeserializeBinaryNode( file, NULL );
	}

	plCloseFile( file );

	return root;
}

/******************************************/
/** Serialisation **/

void SerializeStringVar( const NLVarString *string, NLFileType fileType, FILE *file ) {
	if ( fileType == NL_FILE_BINARY ) {
		fwrite( &string->strBufLength, sizeof( uint32_t ), 1, file );
		fwrite( string->strBuf, sizeof( char ), string->strBufLength, file );
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
		case NODE_PROPERTY_VEC2:
			return "vec2";
		case NODE_PROPERTY_VEC3:
			return "vec3";
		case NODE_PROPERTY_VEC4:
			return "vec4";
		default:
			return "invalid";
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
		if ( node->type == NODE_PROPERTY_OBJECT ) {
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
				float v = Node_GetFloat( node );
				fwrite( &v, sizeof( float ), 1, file );
				break;
			}
			case NODE_PROPERTY_INTEGER: {
				int v = ( int ) Node_GetFloat( node );
				fwrite( &v, sizeof( uint32_t ), 1, file );
				break;
			}
			case NODE_PROPERTY_STRING: {
				SerializeStringVar( &node->data, fileType, file );
				break;
			}
			case NODE_PROPERTY_BOOLEAN: {
				bool v = Node_GetBoolean( node );
				fwrite( &v, sizeof( uint8_t ), 1, file );
				break;
			}
			case NODE_PROPERTY_OBJECT: {
				uint32_t i = plGetNumLinkedListNodes( node->linkedList );
				fwrite( &i, sizeof( uint32_t ), 1, file );
				SerializeNodeTree( file, node, fileType );
				break;
			}
			case NODE_PROPERTY_VEC2: {
				PLVector2 v = Node_GetVec2( node );
				fwrite( &v, sizeof( PLVector2 ), 1, file );
				break;
			}
			case NODE_PROPERTY_VEC3: {
				PLVector3 v = Node_GetVec3( node );
				fwrite( &v, sizeof( PLVector3 ), 1, file );
				break;
			}
			case NODE_PROPERTY_VEC4: {
				PLVector4 v = Node_GetVec4( node );
				fwrite( &v, sizeof( PLVector4 ), 1, file );
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
		NL_SetErrorMessage( "failed to open path \"%s\"", path );
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
	if ( node->type == NODE_PROPERTY_OBJECT ) {
		index++;

		printf( "%s %s\n", node->name.strBuf, StringForPropertyType( node->type ) );

		NLNode *child = NL_GetFirstChild( node );
		while ( child != NULL ) {
			NL_PrintNodeTree( child, index );
			child = NL_GetNextChild( child );
		}
	} else {
		printf( "%s %s %s\n", StringForPropertyType( node->type ), node->name.strBuf, node->data.strBuf );
	}
}
