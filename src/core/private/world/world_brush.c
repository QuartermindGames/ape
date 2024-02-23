// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Implementation of the world building blocks - brushes.
// Author:  Mark E. Sowden

#include "world.h"

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
