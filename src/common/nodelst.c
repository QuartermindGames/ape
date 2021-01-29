/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>

#include "common/nodelst.h"

static char *nlErrorMsg = NULL;
static void NL_ClearErrorMessage( void ) { free( nlErrorMsg ); nlErrorMsg = NULL; }
static void NL_SetErrorMessage( const char *msg, ... ) {
    NL_ClearErrorMessage();

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

typedef struct NLNode {
	NLVarString name;
	NLPropertyType type;
	union {
		bool booleanVar;
		NLVarString stringVar;
        NLVarNumeric numVar;
	};

	PLLinkedListNode *node;
	PLLinkedList *list;
} NLNode;

NLNode *NL_LoadFile( const char *path ) {

}

const NLVarNumeric *NL_GetNumericProperty( const NLNode *root, const char *value ) {

}

const NLNode *NL_GetNodeByName( NLNode *root, const char *path, unsigned int index ) {

}

NLPropertyType NL_GetType( const NLNode *root ) {
	return root->type;
}

unsigned int NL_GetNumOfChildren( const NLNode *root ) {
	return ( root->list != NULL ) ? plGetNumLinkedListNodes( root->list ) : 0;
}

void TestNodeLists( void ) {
	NLNode *rootNode = NL_LoadFile( "scripts/tests/config.node" );
}
