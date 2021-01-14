/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>

#include "yin.h"
#include "scenegraph.h"

typedef struct SGNode {
	PLMatrix4 transform;
	unsigned int dataType;
	void *data;
	PLLinkedListNode *node;
	PLLinkedList *children;
} SGNode;

static PLLinkedList *sceneGraph = NULL;
static PLLinkedListNode *rootNode = NULL;

void SG_Initialize( void ) {
	sceneGraph = plCreateLinkedList();
	if ( sceneGraph == NULL ) {
		PrintError( "Failed to create scene graph!\n" );
	}
}

void SG_Shutdown( void ) {
	/* destruction of node data is left to the handlers */
	plDestroyLinkedListNodes( sceneGraph );
	plDestroyLinkedList( sceneGraph );
}

const PLMatrix4 *SG_GetNodeTransform( const SGNode *node ) {
	return &node->transform;
}

unsigned int SG_GetNodeType( const SGNode *node ) {
	return node->dataType;
}

void *SG_GetNodeData( SGNode *node ) {
	return node->data;
}

/*
 * Typically a world instance.
 */
SGNode *SG_AddHeadNode( unsigned int dataType, void *data ) {
	SGNode *head = Sys_calloc( 1, sizeof( SGNode ) );
	head->data = data;
	head->dataType = dataType;
	head->node = plInsertLinkedListNode( sceneGraph, head );

	return head;
}

SGNode *SG_AddChildNode( SGNode *parent, unsigned int dataType, void *data ) {
	SGNode *child = Sys_calloc( 1, sizeof( SGNode ) );
	child->data = data;
	child->dataType = dataType;

	if ( parent->children == NULL ) {
		parent->children = plCreateLinkedList();
	}
	child->node = plInsertLinkedListNode( parent->children, child );

	return child;
}

void SG_RemoveChildNode( SGNode *parent, SGNode *node ) {
	plDestroyLinkedListNode( parent->children, node->node );

	free( node );
}

void SG_RemoveAllChildren( SGNode *parent ) {
	//PLLinkedListNode *node =

	plDestroyLinkedListNodes( parent->children );
}

void SG_SimpleTraversal( SGNode *start, PLCamera *camera ) {

}
