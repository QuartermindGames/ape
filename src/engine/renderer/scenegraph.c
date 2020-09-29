/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/pl_llist.h>

#include "yin.h"
#include "actor.h"
#include "light.h"

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
}

void SG_Shutdown( void ) {
	/* destruction of node data is left to the handlers */
	plDestroyLinkedListNodes( sceneGraph );
	plDestroyLinkedList( sceneGraph );
}

SGNode *SG_AddChildNode( SGNode *parent, unsigned int dataType, void *data ) {
	SGNode *child = Sys_malloc( sizeof( SGNode ) );
	child->data = data;
	child->dataType = dataType;
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
