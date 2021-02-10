/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>
#include <PL/pl_parse.h>
#include <PL/platform_filesystem.h>

#include "common/nodelst.h"

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

typedef struct NLVarNumeric {
	float numericVar[ 32 ];
	unsigned int arrayLength;
} NLVarNumeric;

typedef struct NLVarString {
	char *strBuf;
	unsigned int strBufLength;
} NLVarString;
static char *AllocVarString( const char *string, unsigned int *lengthOut ) {
	*lengthOut = strlen( string );
	char *buf = calloc( 1, *lengthOut );
	strcpy( buf, string );
	return buf;
}

typedef struct NLNode {
	NLVarString name;
	NLPropertyType type;
	NLVarString data;

	PLLinkedListNode *node;
	PLLinkedList *list;
} NLNode;

unsigned int NL_GetNumOfChildren( const NLNode *root ) {
	return plGetNumLinkedListNodes( root->list );
}

NLNode *NL_GetFirstChild( NLNode *root ) {
	PLLinkedListNode *n = plGetFirstNode( root->list );
	if ( n == NULL ) {
		return NULL;
	}

	return plGetLinkedListNodeUserData( n );
}

NLNode *NL_GetNextChild( NLNode *node ) {
	PLLinkedListNode *n = plGetNextLinkedListNode( node->node );
	if ( n == NULL ) {
		return NULL;
	}

	return plGetLinkedListNodeUserData( n );
}

const char *NL_GetNodeName( const NLNode *node ) {
	return node->name.strBuf;
}

const char *NL_GetNodeStringData( const NLNode *node ) {
	return node->data.strBuf;
}

bool NL_GetNodeBooleanData( const NLNode *node ) {
	if ( ( strcmp( node->data.strBuf, "true" ) == 0 ) || ( node->data.strBuf[ 0 ] == '1' && node->data.strBuf[ 1 ] == '\0' ) ) {
		return true;
	} else if ( ( strcmp( node->data.strBuf, "false" ) == 0 ) || ( node->data.strBuf[ 0 ] == '0' && node->data.strBuf[ 1 ] == '\0' ) ) {
		return false;
	}

	NL_SetErrorMessage( "invalid data passed from var" );
	return false;
}

float NL_GetNodeNumericData( const NLNode *node ) {
	return strtof( node->data.strBuf, NULL );
}

static NLNode *InsertNode( NLNode *root, const char *name, NLPropertyType propertyType ) {
	NLNode *node = calloc( 1, sizeof( NLNode ) );
	if ( node == NULL ) {
		NL_SetErrorMessage( "failed to allocate node" );
		return NULL;
	}

	/* assign the node name */
	node->name.strBuf = AllocVarString( name, &node->name.strBufLength );
	node->type = propertyType;
    node->list = plCreateLinkedList();

	/* if root is provided, this is treated as a child of that node */
	if ( root != NULL ) {
		if ( root->list == NULL ) {
			root->list = plCreateLinkedList();
		}

		node->node = plInsertLinkedListNode( root->list, node );
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

const NLVarNumeric *NL_GetNumericProperty( const NLNode *root, const char *value ) {
}

const NLNode *NL_GetNodeByName( NLNode *root, const char *path, unsigned int index ) {
}

NLPropertyType NL_GetType( const NLNode *root ) {
	return root->type;
}

/******************************************/
/** Deserialisation **/

static NLNode *DeserializeNode( PLFile *file, NLFileType fileType ) {

}

static NLFileType ParseNodeFileType( PLFile *file ) {
	char token[ 32 ];
	if ( plReadString( file, token, sizeof( token ) ) == NULL ) {
		NL_SetErrorMessage( "failed to read in file type: %s", plGetError() );
		return NL_FILE_INVALID;
	}

	if ( strncmp( token, NL_BINARY_HEADER, sizeof( NL_BINARY_HEADER ) ) == 0 ) {
		return NL_FILE_BINARY;
	} else if ( strncmp( token, NL_ASCII_HEADER, sizeof( NL_ASCII_HEADER ) ) == 0 ) {
		return NL_FILE_ASCII;
	}

	NL_SetErrorMessage( "unknown file type \"%s\"", token );
	return NL_FILE_INVALID;
}

NLNode *NL_LoadFile( const char *path ) {
	NL_ClearErrorMessage();

	PLFile *file = plOpenFile( path, true );
	if ( file == NULL ) {
		NL_SetErrorMessage( "failed to open \"%s\": %s", path, plGetError() );
		return NULL;
	}

	NLFileType fileType = ParseNodeFileType( file );
	if ( fileType == NL_FILE_INVALID ) {
		return NULL;
	}

	NLNode *root = NULL;
	if ( fileType == NL_FILE_ASCII ) {
		const char *buf = ( const char * ) plGetFileData( file );
		size_t len = plGetFileSize( file );
	} else {
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
	while( *c != '\0' ) {
		if ( *c == ' ' ) {
            encloseString = true;
			break;
		}

		c++;
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
            fprintf( file, "}\n" );
        } else {
            SerializeStringVar( &node->data, fileType, file );
            fprintf( file, "\n" );
        }
		fprintf( file, "\n" );
    } else {
        SerializeStringVar( &node->name, fileType, file );
        fwrite( &node->type, sizeof( int8_t ), 1, file );
        switch( node->type ) {
            case NODE_PROPERTY_FLOAT: {
                float v = NL_GetNodeNumericData( node );
                fwrite( &v, sizeof( float ), 1, file );
                break;
            }
            case NODE_PROPERTY_INTEGER: {
                break;
            }
			default:
            case NODE_PROPERTY_STRING: {
                SerializeStringVar( &node->data, fileType, file );
                break;
            }
            case NODE_PROPERTY_BOOLEAN: {
                bool v = NL_GetNodeBooleanData( node );
                fwrite( &v, sizeof( uint8_t ), 1, file );
                break;
            }
            case NODE_PROPERTY_OBJECT: {
                uint32_t i = plGetNumLinkedListNodes( node->list );
                fwrite( &i, sizeof( uint32_t ), 1, file );
                SerializeNodeTree( file, node, fileType );
                break;
            }
        }
    }
}

static void SerializeNodeTree( FILE *file, NLNode *root, NLFileType fileType ) {
	PLLinkedListNode *i = plGetFirstNode( root->list );
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

bool TestNodeLists( void ) {
	NLNode *rootNode = NL_LoadFile( "scripts/tests/config.node" );
	if ( rootNode == NULL ) {
		printf( "Failed on LoadFile: %s\n", NL_GetError() );
		return false;
	}

	return true;
}
