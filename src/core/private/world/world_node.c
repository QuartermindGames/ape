// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Nodes form the foundation of everything in a scene.
// Author:  Mark E. Sowden

#include "world.h"

#define APE_WORLD_NODE_MAGIC PL_MAGIC_TO_NUM( 'N', 'O', 'D', 'E' )

extern const ApeWorldNodeClass ape_rootClass;
extern const ApeWorldNodeClass ape_roomClass;
extern const ApeWorldNodeClass ape_brushClass;
extern const ApeWorldNodeClass ape_lightClass;
extern const ApeWorldNodeClass ape_cameraClass;
extern const ApeWorldNodeClass ape_entityClass;
extern const ApeWorldNodeClass ape_modelClass;

static const ApeWorldNodeClass *nodeClasses[ APE_WORLD_MAX_NODE_TYPES ] = {
        [APE_WORLD_NODE_TYPE_ROOT]   = &ape_rootClass,
        [APE_WORLD_NODE_TYPE_ROOM]   = &ape_roomClass,
        [APE_WORLD_NODE_TYPE_BRUSH]  = &ape_brushClass,
        [APE_WORLD_NODE_TYPE_LIGHT]  = &ape_lightClass,
        [APE_WORLD_NODE_TYPE_CAMERA] = &ape_cameraClass,
        [APE_WORLD_NODE_TYPE_ENTITY] = &ape_entityClass,
        [APE_WORLD_NODE_TYPE_MODEL]  = &ape_modelClass,
};

static const ApeWorldNodeClass *get_class_by_magic( ApeWorldNodeMagic magic )
{
	for ( uint i = 0; i < APE_WORLD_MAX_NODE_TYPES; ++i )
	{
		if ( nodeClasses[ i ] == nullptr || nodeClasses[ i ]->magic != magic )
		{
			continue;
		}

		return nodeClasses[ i ];
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_world_node_generate_bounds_( ApeWorldNode *root )
{
}

/////////////////////////////////////////////////////////////////////////////////////

bool ape_world_node_has_magic( const ApeWorldNode *self )
{
	return ( self->magic == APE_WORLD_NODE_MAGIC );
}

uint32_t ape_world_node_get_class_magic( const ApeWorldNode *self )
{
	assert( self->classType != nullptr );
	return self->classType->magic;
}

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
	self->classType = nodeClasses[ self->type ];

	if ( parent != nullptr )
	{
		ape_world_node_attach( self, parent );
	}

	return self;
}

void ape_world_node_destroy( ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	// dettach it from the parent (but grab so we can pass to destructor)
	ApeWorldNode *parent = self->parent;
	ape_world_node_dettach( self );

	PLLinkedListNode *node = PlGetFirstNode( self->children );
	while ( node != NULL )
	{
		ApeWorldNode *child = PlGetLinkedListNodeUserData( node );
		ape_world_node_destroy( child );
		node = PlGetNextLinkedListNode( node );
	}

	PlDestroyLinkedList( self->children );

	assert( self->classType->destroyFunction );
	self->classType->destroyFunction( self, parent );
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

	// return early if the node provided is already a room
	if ( self->type == APE_WORLD_NODE_TYPE_ROOM )
	{
		return ( ApeRoom * ) self;
	}

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
}

AcmBranch *ape_world_node_serialize( ApeWorldNode *self, AcmBranch *root )
{
	assert( self->classType != nullptr );

	AcmBranch *nodeBranch = acm_push_object( root, "node" );

	if ( *self->name != '\0' )
	{
		acm_push_string( nodeBranch, "name", self->name, false );
	}

	acm_push_vector3( nodeBranch, "position", &self->position, true );
	acm_push_vector3( nodeBranch, "angles", &self->angles, true );
	acm_push_vector3( nodeBranch, "scale", &self->scale, true );

	acm_push_array_f32( nodeBranch, "transform", ( float * ) &self->transform, 16 );

	acm_push_array_f32( nodeBranch, "localBounds", ( float * ) &self->localBounds, 12 );
	acm_push_array_f32( nodeBranch, "bounds", ( float * ) &self->bounds, 12 );

	acm_push_ui32( nodeBranch, "classMagic", self->classType->magic );
	if ( self->classType->serializeFunction != nullptr )
	{
		AcmBranch *classBranch = acm_push_object( nodeBranch, "class" );
		self->classType->serializeFunction( self, classBranch );
	}

	if ( PlGetNumLinkedListNodes( self->children ) > 0 )
	{
		AcmBranch    *childBranch = acm_push_array_object( nodeBranch, "children" );
		ApeWorldNode *child;
		COM_ITERATE_LINKED_LIST( child, self->children, i )
		{
			ape_world_node_serialize( child, childBranch );
		}
	}

	return nodeBranch;
}

ApeWorldNode *ape_world_node_deserialize( ApeWorldNode *parent, AcmBranch *root )
{
	ApeWorldNodeMagic magic = ACM_GET_INT( magic, root, "classMagic", 0 );
	if ( magic == 0 )
	{
		ape_warning_( "No class provided for world node!\n" );
		return nullptr;
	}

	const ApeWorldNodeClass *worldNodeClass = get_class_by_magic( magic );
	if ( worldNodeClass == nullptr )
	{
		ape_warning_( "Unknown class type (%u) for world node!\n", magic );
		return nullptr;
	}

	//TODO: make this an assert
	if ( worldNodeClass->deserializeFunction == nullptr )
	{
		ape_warning_( "No deserialization method specified for class (%s), skipping!\n", worldNodeClass->identifier );
		return nullptr;
	}

	AcmBranch *classBranch = acm_get_child_by_name( root, "class" );
	if ( classBranch == nullptr )
	{
		ape_warning_( "Class data not specified for node!\n" );
		return nullptr;
	}

	ApeWorldNode *self = worldNodeClass->deserializeFunction( parent, classBranch );
	if ( self == nullptr )
	{
		ape_warning_( "Failed to deserialize world node!\n" );
		return nullptr;
	}

	snprintf( self->name, sizeof( self->name ), "%s", acm_get_string( root, "name", "" ) );

	self->position = acm_get_vector3( root, "position", &pl_vecOrigin3 );
	self->angles   = acm_get_vector3( root, "angles", &pl_vecOrigin3 );
	self->scale    = acm_get_vector3( root, "scale", &pl_vecOrigin3 );

	acm_get_array_f32( root, "transform", ( float * ) &self->transform, 16 );
	acm_get_array_f32( root, "localBounds", ( float * ) &self->localBounds, 12 );
	acm_get_array_f32( root, "bounds", ( float * ) &self->bounds, 12 );

	// deal with the children
	AcmBranch *childrenBranch = acm_get_child_by_name( root, "children" );
	if ( childrenBranch != nullptr )
	{
		AcmBranch *childBranch = acm_get_first_child( childrenBranch );
		while ( childBranch != nullptr )
		{
			ape_world_node_deserialize( self, childBranch );
			childBranch = acm_get_next_child( childBranch );
		}
	}

	return self;
}
