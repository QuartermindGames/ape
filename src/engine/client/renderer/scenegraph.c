/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "scenegraph.h"
#include "common/node.h"

typedef struct SGNode
{
	SGTransform       transform;
	unsigned int      dataType;
	void *            data;
	PLLinkedListNode *node;
	PLLinkedList *    children;
} SGNode;

static PLLinkedList *    sceneGraph = NULL;
static PLLinkedListNode *rootNode   = NULL;

SGTransform *SG_DS_Transform( NLNode *root, const char *childName, SGTransform *out )
{
	SG_InitializeTransform( out );

	NLNode *child = NL_GetChildByName( root, childName );
	if ( child == NULL )
		return NULL;

	NLNode *n;
	if ( ( n = NL_GetChildByName( child, "rotation" ) ) != NULL )
		NL_DS_DeserializeQuaternion( n, &out->rotation );
	if ( ( n = NL_GetChildByName( child, "scale" ) ) != NULL )
		NL_DS_DeserializeVector3( n, &out->scale );
	if ( ( n = NL_GetChildByName( child, "translation" ) ) != NULL )
		NL_DS_DeserializeVector3( n, &out->translation );

	return out;
}

void SG_Initialize( void )
{
	sceneGraph = PlCreateLinkedList();
	if ( sceneGraph == NULL )
		PrintError( "Failed to create scene graph!\n" );
}

void SG_Shutdown( void )
{
	/* destruction of node data is left to the handlers */
	PlDestroyLinkedListNodes( sceneGraph );
	PlDestroyLinkedList( sceneGraph );
}

const SGTransform *SG_GetNodeTransform( const SGNode *node )
{
	return &node->transform;
}

unsigned int SG_GetNodeType( const SGNode *node )
{
	return node->dataType;
}

void *SG_GetNodeData( SGNode *node )
{
	return node->data;
}

/*
 * Typically a world instance.
 */
SGNode *SG_AddHeadNode( unsigned int dataType, void *data )
{
	SGNode *head   = globalSystem.MAlloc( sizeof( SGNode ), true );
	head->data     = data;
	head->dataType = dataType;
	head->node     = PlInsertLinkedListNode( sceneGraph, head );

	return head;
}

SGNode *SG_AddChildNode( SGNode *parent, unsigned int dataType, void *data )
{
	SGNode *child   = globalSystem.MAlloc( sizeof( SGNode ), true );
	child->data     = data;
	child->dataType = dataType;

	if ( parent->children == NULL )
		parent->children = PlCreateLinkedList();

	child->node = PlInsertLinkedListNode( parent->children, child );

	return child;
}

void SG_RemoveChildNode( SGNode *parent, SGNode *node )
{
	PlDestroyLinkedListNode( parent->children, node->node );

	globalSystem.Free( node );
}

void SG_RemoveAllChildren( SGNode *parent )
{
	//PLLinkedListNode *node =

	PlDestroyLinkedListNodes( parent->children );
}

void SG_SimpleTraversal( SGNode *start, PLGCamera *camera )
{
}
