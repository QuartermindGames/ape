// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
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

void ape_world_destroy_( void *data );
void ape_room_destroy_( void *data );
void ape_brush_destroy_( void *data );
void ape_light_destroy_( void *data );
void ape_camera_destroy_( void *data );
void ape_entity_destroy_( void *data );

static const ApeWorldNodeClass nodeClasses[ APE_WORLD_MAX_NODE_TYPES ] = {
        [APE_WORLD_NODE_TYPE_ROOT] = {
                                      .identifier = "root",
                                      .magic = APE_WORLD_NODE_ROOT_MAGIC,
                                      .destroyFunction = ape_world_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_ROOM] = {
                                      .identifier = "room",
                                      .magic = APE_WORLD_NODE_ROOM_MAGIC,
                                      .destroyFunction = ape_room_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_BRUSH] = {
                                      .identifier = "brush",
                                      .magic = APE_WORLD_NODE_BRUSH_MAGIC,
                                      .destroyFunction = ape_brush_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_LIGHT] = {
                                      .identifier = "light",
                                      .magic = APE_WORLD_NODE_LIGHT_MAGIC,
                                      .destroyFunction = ape_light_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_CAMERA] = {
                                      .identifier = "camera",
                                      .magic = APE_WORLD_NODE_CAMERA_MAGIC,
                                      .destroyFunction = ape_camera_destroy_,
                                      },
        [APE_WORLD_NODE_TYPE_ENTITY] = {
                                      .identifier = "entity",
                                      .magic = APE_WORLD_NODE_ENTITY_MAGIC,
                                      .destroyFunction = ape_entity_destroy_,
                                      },
};

/**
 * Excludes validation.
 */
static void attach_data( ApeWorldNode *self, void *data, ApeWorldNodeType type )
{
	self->type = type;
	self->data = data;
	if ( self->data != nullptr )
	{
		( ( ApeWorldNodeHeader * ) data )->node = self;
	}
}

static bool data_has_world_node_header( const void *data )
{
	assert( data != nullptr );
	return ( ( ( ApeWorldNodeHeader * ) data )->magic == APE_WORLD_NODE_MAGIC );
}

static ApeWorldNode *get_data_world_node( const void *data )
{
	if ( !data_has_world_node_header( data ) )
	{
		return nullptr;
	}

	return ( ( ApeWorldNodeHeader * ) data )->node;
}

static bool validate_data( void *data, ApeWorldNodeType type )
{
	if ( !data_has_world_node_header( data ) )
	{
		ape_warning_( "Data is not valid world node!\n" );
		return false;
	}

	if ( data == NULL && type != APE_WORLD_NODE_TYPE_EMPTY )
	{
		ape_warning_( "Attempted to attach a null pointer on a non empty node!\n" );
		return false;
	}
	else if ( data != NULL && type == APE_WORLD_NODE_TYPE_EMPTY )
	{
		ape_warning_( "Passing data to an empty node type!\n" );
		return false;
	}

	if ( type == APE_WORLD_NODE_TYPE_EMPTY )
	{
		return true;
	}

	ApeWorldNodeHeader *header = ( ApeWorldNodeHeader * ) data;
	if ( header->typeMagic != nodeClasses[ type ].magic )
	{
		ape_warning_( "Invalid data for node type!\n" );
		return false;
	}

	return true;
}

static void setup_data_header( ApeWorldNodeHeader *header, ApeWorldNodeType type )
{
	header->magic = APE_WORLD_NODE_MAGIC;
	header->typeMagic = nodeClasses[ type ].magic;
}

/**
 * Performs some basic validation on the node type before passing back the data.
 */
static void *get_world_node_data( ApeWorldNode *self, ApeWorldNodeType expectedType )
{
	if ( self->type != expectedType )
	{
		return nullptr;
	}

	return self->data;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeWorldNode *ape_world_node_create( ApeWorldNode *parent, ApeWorldNodeType type, const PLVector3 *position, const PLVector3 *angles, void *data )
{
	setup_data_header( ( ApeWorldNodeHeader * ) data, type );

	if ( !validate_data( data, type ) )
	{
		ape_warning_( "Invalid data for node type!\n" );
		return nullptr;
	}

	ApeWorldNode *self = PL_NEW( ApeWorldNode );
	snprintf( self->name, sizeof( self->name ), "%s", nodeClasses[ type ].identifier );

	self->children = PlCreateLinkedList();

	self->transform = PlMatrix4Identity();
	self->transform = PlMultiplyMatrix4( PlRotateMatrix4( angles->x, &( PLVector3 ){ 1.0f, 0.0f, 0.0f } ), &self->transform );
	self->transform = PlMultiplyMatrix4( PlRotateMatrix4( angles->y, &( PLVector3 ){ 0.0f, 1.0f, 0.0f } ), &self->transform );
	self->transform = PlMultiplyMatrix4( PlRotateMatrix4( angles->z, &( PLVector3 ){ 0.0f, 0.0f, 1.0f } ), &self->transform );
	self->transform = PlMultiplyMatrix4( PlTranslateMatrix4( *position ), &self->transform );

	self->classType = &nodeClasses[ type ];

	if ( parent != nullptr )
	{
		ape_world_node_attach( self, parent );
	}

	attach_data( self, data, type );

	return self;
}

void ape_world_node_destroy( ApeWorldNode *self )
{
	if ( self->data != nullptr )
	{
		self->classType->destroyFunction( self->data );
		self->data = nullptr;
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

void ape_world_node_dettach( ApeWorldNode *self )
{
	if ( self->parent == nullptr )
	{
		return;
	}

	assert( self->parentListNode != nullptr );
	PlDestroyLinkedListNode( self->parentListNode );

	self->parent = nullptr;
	self->parentListNode = nullptr;
}

void ape_world_node_attach( ApeWorldNode *self, ApeWorldNode *parent )
{
	if ( self->parent == parent )
	{
		return;
	}

	ape_world_node_dettach( self );

	self->parent = parent;
	self->parentListNode = PlInsertLinkedListNode( self->parent->children, self );
}

void ape_world_node_set_position( ApeWorldNode *self, const PLVector3 *position )
{
	self->transform = PlMultiplyMatrix4( PlTranslateMatrix4( *position ), &self->transform );
}

void ape_world_node_set_angles( ApeWorldNode *self, const PLVector3 *angles )
{
	self->transform = PlMultiplyMatrix4( PlRotateMatrix4( angles->x, &( PLVector3 ){ 1.0f, 0.0f, 0.0f } ), &self->transform );
	self->transform = PlMultiplyMatrix4( PlRotateMatrix4( angles->y, &( PLVector3 ){ 0.0f, 1.0f, 0.0f } ), &self->transform );
	self->transform = PlMultiplyMatrix4( PlRotateMatrix4( angles->z, &( PLVector3 ){ 0.0f, 0.0f, 1.0f } ), &self->transform );
}

ApeRoom *ape_world_node_get_room( ApeWorldNode *self )
{
	ApeWorldNode *next = self->parent;
	while ( next != nullptr )
	{
		if ( next->type == APE_WORLD_NODE_TYPE_ROOM )
		{
			break;
		}

		next = next->parent;
	}

	return ( next != nullptr ) ? ape_world_node_get_room_data( next ) : nullptr;
}

ApeWorldNode *ape_world_node_get_root( ApeWorldNode *self )
{
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
	ApeWorldNode *child = nullptr;
	PLLinkedListNode *node = PlGetFirstNode( self->children );
	while ( node != nullptr )
	{
		ApeWorldNode *current = PlGetLinkedListNodeUserData( node );
		if ( strcmp( current->name, name ) == 0 )
		{
			child = current;
			break;
		}

		node = PlGetNextLinkedListNode( node );
	}

	return child;
}

ApeWorld *ape_world_node_get_root_data( ApeWorldNode *self ) { return ( ApeWorld * ) get_world_node_data( self, APE_WORLD_NODE_TYPE_ROOT ); }
ApeRoom *ape_world_node_get_room_data( ApeWorldNode *self ) { return ( ApeRoom * ) get_world_node_data( self, APE_WORLD_NODE_TYPE_ROOM ); }
ApeLight *ape_world_node_get_light_data( ApeWorldNode *self ) { return ( ApeLight * ) get_world_node_data( self, APE_WORLD_NODE_TYPE_LIGHT ); }
ApeCamera *ape_world_node_get_camera_data( ApeWorldNode *self ) { return ( ApeCamera * ) get_world_node_data( self, APE_WORLD_NODE_TYPE_CAMERA ); }
ApeEntity *ape_world_node_get_entity_data( ApeWorldNode *self ) { return ( ApeEntity * ) get_world_node_data( self, APE_WORLD_NODE_TYPE_ENTITY ); }
ApeBrush *ape_world_node_get_brush_data( ApeWorldNode *self ) { return ( ApeBrush * ) get_world_node_data( self, APE_WORLD_NODE_TYPE_BRUSH ); }

ApeWorldNode *ape_world_get_world_node( ApeWorld *self ) { return get_data_world_node( self ); }
ApeWorldNode *ape_room_get_world_node( ApeRoom *self ) { return get_data_world_node( self ); }
ApeWorldNode *ape_light_get_world_node( ApeLight *self ) { return get_data_world_node( self ); }
ApeWorldNode *ape_camera_get_world_node( ApeCamera *self ) { return get_data_world_node( self ); }
ApeWorldNode *ape_entity_get_world_node( ApeEntity *self ) { return get_data_world_node( self ); }
ApeWorldNode *ape_brush_get_world_node( ApeBrush *self ) { return get_data_world_node( self ); }
