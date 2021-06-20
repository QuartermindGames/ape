/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

enum SGNodeType
{
	SG_NODE_TYPE_WORLD,    /* world node; each one has a list of sectors */
	SG_NODE_TYPE_SECTOR,   /* sectors exist under each world */
	SG_NODE_TYPE_GEOMETRY, /* raw geometry */

	SG_NODE_TYPE_ACTOR, /* actors exist under the sectors */
	SG_NODE_TYPE_LIGHT, /* and these, typically, also exist under the sectors */
};

/**
 * Standard transform structure.
 */
typedef struct SGTransform
{
	PLVector3    translation;
	PLVector3    scale;
	PLQuaternion rotation;
} SGTransform;

typedef struct SGNode SGNode;

void SG_Initialize( void );

const SGTransform *SG_GetNodeTransform( const SGNode *node );
unsigned int       SG_GetNodeType( const SGNode *node );
void *             SG_GetNodeData( SGNode *node );

SGNode *SG_AddChildNode( SGNode *parent, unsigned int dataType, void *data );
void    SG_RemoveChildNode( SGNode *parent, SGNode *node );
void    SG_RemoveAllChildren( SGNode *parent );
