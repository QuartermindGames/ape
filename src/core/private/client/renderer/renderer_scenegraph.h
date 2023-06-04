// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

typedef struct NdBranch NdBranch;

enum SGNodeType
{
	SG_NODE_TYPE_WORLD,	   /* world node; each one has a list of sectors */
	SG_NODE_TYPE_SECTOR,   /* sectors exist under each world */
	SG_NODE_TYPE_GEOMETRY, /* raw geometry */

	SG_NODE_TYPE_ACTOR, /* actors exist under the sectors */
	SG_NODE_TYPE_LIGHT, /* and these, typically, also exist under the sectors */
	SG_NODE_TYPE_PARTICLE_EMITTER,
	SG_NODE_TYPE_PARTICLE,
};

/**
 * Standard transform structure.
 */
typedef struct SGTransform
{
	PLVector3	 translation;
	PLVector3	 scale;
	PLQuaternion rotation;
} SGTransform;
#define SG_InitializeTransform( TRANSFORM ) memset( ( TRANSFORM ), 0, sizeof( SGTransform ) )

typedef struct SGNode SGNode;

/* ======================================================================
 * Serialisation/Deserialisation
 * ====================================================================*/

SGTransform *SG_DS_Transform( NdBranch *root, const char *childName, SGTransform *out );

/* ======================================================================
 * ====================================================================*/

void YnCore_InitializeSceneGraph( void );

const SGTransform *SG_GetNodeTransform( const SGNode *node );
unsigned int	   SG_GetNodeType( const SGNode *node );
void *			   SG_GetNodeData( SGNode *node );

SGNode *SG_AddChildNode( SGNode *parent, unsigned int dataType, void *data );
void	SG_RemoveChildNode( SGNode *parent, SGNode *node );
void	SG_RemoveAllChildren( SGNode *parent );
