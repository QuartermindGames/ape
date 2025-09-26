// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
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

const ApeWorldNodeProperty *ape_world_node_get_properties( unsigned int *numProperties )
{
	static const ApeWorldNodeProperty properties[] = {
	        APE_WORLD_NODE_PROPERTY_STRING( "Name", "Name of the node.", ApeWorldNode, name ),
	        APE_WORLD_NODE_PROPERTY_BASIC( "Position", "Position of the node in 3D space.", ApeWorldNode, position, VEC3 ),
	        APE_WORLD_NODE_PROPERTY_BASIC( "Angles", "Angles of the node in 3D space.", ApeWorldNode, angles, VEC3 ),
	        APE_WORLD_NODE_PROPERTY_BASIC( "Scale", "Scale of the node in 3D space.", ApeWorldNode, scale, VEC3 ),
	};
	*numProperties = QM_OS_ARRAY_ELEMENTS( properties );

	return properties;
}

const ApeWorldNodeProperty *ape_world_node_get_class_properties( unsigned int *numProperties, ApeWorldNodeType type )
{
	const ApeWorldNodeClass *nodeClass = nodeClasses[ type ];
	*numProperties                     = nodeClass->numProperties;
	return nodeClass->properties;
}

void *ape_world_node_get_property_pointer( ApeWorldNode *self, const ApeWorldNodeProperty *property )
{
	return ( char * ) self + property->offset;
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_world_node_compute_bounds_( ApeWorldNode *self )
{
	self->bounds.maxs = qm_math_vector3f( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	self->bounds.mins = qm_math_vector3f( FLT_MAX, FLT_MAX, FLT_MAX );

	for ( unsigned int i = 0; i < 3; ++i )
	{
		PLCollisionAABB localBounds = ape_world_node_get_transformed_local_bounds( self );
		localBounds.mins            = qm_math_vector3f_add( localBounds.mins, localBounds.origin );
		localBounds.maxs            = qm_math_vector3f_add( localBounds.maxs, localBounds.origin );

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
		ape_world_node_compute_bounds_( child );
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
		char tmp[ 64 ];
		qm_math_vector3f_print( self->position, tmp, sizeof( tmp ) );

		ape_warning_( "Encountered a node (%s) without an associated room!\n", tmp );
		return nullptr;
	}

	return ( ApeRoom * ) roomNode;
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

	self->type      = type;
	self->classType = nodeClasses[ self->type ];

	if ( parent != nullptr )
	{
		ape_world_node_attach( self, parent );
	}

	update_world_transform( self );

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

	update_world_transform( self );
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

	update_world_transform( self );
}

void ape_world_node_set_local_bounds( ApeWorldNode *self, const QmMathVector3f *mins, const QmMathVector3f *maxs )
{
	assert( ape_world_node_is_valid( self, self->type ) );

	self->localBounds.mins = *mins;
	self->localBounds.maxs = *maxs;

	// need to go ahead and recalc bounds

	self->bounds = self->localBounds;

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->children, i )
	{
		//TODO: THIS SHOULD BE ACCOUNTING FOR TRANSFORMS YOU DUMB BASTARD
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
	PLMatrix4 transform = PlMatrix4Identity();

	PLMatrix4 translate = PlTranslateMatrix4( self->position );
	transform           = PlMultiplyMatrix4( &transform, &translate );

	PLMatrix4 rotate;
	rotate    = PlRotateMatrix4( PL_DEG2RAD( self->angles.x ), &QM_MATH_VECTOR3F( 1.0f, 0.0f, 0.0f ) );
	transform = PlMultiplyMatrix4( &transform, &rotate );
	rotate    = PlRotateMatrix4( PL_DEG2RAD( self->angles.y ), &QM_MATH_VECTOR3F( 0.0f, 1.0f, 0.0f ) );
	transform = PlMultiplyMatrix4( &transform, &rotate );
	rotate    = PlRotateMatrix4( PL_DEG2RAD( self->angles.z ), &QM_MATH_VECTOR3F( 0.0f, 0.0f, 1.0f ) );
	transform = PlMultiplyMatrix4( &transform, &rotate );

	return transform;
}

AcmBranch *ape_world_node_serialize( ApeWorldNode *self, AcmBranch *root )
{
	assert( self->classType != nullptr );

	if ( self->flags & APE_WORLD_NODE_FLAG_DISCARD )
	{
		// not a type that's saved
		return nullptr;
	}

	AcmBranch *nodeBranch = acm_push_object( root, "node" );

	if ( *self->name != '\0' )
	{
		acm_push_string( nodeBranch, "name", self->name, false );
	}

	com_acm_push_vector3( nodeBranch, "position", &self->position, true );
	com_acm_push_vector3( nodeBranch, "angles", &self->angles, true );
	com_acm_push_vector3( nodeBranch, "scale", &self->scale, true );

	acm_push_array_f32( nodeBranch, "localTransform", ( float * ) &self->localTransform, 16 );

	//	acm_push_array_f32( nodeBranch, "localBounds", ( float * ) &self->localBounds, 12 );
	//	acm_push_array_f32( nodeBranch, "bounds", ( float * ) &self->bounds, 12 );

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
	if ( worldNodeClass->deserialize == nullptr )
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

	ApeWorldNode *self = worldNodeClass->deserialize( parent, classBranch );
	if ( self == nullptr )
	{
		ape_warning_( "Failed to deserialize world node!\n" );
		return nullptr;
	}

	snprintf( self->name, sizeof( self->name ), "%s", acm_get_string( root, "name", "" ) );

	self->position = com_acm_get_vector3( root, "position", &pl_vecOrigin3 );
	self->angles   = com_acm_get_vector3( root, "angles", &pl_vecOrigin3 );
	self->scale    = com_acm_get_vector3( root, "scale", &pl_vecOrigin3 );

	acm_get_array_f32( root, "localTransform", ( float * ) &self->localTransform, 16 );
	//	acm_get_array_f32( root, "localBounds", ( float * ) &self->localBounds, 12 );
	//	acm_get_array_f32( root, "bounds", ( float * ) &self->bounds, 12 );

	self->flags = acm_get_uint( root, "flags", 0 );

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

	ape_world_node_compute_bounds_( self );

	update_world_transform( self );

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
		ape_warning_( "Failed to gather children: %s\n", PlGetError() );
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
		ape_warning_( "Failed to load the specified node (%s): %s\n", path, acm_get_error_message() );
		return nullptr;
	}

	ApeWorldNode *worldNode = ape_world_node_deserialize( parent, branch );
	if ( worldNode != nullptr )
	{
		PlSetupPath( worldNode->path, true, "%s", path );
	}
	else
	{
		ape_warning_( "Failed to deserialize node (%s)!\n", path );
	}

	acm_branch_destroy( branch );

	return worldNode;
}

const char *ape_world_node_get_path( const ApeWorldNode *self )
{
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

static unsigned int get_total_verts_for_tree( ApeWorldNode *node )
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
		numVertices += get_total_verts_for_tree( child );
	}

	return numVertices;
}

void ape_world_node_update_mesh_cache_( ApeWorldNode *self )
{
	if ( !self->isMeshDirty )
	{
		return;
	}

	unsigned int numVertices = get_total_verts_for_tree( self );
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
			ape_warning_( "Failed to create mesh for node: %s\n", PlGetError() );
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
				const ApeBrushFaceVertex *vertex = face->edgeLoop[ k ];

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

				const unsigned int idx = PlgAddMeshVertex( self->mesh, vertex->position, &vertex->normal, &colour, &vertex->textureCoords );

				// these have to be set seperate for now, need an api for it
				self->mesh->vertices[ idx ].tangent   = face->tangent;
				self->mesh->vertices[ idx ].bitangent = face->bitangent;
			}
		}
	}

	PlgUploadMesh( self->mesh );
	self->isMeshDirty = false;

	COM_PROFILE_FUNCTION_END();
}
