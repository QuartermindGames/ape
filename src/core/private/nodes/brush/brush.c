// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Implementation of the world building blocks - brushes.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "world/world.h"
#include "client/renderer/material/material.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static PLHashTable *brushClassesLookup;
static PLVectorArray *brushClasses;

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_brushes_()
{
	brushClassesLookup = PlCreateHashTable();
	if ( brushClassesLookup == nullptr )
	{
		ape_error_( true, "Failed to create brush classes lookup table: %s\n", PlGetError() );
	}

	brushClasses = PlCreateVectorArray( 0 );
	if ( brushClasses == nullptr )
	{
		ape_error_( true, "Failed to create brush classes vector: %s\n", PlGetError() );
	}

	extern ApeBrushClass ape_polyBrushClass;
	ape_register_brush_class( &ape_polyBrushClass );
}

void ape_shutdown_brushes_()
{
	PlDestroyVectorArray( brushClasses );
	brushClasses = nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////
// Brush Classes

const ApeBrushClass **ape_get_available_brush_classes( unsigned int *numClasses )
{
	return ( const ApeBrushClass ** ) PlGetVectorArrayDataEx( brushClasses, numClasses );
}

void ape_register_brush_class( const ApeBrushClass *classPtr )
{
	if ( PlLookupHashTableUserData( brushClassesLookup, classPtr->name, strlen( classPtr->name ) ) != nullptr )
	{
		ape_warning_( "Brush class (%s) already registered!\n", classPtr->name );
		return;
	}

	PlInsertHashTableNode( brushClassesLookup, classPtr->name, strlen( classPtr->name ), ( ApeBrushClass * ) classPtr );
	PlPushBackVectorArrayElement( brushClasses, ( ApeBrushClass * ) classPtr );

	if ( classPtr->registerFunction )
	{
		classPtr->registerFunction();
	}
}

/////////////////////////////////////////////////////////////////////////////////////

ApeBrush *ape_create_brush( ApeWorldNode *parent, const char *className, const PLVector3 *position, const PLVector3 *angles )
{
	const ApeBrushClass *brushClass = PlLookupHashTableUserData( brushClassesLookup, className, strlen( className ) );
	if ( brushClass == nullptr )
	{
		ape_warning_( "Invalid brush class specified (%s)!\n", className );
		return nullptr;
	}

	ApeBrush *brush = PL_NEW( ApeBrush );
	ape_world_node_create( parent, APE_WORLD_NODE_TYPE_BRUSH, position, angles, brush );

	brush->classPtr = brushClass;
	if ( brush->classPtr->createFunction )
	{
		brush->user = brush->classPtr->createFunction();
	}

	return brush;
}

void ape_brush_destroy_( void *data )
{
	ApeBrush *self = ( ApeBrush * ) data;
	if ( self == nullptr )
	{
		return;
	}

	if ( self->classPtr->destroyFunction != nullptr )
	{
		self->classPtr->destroyFunction( self->user );
		self->user = nullptr;
	}

	PL_DELETE( self );
}

void ape_brush_draw( ApeBrush *self )
{
	if ( self->classPtr->drawFunction != nullptr )
	{
		self->classPtr->drawFunction( self );
	}
}

/////////////////////////////////////////////////////////////////////////////////////

bool ape_world_face_is_backface( const ApeWorldFace *self, const ApeCamera *camera )
{
	PLVector3 angles = ape_camera_get_angles( camera );
	PLVector3 forward;
	PlAnglesAxes( angles, nullptr, nullptr, &forward );

	PLVector3 cameraPos = ape_camera_get_position( camera );
	if ( PlVector3DotProduct( self->normal, forward ) >= 0 )
	{
		return true;
	}

	return false;
}

bool ape_world_face_is_mirror( const ApeWorldFace *self )
{
	unsigned int flags = ape_material_get_flags( self->material );
	if ( flags & APE_MATERIAL_FLAG_MIRROR )
	{
		return true;
	}

	return ( self->flags & APE_WORLD_FACE_FLAG_MIRRORED );
}

bool ape_world_face_is_portal( const ApeWorldFace *self )
{
	return ( ape_world_face_is_mirror( self ) || ( self->portal != NULL ) );
}
