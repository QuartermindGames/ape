/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

#pragma once

enum SGNodeType {
	SG_NODE_TYPE_WORLD,    /* world node; each one has a list of sectors */
	SG_NODE_TYPE_SECTOR,   /* sectors exist under each world */
	SG_NODE_TYPE_GEOMETRY, /* raw geometry */

	SG_NODE_TYPE_ACTOR,     /* actors exist under the sectors */
	SG_NODE_TYPE_LIGHT,     /* and these, typically, also exist under the sectors */
};

typedef struct SGNode SGNode;

void SG_Initialize( void );

const PLMatrix4 *SG_GetNodeTransform( const SGNode *node );
unsigned int SG_GetNodeType( const SGNode *node );
void *SG_GetNodeData( SGNode *node );

SGNode *SG_AddChildNode( SGNode *parent, unsigned int dataType, void *data );
void SG_RemoveChildNode( SGNode *parent, SGNode *node );
void SG_RemoveAllChildren( SGNode *parent );
