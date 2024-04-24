// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Implementation of the world building blocks - brushes.
// Author:  Mark E. Sowden

#include "world.h"

#include "client/renderer/material/material.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeBrush *ape_brush_create( ApeBrushType type, ApeBrushGeometryType geometryType )
{
	ApeBrush *brush = PL_NEW( ApeBrush );
	brush->faces = PlCreateLinkedList();
	brush->type = type;
	brush->geometryType = geometryType;

	return brush;
}

void ape_brush_destroy( ApeBrush *brush )
{
	if ( brush == NULL )
	{
		return;
	}

	if ( brush->faces != NULL )
	{
		PlDestroyLinkedListEx( brush->faces, pl_free );
	}

	PL_DELETE( brush );
}

void ape_world_brush_draw_( const ApeBrush *brush )
{
}

/////////////////////////////////////////////////////////////////////////////////////

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
