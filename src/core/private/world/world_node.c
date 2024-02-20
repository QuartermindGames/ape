// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Nodes form the foundation of everything in a scene.
// Author:  Mark E. Sowden

#include "world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeWorldNode *ape_world_node_create( ApeWorldNode *parent, const char *name, ApeWorldNodeType type, void *data )
{
	ApeWorldNode *node = PL_NEW( ApeWorldNode );
	snprintf( node->name, sizeof( node->name ), "%s", name );

	node->children = PlCreateLinkedList();
	node->transform = PlMatrix4Identity();

	node->parent = parent;
	if ( node->parent != NULL )
	{
		node->parentListNode = PlInsertLinkedListNode( node->parent->children, node );
	}

	ape_world_node_attach_data( node, type, data );

	return node;
}

void ape_world_node_destroy( ApeWorldNode *self )
{
	if ( self->data != NULL )
	{
		switch ( self->type )
		{
			default:
				PRINT_WARNING( "Node is of type empty but has a valid pointer!\n" );
				break;
			case APE_WORLD_NODE_TYPE_ROOM:
				ape_world_room_destroy( self->data );
				break;
			case APE_WORLD_NODE_TYPE_BRUSH:
				//TODO
				break;
			case APE_WORLD_NODE_TYPE_LIGHT:
				ape_light_destroy( self->data );
				break;
			case APE_WORLD_NODE_TYPE_CAMERA:
				ape_camera_destroy( self->data );
				break;
			case APE_WORLD_NODE_TYPE_ENTITY:
				ape_entity_destroy( self->data );
				break;
		}

		self->data = NULL;
	}
}

void ape_world_node_attach_data( ApeWorldNode *self, ApeWorldNodeType type, void *data )
{
	self->type = type;
	if ( self->type != APE_WORLD_NODE_TYPE_EMPTY && data == NULL )
	{
		PRINT_WARNING( "Created a node of a specific type without any data!\n" );
		return;
	}

	self->data = data;
}

/// Function dedicated for testing node API.
void ape_world_node_test_command_( unsigned int, char ** )
{
	ApeWorldNode *node = ape_world_node_create( NULL, "root", APE_WORLD_NODE_TYPE_EMPTY, NULL );
	ApeLight *light = ape_light_create( &pl_vecOrigin3, &( PLColourF32 ){ 1.0f, 0.0f, 0.0f, 1.0f }, 1.0f, 0, 0 );
	if ( light != NULL )
	{
		ape_world_node_attach_data( node, APE_WORLD_NODE_TYPE_LIGHT, light );
	}

	ape_world_node_destroy( node );

	PRINT( "done\n" );
}
