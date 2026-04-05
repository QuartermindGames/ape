// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Nodes form the foundation of everything in a scene.
// Author:  Mark E. Sowden

#include <float.h>

#include "qmos/public/qm_os_random.h"

#include "world.h"

#define APE_WORLD_NODE_MAGIC QM_OS_MAGIC_TO_NUM( 'N', 'O', 'D', 'E' )

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

const ApeProperty *ape_world_node_get_properties( unsigned int *numProperties )
{
	static const ApeProperty properties[] = {
	        APE_PROPERTY_STRING( "Name", "Name of the node.", ApeWorldNode, name ),
	        APE_PROPERTY_BASIC( "Position", "Position of the node in 3D space.", ApeWorldNode, position, VEC3 ),
	        APE_PROPERTY_BASIC( "Angles", "Angles of the node in 3D space.", ApeWorldNode, angles, VEC3 ),
	        APE_PROPERTY_BASIC( "Scale", "Scale of the node in 3D space.", ApeWorldNode, scale, VEC3 ),
	};
	*numProperties = QM_OS_ARRAY_ELEMENTS( properties );

	return properties;
}

const ApeProperty *ape_world_node_get_class_properties( unsigned int *numProperties, ApeWorldNodeType type )
{
	const ApeWorldNodeClass *nodeClass = nodeClasses[ type ];
	*numProperties                     = nodeClass->numProperties;
	return nodeClass->properties;
}

void *ape_world_node_get_property_pointer( ApeWorldNode *self, const ApeProperty *property )
{
	return ( char * ) self + property->offset;
}

/////////////////////////////////////////////////////////////////////////////////////

static void compute_bounds( ApeWorldNode *self, const ApeWorldNode *other )
{
	PLCollisionAABB localBounds = ape_world_node_get_transformed_local_bounds( other );
	localBounds.mins            = qm_math_vector3f_add( localBounds.mins, localBounds.origin );
	localBounds.maxs            = qm_math_vector3f_add( localBounds.maxs, localBounds.origin );

	for ( unsigned int i = 0; i < 3; ++i )
	{
		if ( localBounds.maxs.v[ i ] > self->bounds.maxs.v[ i ] )
		{
			self->bounds.maxs.v[ i ] = localBounds.maxs.v[ i ];
		}
		if ( localBounds.mins.v[ i ] < self->bounds.mins.v[ i ] )
		{
			self->bounds.mins.v[ i ] = localBounds.mins.v[ i ];
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, other->children, i )
	{
		compute_bounds( self, child );
	}
}

void ape_world_node_compute_bounds_( ApeWorldNode *self )
{
	self->bounds.maxs = qm_math_vector3f( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	self->bounds.mins = qm_math_vector3f( FLT_MAX, FLT_MAX, FLT_MAX );

	compute_bounds( self, self );

	if ( self->parent != nullptr )
	{
		ape_world_node_compute_bounds_( self->parent );
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
		ape_console_warning_( "Unexpected magic for world node (%u != %u)!\n", self->magic, APE_WORLD_NODE_MAGIC );
		return false;
	}

	if ( self->type != expectedType )
	{
		ape_console_warning_( "Unexpected type for world node (%u != %u)!\n", self->type, expectedType );
		return false;
	}

	return true;
}

static ApeRoom *lookup_parent_room( ApeWorldNode *self )
{
	ApeWorldNode *roomNode = ape_world_node_get_parent_by_type( self, APE_WORLD_NODE_TYPE_ROOM );
	if ( roomNode == nullptr )
	{
		char tmp[ 64 ];
		qm_math_vector3f_print( self->position, tmp, sizeof( tmp ) );

		ape_console_warning_( "Encountered a node (%s) without an associated room!\n", tmp );
		return nullptr;
	}

	return ( ApeRoom * ) roomNode;
}

static void update_local_transform( ApeWorldNode *self )
{
	PLMatrix4 transform = PlTranslateMatrix4( self->position );

	PLMatrix4 rotate;
	rotate    = PlRotateMatrix4( QM_MATH_DEG2RAD( self->angles.x ), &QM_MATH_VECTOR3F( 1.0f, 0.0f, 0.0f ) );
	transform = PlMultiplyMatrix4( &transform, &rotate );
	rotate    = PlRotateMatrix4( QM_MATH_DEG2RAD( self->angles.y ), &QM_MATH_VECTOR3F( 0.0f, 1.0f, 0.0f ) );
	transform = PlMultiplyMatrix4( &transform, &rotate );
	rotate    = PlRotateMatrix4( QM_MATH_DEG2RAD( self->angles.z ), &QM_MATH_VECTOR3F( 0.0f, 0.0f, 1.0f ) );
	transform = PlMultiplyMatrix4( &transform, &rotate );

	transform = PlScaleMatrix4( transform, self->scale );

	self->localTransform = transform;
}

static void update_world_transform( ApeWorldNode *self )
{
	PLMatrix4 transform = ape_world_node_get_local_transform( self );

	ApeWorldNode *parent = self->parent;
	while ( parent != nullptr )
	{
		PLMatrix4 parentTransform = ape_world_node_get_local_transform( parent );
		transform                 = PlMultiplyMatrix4( &parentTransform, &transform );
		parent                    = parent->parent;
	}

	self->worldTransform = transform;

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->children, i )
	{
		update_world_transform( child );
	}
}

static void update_transform( ApeWorldNode *self )
{
	update_local_transform( self );
	update_world_transform( self );

	ape_world_node_compute_bounds_( self );
}

ApeWorldNode *ape_world_node_setup_( ApeWorldNode *self, ApeWorldNode *parent, ApeWorldNodeType type, const char *name, const QmMathVector3f *position, const QmMathVector3f *angles )
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
	self->scale    = QM_MATH_VECTOR3F( 1.0f, 1.0f, 1.0f );

	self->type      = type;
	self->classType = nodeClasses[ self->type ];

	if ( parent != nullptr )
	{
		ape_world_node_attach( self, parent );
	}

	update_transform( self );

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

	PlgDestroyMesh( self->mesh );

	assert( self->classType->destroy );
	self->classType->destroy( self, parent );
}

void ape_world_node_dettach( ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	if ( self->parent == nullptr )
	{
		return;
	}

	// allow the class to operate any logic as needed
	ApeWorldNode *parent = self->parent;
	if ( parent->classType->onDettachChild != nullptr )
	{
		parent->classType->onDettachChild( parent, self );
	}
	if ( self->classType->onDettachParent != nullptr )
	{
		self->classType->onDettachParent( self, parent );
	}

	ape_world_node_mark_dirty_( parent );

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
		// sigh...this is very specific, so we can notify a brush (or anything else)
		// that it's moved to a new room.
		ApeRoom *room = lookup_parent_room( self );
		if ( self->room != nullptr && room != nullptr && room != self->room )
		{
			if ( self->classType->onChangeRoom != nullptr )
			{
				self->classType->onChangeRoom( self, self->room, room );
			}
		}

		self->room = room;
	}

	// allow the class to operate any logic as needed
	if ( parent->classType->onAttachChild != nullptr )
	{
		parent->classType->onAttachChild( parent, self );
	}
	if ( self->classType->onAttachParent != nullptr )
	{
		self->classType->onAttachParent( self, parent );
	}

	ape_world_node_mark_dirty_( parent );
}

QmMathVector3f ape_world_node_get_local_position( const ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );
	return self->position;
}

QmMathVector3f ape_world_node_get_position( const ApeWorldNode *self )
{
	return PlGetMatrix4Translation( &self->worldTransform );
}

void ape_world_node_set_position( ApeWorldNode *self, const QmMathVector3f *position )
{
	assert( ape_world_node_is_valid( self, self->type ) );
	self->position = *position;

	update_transform( self );
}

QmMathVector3f ape_world_node_get_angles( const ApeWorldNode *self )
{
	assert( ape_world_node_is_valid( self, self->type ) );
	return self->angles;
}

void ape_world_node_set_angles( ApeWorldNode *self, const QmMathVector3f *angles )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	com_math_normalize_angles( angles, &self->angles );

	update_transform( self );
}

void ape_world_node_set_local_bounds( ApeWorldNode *self, const QmMathVector3f *mins, const QmMathVector3f *maxs )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	self->localBounds.mins = *mins;
	self->localBounds.maxs = *maxs;

	ape_world_node_compute_bounds_( self );
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

ApeWorldNode *ape_world_node_get_parent_by_name( const ApeWorldNode *self, const char *name )
{
	ApeWorldNode *parent = self->parent;
	while ( parent != nullptr )
	{
		if ( strcmp( parent->name, name ) == 0 )
		{
			break;
		}

		parent = parent->parent;
	}

	return parent;
}

bool ape_world_node_is_descendant_of_node( const ApeWorldNode *self, const ApeWorldNode *lookup )
{
	assert( self != lookup );

	ApeWorldNode *parent = self->parent;
	while ( parent != nullptr )
	{
		if ( parent == lookup )
		{
			return true;
		}

		parent = parent->parent;
	}

	return false;
}

ApeRoom *ape_world_node_get_room( ApeWorldNode *self )
{
	// if we want the parent room of a room, well that's us
	if ( self->type == APE_WORLD_NODE_TYPE_ROOM )
	{
		return ( ApeRoom * ) self;
	}

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

	if ( self->parent == nullptr )
	{
		return self;
	}

	ApeWorldNode *root = self;
	ApeWorldNode *next = self->parent;
	while ( next != nullptr )
	{
		root = next;
		next = next->parent;
	}

	return root->type == APE_WORLD_NODE_TYPE_ROOT ? root : nullptr;
}

ApeWorldNode *ape_world_node_get_child_by_name( const ApeWorldNode *self, const char *name )
{
	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->children, i )
	{
		if ( strcmp( child->name, name ) == 0 )
		{
			return child;
		}
	}

	return nullptr;
}

ApeWorldNode *ape_world_node_get_descendant_by_name( const ApeWorldNode *self, const char *name )
{
	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->children, i )
	{
		if ( strcmp( child->name, name ) == 0 )
		{
			return child;
		}

		ApeWorldNode *next = ape_world_node_get_descendant_by_name( self, name );
		if ( next != nullptr )
		{
			return next;
		}
	}

	return nullptr;
}

const char *ape_world_node_get_name( const ApeWorldNode *self )
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

PLCollisionAABB ape_world_node_get_local_bounds( const ApeWorldNode *self )
{
	return self->localBounds;
}

PLCollisionAABB ape_world_node_get_bounds( const ApeWorldNode *self )
{
	return self->bounds;
}

PLMatrix4 ape_world_node_get_transform( const ApeWorldNode *self )
{
	return self->worldTransform;
}

PLMatrix4 ape_world_node_get_local_transform( const ApeWorldNode *self )
{
	return self->localTransform;
}

QmMathVector3f ape_world_node_get_forward( const ApeWorldNode *self )
{
	QmMathVector3f forward;
	PlAnglesAxes( self->angles, nullptr, nullptr, &forward );
	return qm_math_vector3f_normalize( forward );
}

//!! KEEP THIS PRIVATE !!
// don't want to end up with a can of worms with other
// code trying to do version-specific behaviour
static constexpr unsigned int WORLD_NODE_VERSION = 1;

AcmBranch *ape_world_node_serialize( ApeWorldNode *self, AcmBranch *root )
{
	assert( self->classType != nullptr );

	if ( self->flags & APE_WORLD_NODE_FLAG_DISCARD )
	{
		// not a type that's saved
		return nullptr;
	}

	AcmBranch *nodeBranch = acm_push_object( root, "node" );

	acm_push_i32( nodeBranch, "version", WORLD_NODE_VERSION );

	if ( *self->name != '\0' )
	{
		acm_push_string( nodeBranch, "name", self->name, false );
	}

	com_acm_push_vector3( nodeBranch, "position", &self->position, true );
	com_acm_push_vector3( nodeBranch, "angles", &self->angles, true );
	com_acm_push_vector3( nodeBranch, "scale", &self->scale, true );

	//acm_push_array_f32( nodeBranch, "localTransform", ( float * ) &self->localTransform, 16 );
	//acm_push_array_f32( nodeBranch, "localBounds", ( float * ) &self->localBounds, 12 );
	//acm_push_array_f32( nodeBranch, "bounds", ( float * ) &self->bounds, 12 );

	acm_push_ui32( nodeBranch, "flags", self->flags );

	acm_push_ui32( nodeBranch, "classMagic", self->classType->magic );
	if ( self->classType->serialize != nullptr )
	{
		AcmBranch *classBranch = acm_push_object( nodeBranch, "class" );
		self->classType->serialize( self, classBranch );
	}

	if ( PlGetNumLinkedListNodes( self->children ) > 0 )
	{
		AcmBranch    *childBranch = acm_push_array_object( nodeBranch, "children" );
		ApeWorldNode *child;
		COM_ITERATE_LINKED_LIST( child, self->children, i )
		{
			ape_world_node_serialize( child, childBranch );
		}

		// this is a little silly, but because some children
		// might be skipped, we may end up with zero children
		// after the fact
		if ( acm_get_num_of_children( childBranch ) == 0 )
		{
			acm_branch_destroy( childBranch );
		}
	}

	return nodeBranch;
}

ApeWorldNode *ape_world_node_deserialize( ApeWorldNode *parent, AcmBranch *root, const char *path )
{
	ApeWorldNodeMagic magic = ACM_GET_INT( magic, root, "classMagic", 0 );
	if ( magic == 0 )
	{
		ape_console_warning_( "No class provided for world node!\n" );
		return nullptr;
	}

	const ApeWorldNodeClass *worldNodeClass = get_class_by_magic( magic );
	if ( worldNodeClass == nullptr )
	{
		ape_console_warning_( "Unknown class type (%u) for world node!\n", magic );
		return nullptr;
	}

	//TODO: make this an assert
	if ( worldNodeClass->deserialize == nullptr )
	{
		ape_console_warning_( "No deserialization method specified for class (%s), skipping!\n", worldNodeClass->identifier );
		return nullptr;
	}

	AcmBranch *classBranch = acm_get_child_by_name( root, "class" );
	if ( classBranch == nullptr )
	{
		ape_console_warning_( "Class data not specified for node!\n" );
		return nullptr;
	}

	assert( worldNodeClass->create != nullptr );
	ApeWorldNode *self = worldNodeClass->create( parent );
	if ( self == nullptr )
	{
		ape_console_warning_( "Failed to create world node!\n" );
		return nullptr;
	}

	//TODO: this should be passed into the deserialize callback,
	//		I've not done so yet just to avoid mass refactoring :(
	if ( path != nullptr )
	{
		QmFsMount *mount = PlGetMountLocationForPath( path );
		if ( mount != nullptr )
		{
			const char *mountPath = qm_fs_mount_get_path( mount );
			assert( mountPath != nullptr );
			snprintf( self->path, sizeof( self->path ), "%s", &path[ strlen( mountPath ) + 1 ] );
		}
		else
		{
			PlSetupPath( self->path, false, "%s", path );
		}
	}

	const unsigned int version = acm_get_int( root, "version", 0 );

	snprintf( self->name, sizeof( self->name ), "%s", acm_get_string( root, "name", "" ) );

	self->position = com_acm_get_vector3( root, "position", &QM_MATH_VECTOR3F_ZERO );
	self->angles   = com_acm_get_vector3( root, "angles", &QM_MATH_VECTOR3F_ZERO );
	if ( version > 0 )
	{
		self->scale = com_acm_get_vector3( root, "scale", &QM_MATH_VECTOR3F_ZERO );
	}
	else
	{
		// for older versions, we incorrectly stored scale
		// (as we didn't really use it) so for those we'll
		// just reset scale to 1
		self->scale = QM_MATH_VECTOR3F( 1.0f, 1.0f, 1.0f );
	}

	//acm_get_array_f32( root, "localTransform", ( float * ) &self->localTransform, 16 );
	//acm_get_array_f32( root, "localBounds", ( float * ) &self->localBounds, 12 );
	//acm_get_array_f32( root, "bounds", ( float * ) &self->bounds, 12 );

	self->flags = acm_get_uint( root, "flags", 0 );

	if ( worldNodeClass->deserialize( self, parent, classBranch ) == nullptr )
	{
		ape_console_warning_( "Failed to deserialize world node!\n" );
		return nullptr;
	}

	// deal with the children
	AcmBranch *childrenBranch = acm_get_child_by_name( root, "children" );
	if ( childrenBranch != nullptr )
	{
		AcmBranch *childBranch = acm_get_first_child( childrenBranch );
		while ( childBranch != nullptr )
		{
			ape_world_node_deserialize( self, childBranch, nullptr );
			childBranch = acm_get_next_child( childBranch );
		}
	}

	update_transform( self );

	return self;
}

static void gather_children( ApeWorldNode *worldNode, ApeWorldNodeType type, PLVectorArray *array, bool recursive )
{
	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, worldNode->children, i )
	{
		if ( child->type == type )
		{
			PlPushBackVectorArrayElement( array, child );
		}

		if ( recursive )
		{
			gather_children( child, type, array, recursive );
		}
	}
}

ApeWorldNode **ape_world_node_gather_children( ApeWorldNode *self, ApeWorldNodeType type, unsigned int *numChildren, bool recursive )
{
	unsigned int reserve = PlGetNumLinkedListNodes( self->children );
	if ( reserve == 0 )
	{
		*numChildren = 0;
		return nullptr;
	}

	PLVectorArray *array = PlCreateVectorArray( reserve );
	if ( array == nullptr )
	{
		ape_console_warning_( "Failed to gather children: %s\n", PlGetError() );
		*numChildren = 0;
		return nullptr;
	}

	gather_children( self, type, array, recursive );

	ApeWorldNode **children = ( ApeWorldNode ** ) PlGetVectorArrayDataEx( array, numChildren );
	PlDestroyVectorArrayContainer( array );

	return children;
}

unsigned int ape_world_node_visit_children( ApeWorldNode *self, ApeWorldNodeType type, bool recursive, bool ( *callback )( ApeWorldNode *self, void *user ), void *user )
{
	unsigned int numVisited = 0;

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->children, i )
	{
		if ( child->type == type )
		{
			callback( child, user );
			numVisited++;
		}

		if ( recursive )
		{
			numVisited += ape_world_node_visit_children( child, type, recursive, callback, user );
		}
	}

	return numVisited;
}

ApeWorldNode *ape_world_node_load( ApeWorldNode *parent, const char *path )
{
	AcmBranch *branch = com_acm_load_file( path, "node" );
	if ( branch == nullptr )
	{
		ape_console_warning_( "Failed to load the specified node (%s): %s\n", path, acm_get_error_message() );
		return nullptr;
	}

	ApeWorldNode *worldNode = ape_world_node_deserialize( parent, branch, path );
	if ( worldNode == nullptr )
	{
		ape_console_warning_( "Failed to deserialize node (%s)!\n", path );
	}

	acm_branch_destroy( branch );

	return worldNode;
}

const char *ape_world_node_get_path( const ApeWorldNode *self )
{
	if ( *self->path == '\0' )
	{
		return nullptr;
	}

	return self->path;
}

ApeWorldNode *ape_world_node_get_parent( ApeWorldNode *self )
{
	return self->parent;
}

/////////////////////////////////////////////////////////////////////////////////////
// Mesh Cache
// ..................................................................................
// This really sucks, and will likely get replaced down the line... Let me try and
// explain my thought process here. So per room, we had a mesh cache that all of the
// brush geometry was loaded into, that worked great, but then we wanted to move some
// brushes around at runtime which is when we hit a slight problem; I can't update
// just *part* of the room mesh (as we don't have an API for doing that) and we need
// to handle the transforms correctly when drawing it too. Someone much wiser than I
// can probably come up with a better solution here, but for now, this abomination
// will be here...
/////////////////////////////////////////////////////////////////////////////////////

void ape_world_node_mark_dirty_( ApeWorldNode *self )
{
	self->isMeshDirty = true;
}

PLGMesh *ape_world_node_get_mesh_( ApeWorldNode *self )
{
	return self->mesh;
}

/**
 * Fetches the total number of vertices for just this level of the tree.
 * @param node World node instance.
 * @return Total number of vertices for just this level of the tree.
 */
static unsigned int get_total_verts_for_layer( ApeWorldNode *node )
{
	unsigned int numVertices = 0;
	if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		const ApeBrush *brush = ( ApeBrush * ) node;
		numVertices           = brush->numVertices;
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		if ( child->type != APE_WORLD_NODE_TYPE_BRUSH )
		{
			continue;
		}

		const ApeBrush *brush = ( ApeBrush * ) child;
		numVertices += brush->numVertices;
	}

	return numVertices;
}

void ape_world_node_update_mesh_cache_( ApeWorldNode *self )
{
	if ( !self->isMeshDirty )
	{
		return;
	}

	unsigned int numVertices = get_total_verts_for_layer( self );
	if ( numVertices == 0 )
	{
		PlgDestroyMesh( self->mesh );
		return;
	}

	if ( self->mesh == nullptr )
	{
		self->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_FAN, PLG_DRAW_STATIC, 0, numVertices );
		if ( self->mesh == nullptr )
		{
			ape_console_warning_( "Failed to create mesh for node: %s\n", PlGetError() );
			return;
		}
	}

	COM_PROFILE_FUNCTION_START();

	PlgClearMesh( self->mesh );

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->children, i )
	{
		if ( child->type != APE_WORLD_NODE_TYPE_BRUSH )
		{
			continue;
		}

		const ApeBrush *brush = ( ApeBrush * ) child;
		for ( unsigned int j = 0; j < brush->numFaces; ++j )
		{
			const ApeBrushFace *face = &brush->faces[ j ];
			for ( unsigned int k = 0; k < face->numVertices; ++k )
			{
				const ApeBrushFaceVertex *vertex = &face->vertices[ face->edgeLoopOrder[ k ] ];

#if !defined( APE_NO_EDITOR )

				// this is a gross botch to allow us to do special shaded
				// types via the editor... *sigh*
				QmMathColour4ub          colour;
				const ApeEditorInstance *editorInstance = ape_editor_get_active_instance();
				if ( editorInstance != nullptr && editorInstance->camera != nullptr )
				{
					const ApeCamera *camera = editorInstance->camera;
					if ( camera->drawMode == APE_CAMERA_DRAW_MODE_SOLID )
					{
						unsigned int seed = ( unsigned int ) ( uintptr_t ) brush;

						colour = QM_MATH_COLOUR4UB(
						        ( uint8_t ) ( qm_os_random_int( &seed ) % 256 ),
						        ( uint8_t ) ( qm_os_random_int( &seed ) % 256 ),
						        ( uint8_t ) ( qm_os_random_int( &seed ) % 256 ), 255 );
					}
					else if ( camera->drawMode == APE_CAMERA_DRAW_MODE_PORTALS )
					{
					}
					else
					{
						colour = QM_MATH_COLOUR4UB( 255, 255, 255, 255 );
					}
				}
				else
				{
					colour = QM_MATH_COLOUR4UB( 255, 255, 255, 255 );
				}

#else

				QmMathColour4ub colour = QM_MATH_COLOUR4UB( 255, 255, 255, 255 );

#endif

				const unsigned int idx = PlgAddMeshVertex( self->mesh, &brush->vertices[ vertex->posIndex ], &vertex->normal, &colour, &vertex->textureCoords );

				// these have to be set seperate for now, need an api for it
				self->mesh->vertices[ idx ].tangent   = face->tangent;
				self->mesh->vertices[ idx ].bitangent = face->bitangent;
				self->mesh->vertices[ idx ].st[ 1 ]   = vertex->lightmapCoords;
			}
		}
	}

	PlgUploadMesh( self->mesh );
	self->isMeshDirty = false;

	COM_PROFILE_FUNCTION_END();
}
