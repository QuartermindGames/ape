// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Nodes form the foundation of everything in a scene.
// Author:  Mark E. Sowden

#include "world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

#define APE_WORLD_NODE_ROOT_MAGIC   PL_MAGIC_TO_NUM( 'W', 'L', 'D', ' ' )
#define APE_WORLD_NODE_ROOM_MAGIC   PL_MAGIC_TO_NUM( 'R', 'O', 'O', 'M' )
#define APE_WORLD_NODE_BRUSH_MAGIC  PL_MAGIC_TO_NUM( 'B', 'R', 'S', 'H' )
#define APE_WORLD_NODE_LIGHT_MAGIC  PL_MAGIC_TO_NUM( 'L', 'I', 'T', ' ' )
#define APE_WORLD_NODE_CAMERA_MAGIC PL_MAGIC_TO_NUM( 'C', 'A', 'M', ' ' )
#define APE_WORLD_NODE_ENTITY_MAGIC PL_MAGIC_TO_NUM( 'E', 'N', 'T', ' ' )

static const ApeWorldNodeMagic typeMagic[ APE_WORLD_MAX_NODE_TYPES ] = {
        [APE_WORLD_NODE_TYPE_ROOT] = APE_WORLD_NODE_ROOT_MAGIC,
        [APE_WORLD_NODE_TYPE_ROOM] = APE_WORLD_NODE_ROOM_MAGIC,
        [APE_WORLD_NODE_TYPE_BRUSH] = APE_WORLD_NODE_BRUSH_MAGIC,
        [APE_WORLD_NODE_TYPE_LIGHT] = APE_WORLD_NODE_LIGHT_MAGIC,
        [APE_WORLD_NODE_TYPE_CAMERA] = APE_WORLD_NODE_CAMERA_MAGIC,
        [APE_WORLD_NODE_TYPE_ENTITY] = APE_WORLD_NODE_ENTITY_MAGIC,
};

/// Excludes validation.
static void attach_data( ApeWorldNode *self, void *data, ApeWorldNodeType type )
{
	self->type = type;
	self->data = data;
	if ( self->data != NULL )
	{
		( ( ApeWorldNodeHeader * ) data )->node = self;
	}
}

static bool validate_data( void *data, ApeWorldNodeType type )
{
	if ( data == NULL && type != APE_WORLD_NODE_TYPE_EMPTY )
	{
		PRINT_WARNING( "Attempted to attach a null pointer on a non empty node!\n" );
		return false;
	}
	else if ( data != NULL && type == APE_WORLD_NODE_TYPE_EMPTY )
	{
		PRINT_WARNING( "Passing data to an empty node type!\n" );
		return false;
	}

	if ( type == APE_WORLD_NODE_TYPE_EMPTY )
	{
		return true;
	}

	ApeWorldNodeHeader *header = ( ApeWorldNodeHeader * ) data;
	if ( header->magic != typeMagic[ type ] )
	{
		PRINT_WARNING( "Invalid data for node type!\n" );
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_world_node_setup_header( ApeWorldNodeHeader *header, ApeWorldNodeType type )
{
	header->magic = typeMagic[ type ];
}

ApeWorldNode *ape_world_node_create( ApeWorldNode *parent, const char *name, ApeWorldNodeType type, void *data )
{
	if ( !validate_data( data, type ) )
	{
		PRINT_WARNING( "Invalid data for node type!\n" );
		return NULL;
	}

	ApeWorldNode *self = PL_NEW( ApeWorldNode );
	snprintf( self->name, sizeof( self->name ), "%s", name );

	self->children = PlCreateLinkedList();
	self->transform = PlMatrix4Identity();

	self->parent = parent;
	if ( self->parent != NULL )
	{
		self->parentListNode = PlInsertLinkedListNode( self->parent->children, self );
	}

	attach_data( self, data, type );

	return self;
}

void ape_world_node_destroy( ApeWorldNode *self )
{
	if ( self->data != NULL )
	{
		assert( self->type != APE_WORLD_NODE_TYPE_EMPTY );
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
	}

	PLLinkedListNode *node = PlGetFirstNode( self->children );
	while ( node != NULL )
	{
		ApeWorldNode *child = PlGetLinkedListNodeUserData( node );
		ape_world_node_destroy( child );
		node = PlGetNextLinkedListNode( node );
	}

	PlDestroyLinkedList( self->children );

	PL_DELETE( self );
}

void *ape_world_node_attach_data( ApeWorldNode *self, ApeWorldNodeType type, void *data )
{
	if ( self->data != NULL )
	{
		PRINT_WARNING( "Data already attached to node!\n" );
		return NULL;
	}

	if ( !validate_data( data, type ) )
	{
		return NULL;
	}

	attach_data( self, data, type );
	return self->data;
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
