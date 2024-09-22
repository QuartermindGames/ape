// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Nodes form the foundation of everything in a scene.
// Author:  Mark E. Sowden

#include "world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

#define APE_WORLD_NODE_MAGIC PL_MAGIC_TO_NUM( 'N', 'O', 'D', 'E' )

#define APE_WORLD_NODE_ROOT_MAGIC   PL_MAGIC_TO_NUM( 'W', 'L', 'D', ' ' )
#define APE_WORLD_NODE_ROOM_MAGIC   PL_MAGIC_TO_NUM( 'R', 'O', 'O', 'M' )
#define APE_WORLD_NODE_BRUSH_MAGIC  PL_MAGIC_TO_NUM( 'B', 'R', 'S', 'H' )
#define APE_WORLD_NODE_LIGHT_MAGIC  PL_MAGIC_TO_NUM( 'L', 'I', 'T', ' ' )
#define APE_WORLD_NODE_CAMERA_MAGIC PL_MAGIC_TO_NUM( 'C', 'A', 'M', ' ' )
#define APE_WORLD_NODE_ENTITY_MAGIC PL_MAGIC_TO_NUM( 'E', 'N', 'T', ' ' )

void ape_brush_node_draw_( void *data, const PLMatrix4 *transform );

void ape_world_destroy_( void *data );
void ape_room_destroy_( void *data );
void ape_brush_destroy_( void *data );
void ape_light_destroy_( void *data );
void ape_camera_destroy_( void *data );
void ape_entity_destroy_( void *data );

static const ApeWorldNodeClass nodeClasses[ APE_WORLD_MAX_NODE_TYPES ] = {
        [APE_WORLD_NODE_TYPE_ROOT] = {
                                      .identifier      = "root",
                                      .magic           = APE_WORLD_NODE_ROOT_MAGIC,
                                      .drawFunction    = nullptr,
                                      .destroyFunction = ape_world_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_ROOM] = {
                                      .identifier      = "room",
                                      .magic           = APE_WORLD_NODE_ROOM_MAGIC,
                                      .drawFunction    = nullptr,
                                      .destroyFunction = ape_room_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_BRUSH] = {
                                      .identifier      = "brush",
                                      .magic           = APE_WORLD_NODE_BRUSH_MAGIC,
                                      .drawFunction    = ape_brush_node_draw_,
                                      .destroyFunction = ape_brush_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_LIGHT] = {
                                      .identifier      = "light",
                                      .magic           = APE_WORLD_NODE_LIGHT_MAGIC,
                                      .drawFunction    = nullptr,
                                      .destroyFunction = ape_light_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_CAMERA] = {
                                      .identifier      = "camera",
                                      .magic           = APE_WORLD_NODE_CAMERA_MAGIC,
                                      .drawFunction    = nullptr,
                                      .destroyFunction = ape_camera_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_ENTITY] = {
                                      .identifier      = "entity",
                                      .magic           = APE_WORLD_NODE_ENTITY_MAGIC,
                                      .drawFunction    = nullptr,
                                      .destroyFunction = ape_entity_destroy_,
                                      },
};

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_world_node_generate_bounds_( ApeWorldNode *root )
{
}

/////////////////////////////////////////////////////////////////////////////////////

bool ape_world_node_is_valid( const ApeWorldNode *self, ApeWorldNodeType expectedType )
{
	if ( self->magic != APE_WORLD_NODE_MAGIC )
	{
		ape_warning_( "Unexpected magic for world node (%u != %u)!\n", self->magic, APE_WORLD_NODE_MAGIC );
		return false;
	}

	if ( self->type != expectedType )
	{
		ape_warning_( "Unexpected type for world node (%u != %u)!\n", self->type, expectedType );
		return false;
	}

	return true;
}

ApeWorldNode *ape_world_node_setup_( ApeWorldNode *self, ApeWorldNode *parent, ApeWorldNodeType type, const char *name, const PLVector3 *position, const PLVector3 *angles )
{
	self->magic = APE_WORLD_NODE_MAGIC;

	if ( name == nullptr )
	{
		name = self->name;
	}

	snprintf( self->name, sizeof( self->name ), "%s", name );

	self->children = PlCreateLinkedList();

	self->position = *position;
	self->angles   = *angles;

	self->type      = type;
	self->classType = &nodeClasses[ self->type ];

	if ( parent != nullptr )
	{
		ape_world_node_attach( self, parent );
	}

	return self;
}

void ape_world_node_destroy( ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	PLLinkedListNode *node = PlGetFirstNode( self->children );
	while ( node != NULL )
	{
		ApeWorldNode *child = PlGetLinkedListNodeUserData( node );
		ape_world_node_destroy( child );
		node = PlGetNextLinkedListNode( node );
	}

	PlDestroyLinkedList( self->children );

	assert( self->classType->destroyFunction );
	self->classType->destroyFunction( self );
}

void ape_world_node_dettach( ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	if ( self->parent == nullptr )
	{
		return;
	}

	assert( self->parentListNode != nullptr );
	PlDestroyLinkedListNode( self->parentListNode );

	self->parent         = nullptr;
	self->parentListNode = nullptr;
}

void ape_world_node_attach( ApeWorldNode *self, ApeWorldNode *parent )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	if ( self->parent == parent )
	{
		return;
	}

	ape_world_node_dettach( self );

	self->parent         = parent;
	self->parentListNode = PlInsertLinkedListNode( self->parent->children, self );
}

PLVector3 ape_world_node_get_position( const ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );
	return self->position;
}

void ape_world_node_set_position( ApeWorldNode *self, const PLVector3 *position )
{
	assert( ape_world_node_is_valid( self, self->type ) );
	self->position = *position;
}

void ape_world_node_move( ApeWorldNode *self, PLVector3 translation )
{
	self->position = PlAddVector3( self->position, translation );

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->children, i )
	{
		ape_world_node_move( child, translation );
	}
}

PLVector3 ape_world_node_get_angles( const ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );
	return self->angles;
}

void ape_world_node_set_angles( ApeWorldNode *self, const PLVector3 *angles )
{
	assert( ape_world_node_is_valid( self, self->type ) );
	self->angles = *angles;
}

void ape_world_node_set_local_bounds( ApeWorldNode *self, const PLVector3 *mins, const PLVector3 *maxs )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	self->localBounds.mins = *mins;
	self->localBounds.maxs = *maxs;

	// need to go ahead and recalc bounds

	self->bounds = self->localBounds;

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->children, i )
	{
#pragma message "THIS SHOULD BE ACCOUNTING FOR TRANSFORMS YOU DUMB BASTARD"
		if ( child->bounds.mins.x < self->bounds.mins.x ) { self->bounds.mins.x = child->bounds.mins.x; }
		if ( child->bounds.mins.y < self->bounds.mins.y ) { self->bounds.mins.y = child->bounds.mins.y; }
		if ( child->bounds.mins.z < self->bounds.mins.z ) { self->bounds.mins.z = child->bounds.mins.z; }
		if ( child->bounds.maxs.x > self->bounds.maxs.x ) { self->bounds.maxs.x = child->bounds.maxs.x; }
		if ( child->bounds.maxs.y > self->bounds.maxs.y ) { self->bounds.maxs.y = child->bounds.maxs.y; }
		if ( child->bounds.maxs.z > self->bounds.maxs.z ) { self->bounds.maxs.z = child->bounds.maxs.z; }
	}

	// and now wake our parents up...
}

ApeRoom *ape_world_node_get_room( ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	ApeWorldNode *next = self->parent;
	while ( next != nullptr )
	{
		if ( next->type == APE_WORLD_NODE_TYPE_ROOM )
		{
			assert( ape_world_node_is_valid( next, APE_WORLD_NODE_TYPE_ROOM ) );
			break;
		}

		next = next->parent;
	}

	return ( next != nullptr ) ? ( ApeRoom * ) next : nullptr;
}

void ape_world_node_set_room( ApeWorldNode *self, ApeRoom *room )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	// just explicitly set the node to it's new parent
	ape_world_node_attach( self, &room->base );
}

ApeWorldNode *ape_world_node_get_root( ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	ApeWorldNode *root = self;
	ApeWorldNode *next = self->parent;
	while ( next != nullptr )
	{
		root = next;
		next = next->parent;
	}

	return ( root->type == APE_WORLD_NODE_TYPE_ROOT ) ? root : nullptr;
}

ApeWorldNode *ape_world_node_get_child_by_name( ApeWorldNode *self, const char *name )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	ApeWorldNode *child = nullptr;
	ApeWorldNode *current;
	COM_ITERATE_LINKED_LIST( current, self->children, i )
	{
		if ( strcmp( current->name, name ) == 0 )
		{
			child = current;
			break;
		}
	}

	return child;
}

const char *ape_world_node_get_name( ApeWorldNode *self )
{
	return self->name;
}

void ape_world_node_set_name( ApeWorldNode *self, const char *name )
{
	snprintf( self->name, sizeof( self->name ), "%s", name );
	printf( "%s\n", self->name );
}

void ape_world_node_draw_bounding_volume_( ApeWorldNode *self )
{
}

void ape_world_node_draw_( ApeWorldNode *self, const PLMatrix4 *transform )
{
}
