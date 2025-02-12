// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
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
	for ( unsigned int i = 0; i < APE_WORLD_MAX_NODE_TYPES; ++i )
	{
		if ( nodeClasses[ i ] == nullptr || nodeClasses[ i ]->magic != magic )
		{
			continue;
		}

		return nodeClasses[ i ];
	}

	return nullptr;
}

const ApeWorldNodeClass **ape_world_node_get_classes( unsigned int *numClasses )
{
	*numClasses = APE_WORLD_MAX_NODE_TYPES;
	return nodeClasses;
}

const ApeWorldNodeProperty *ape_world_node_get_properties( unsigned int *numProperties )
{
	static const ApeWorldNodeProperty properties[] = {
	        APE_WORLD_NODE_PROPERTY_STRING( "Name", "Name of the node.", ApeWorldNode, name ),
	        APE_WORLD_NODE_PROPERTY_BASIC( "Position", "Position of the node in 3D space.", ApeWorldNode, position, VEC3 ),
	        APE_WORLD_NODE_PROPERTY_BASIC( "Angles", "Angles of the node in 3D space.", ApeWorldNode, angles, VEC3 ),
	        APE_WORLD_NODE_PROPERTY_BASIC( "Scale", "Scale of the node in 3D space.", ApeWorldNode, scale, VEC3 ),
	};
	*numProperties = PL_ARRAY_ELEMENTS( properties );

	return properties;
}

const ApeWorldNodeProperty *ape_world_node_get_class_properties( unsigned int *numProperties, ApeWorldNodeType type )
{
	const ApeWorldNodeClass *nodeClass = nodeClasses[ type ];
	assert( nodeClass->properties != nullptr );
	*numProperties = nodeClass->numProperties;

	return nodeClass->properties;
}

void *ape_world_node_get_property_value( ApeWorldNode *self, const ApeWorldNodeProperty *property )
{
	return self + property->offset;
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_world_node_generate_bounds_( ApeWorldNode *self )
{
	for ( unsigned int i = 0; i < 3; ++i )
	{
		PLCollisionAABB localBounds = ape_world_node_get_transformed_local_bounds( self );
		localBounds.mins            = PlAddVector3( localBounds.mins, localBounds.origin );
		localBounds.maxs            = PlAddVector3( localBounds.maxs, localBounds.origin );

		if ( PL_VECTOR3_I( localBounds.maxs, i ) > PL_VECTOR3_I( self->bounds.maxs, i ) )
		{
			PL_VECTOR3_I( self->bounds.maxs, i ) = PL_VECTOR3_I( localBounds.maxs, i );
		}
		if ( PL_VECTOR3_I( localBounds.mins, i ) < PL_VECTOR3_I( self->bounds.mins, i ) )
		{
			PL_VECTOR3_I( self->bounds.mins, i ) = PL_VECTOR3_I( localBounds.mins, i );
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->children, i )
	{
		ape_world_node_generate_bounds_( child );
	}
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

static ApeRoom *lookup_parent_room( ApeWorldNode *self )
{
	ApeWorldNode *roomNode = ape_world_node_get_parent_by_type( self, APE_WORLD_NODE_TYPE_ROOM );
	if ( roomNode == nullptr )
	{
		ape_warning_( "Encountered a node (%s) without an associated room!\n", PlPrintVector3( &self->position, PL_VAR_F32 ) );
		return nullptr;
	}

	return ( ApeRoom * ) roomNode;
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

	self->room = nullptr;
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

	// determine if we're now under a new room
	if ( self->type != APE_WORLD_NODE_TYPE_ROOM )
	{
		self->room = lookup_parent_room( self );
	}
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

ApeWorldNode *ape_world_node_get_parent_by_type( ApeWorldNode *self, ApeWorldNodeType type )
{
	ApeWorldNode *parent = self->parent;
	while ( parent != nullptr )
	{
		if ( parent->type == type )
		{
			break;
		}

		parent = parent->parent;
	}

	return parent;
}

ApeRoom *ape_world_node_get_room( ApeWorldNode *self )
{
	return self->room;
}

void ape_world_node_set_room( ApeWorldNode *self, ApeRoom *room )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	// just explicitly set the node to its new parent
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

PLCollisionAABB ape_world_node_get_transformed_local_bounds( const ApeWorldNode *self )
{
	PLCollisionAABB bounds    = self->localBounds;
	PLMatrix4       transform = ape_world_node_get_transform( self );
	bounds.origin             = PlGetMatrix4Translation( &transform );
	return bounds;
}

PLCollisionAABB ape_world_node_get_bounds( const ApeWorldNode *self )
{
	return self->bounds;
}

PLMatrix4 ape_world_node_get_transform( const ApeWorldNode *self )
{
	PLMatrix4 transform = ape_world_node_get_local_transform( self );

	ApeWorldNode *parent = self->parent;
	while ( parent != nullptr )
	{
		PLMatrix4 parentTransform = ape_world_node_get_local_transform( parent );
		transform                 = PlMultiplyMatrix4( &parentTransform, &transform );
		parent                    = parent->parent;
	}

	return transform;
}

PLMatrix4 ape_world_node_get_local_transform( const ApeWorldNode *self )
{
	PLMatrix4 transform = PlMatrix4Identity();

	PLMatrix4 translate = PlTranslateMatrix4( self->position );
	transform           = PlMultiplyMatrix4( &transform, &translate );

	PLMatrix4 rotate;
	rotate    = PlRotateMatrix4( PL_DEG2RAD( self->angles.x ), &PL_VECTOR3( 1.0f, 0.0f, 0.0f ) );
	transform = PlMultiplyMatrix4( &transform, &rotate );
	rotate    = PlRotateMatrix4( PL_DEG2RAD( self->angles.y ), &PL_VECTOR3( 0.0f, 1.0f, 0.0f ) );
	transform = PlMultiplyMatrix4( &transform, &rotate );
	rotate    = PlRotateMatrix4( PL_DEG2RAD( self->angles.z ), &PL_VECTOR3( 0.0f, 0.0f, 1.0f ) );
	transform = PlMultiplyMatrix4( &transform, &rotate );

	return transform;
}

AcmBranch *ape_world_node_serialize( ApeWorldNode *self, AcmBranch *root )
{
	assert( self->classType != nullptr );

	AcmBranch *nodeBranch = acm_push_object( root, "node" );

	if ( *self->name != '\0' )
	{
		acm_push_string( nodeBranch, "name", self->name, false );
	}

	com_acm_push_vector3( nodeBranch, "position", &self->position, true );
	com_acm_push_vector3( nodeBranch, "angles", &self->angles, true );
	com_acm_push_vector3( nodeBranch, "scale", &self->scale, true );

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

	self->position = com_acm_get_vector3( root, "position", &pl_vecOrigin3 );
	self->angles   = com_acm_get_vector3( root, "angles", &pl_vecOrigin3 );
	self->scale    = com_acm_get_vector3( root, "scale", &pl_vecOrigin3 );

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
