// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Lightmapper
// Author:  Mark E. Sowden

#define LIGHTMAPPER
#ifdef LIGHTMAPPER

#	include "ape_private.h"

#	include "world/world.h"
#	include "renderer/renderer.h"

/**
 * Some thoughts...
 *
 *	Lightmap per light. This will result in multiple passes, but will allow us to do
 *	specular etc.? Switching lights on the fly, or recomputing lightmaps at runtime should be cheaper...?
 *	Our biggest overhead right now are stencil shadows, though we're not caching so, go figure
 *
 *	Consider moving this into the cook tool?
 *	Should the cook tool be turned into a library?
 */

static constexpr char LIGHTMAP_EXTENSION[] = ".lmp";

static void generate_lightmap_( ApeLight *light )
{
	if ( light->lightmap == nullptr )
	{
		light->lightmap = PL_NEW_( ApeLightmapPixel, APE_LIGHTMAP_SIZE * APE_LIGHTMAP_SIZE );
	}
	else
	{
		PL_ZERO( light->lightmap, sizeof( ApeLightmapPixel ) * APE_LIGHTMAP_SIZE * APE_LIGHTMAP_SIZE );
	}

	float step = 1.0f / ( float ) APE_LIGHTMAP_SIZE;
	for ( unsigned int i = 0; i < APE_LIGHTMAP_SIZE; ++i )
	{
		for ( unsigned int j = 0; j < APE_LIGHTMAP_SIZE; ++j )
		{
		}
	}
}

static void gather_lights_( ApeWorldNode *node, PLLinkedList *lights )
{
	if ( node->type == APE_WORLD_NODE_TYPE_LIGHT )
	{
		ApeLight *light = ( ApeLight * ) node;
		//TODO: for now, for the sake of testing, we're ignoring this
		//if ( !( light->flags & APE_LIGHT_FLAG_DYNAMIC ) )
		{
			PlInsertLinkedListNode( lights, light );
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		gather_lights_( child, lights );
	}
}

void ape_editor_light_generate_( ApeRoom *room )
{
	ape_print_( "Generating lightmap...\n" );

	double startTime = PlGetCurrentSeconds();

	// first, gather all the lights for the given room we need to operate on

	PLLinkedList *lights = PlCreateLinkedList();
	if ( lights == nullptr )
	{
		ape_error_( true, "Failed to create lights list: %s\n", PlGetError() );
	}

	gather_lights_( APE_WORLD_NODE( room ), lights );

	// now, generate the lightmap for each light
	ApeLight *light;
	COM_ITERATE_LINKED_LIST( light, lights, i )
	{
		generate_lightmap_( light );
	}

	// cleanup
	PlDestroyLinkedList( lights );

	double endTime = PlGetCurrentSeconds();
	ape_print_( "Lightmap generation took %.3f seconds.\n", endTime - startTime );
}

#endif
