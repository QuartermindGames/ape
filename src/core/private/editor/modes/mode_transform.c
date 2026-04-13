// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Transform Mode
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "editor/editor.h"
#include "renderer/renderer.h"

void ape_editor_mode_transform_attach_to( const ApeEditorInstance *instance, ApeWorldNode *parent )
{
	if ( instance->geometryMode != APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
	{
		return;
	}

	ApeWorldNode *node;
	QM_OS_LINKED_LIST_ITERATE( node, instance->selectedObjects, i )
	{
		// attach has protection against this, but will throw a warning
		if ( node == parent )
		{
			continue;
		}

		ape_world_node_attach( node, parent );
	}
}

void ape_editor_mode_transform_pre_render_( const ApeEditorInstance *instance )
{
	ApeWorldNode *node;
	QM_OS_LINKED_LIST_ITERATE( node, instance->selectedObjects, i )
	{
		ApeWorldNode *parent = node->parent;
		if ( parent == nullptr || parent->type == APE_WORLD_NODE_TYPE_ROOM )
		{
			continue;
		}

		QmMathVector3f nodePos   = ape_world_node_get_position( node );
		QmMathVector3f parentPos = ape_world_node_get_position( parent );

		ape_draw_debug_arrow( nodePos, parentPos, PL_COLOUR_LIGHT_GREEN, 2.0f );
	}
}
