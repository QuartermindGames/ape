// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Lightmapper
// Author:  Mark E. Sowden

//#define LIGHTMAPPER
#ifdef LIGHTMAPPER

#	include "../ape_private.h"
#	include "../world/world.h"

static PLLinkedList *lightList;

void editor_light_initialize_()
{
	lightList = PlCreateLinkedList();
	if ( lightList == nullptr )
	{
		ape_error_( true, "Failed to create light list for lightmapper!\n" );
	}
}

void editor_light_shutdown_()
{
	PlDestroyLinkedList( lightList );
}

void editor_light_build_light_list_( ApeRoom *room )
{

	ApeWorldNode *worldNode;
	COM_ITERATE_LINKED_LIST( worldNode, room->base.children, i )
	{
		if ( worldNode->type == APE_WORLD_NODE_TYPE_LIGHT )
		{
			ApeLight *light = ( ApeLight * ) worldNode;
			if ( ape_light_get_shadow_type( light ) != SS_APE_LIGHT_SHADOW_TYPE_STATIC )
			{
				continue;
			}

			PlInsertLinkedListNode( lightList, light );
		}
	}
}

void editor_light_generate_( ApeRoom *room )
{
	PlDestroyLinkedListNodes( lightList );

	editor_light_build_light_list_( room );

	ApeLight *light;
	COM_ITERATE_LINKED_LIST( light, lightList, i )
	{
	}
}

#endif
