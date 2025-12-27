// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Lightmapper
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_time.h"

#include "ape_private.h"

#include "common/public/aux_texture_packer.h"

#include "game/game_public.h"

#include "world/world.h"
#include "renderer/renderer.h"

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

static constexpr unsigned int LIGHT_MAX_TEXTURE_WIDTH  = 512;
static constexpr unsigned int LIGHT_MAX_TEXTURE_HEIGHT = 512;

static constexpr char LIGHTMAP_EXTENSION[] = ".lmp";

static AuxTexturePackerNode *lightmapCache;

static void generate_lightmap_( ApeLight *light )
{
	if ( light->lightmap == nullptr )
	{
		light->lightmap = QM_OS_MEMORY_NEW_( ApeLightmapPixel, APE_LIGHTMAP_SIZE * APE_LIGHTMAP_SIZE );
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
	ape_console_print_( "Generating lightmap...\n" );

	double startTime = qm_os_time_get_seconds();

	// first, gather all the lights for the given room we need to operate on

	PLLinkedList *lights = PlCreateLinkedList();
	if ( lights == nullptr )
	{
		ape_console_error_( true, "Failed to create lights list: %s\n", PlGetError() );
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

	double endTime = qm_os_time_get_seconds();
	ape_console_print_( "Lightmap generation took %.3f seconds.\n", endTime - startTime );
}

void ape_light_command_( unsigned int, char ** )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr || instance->camera == nullptr )
	{
		ape_console_warning_( "Unable to generate lightmap, invalid editor instance!\n" );
		return;
	}

	ApeRoom *room = ape_camera_get_room( instance->camera );
	if ( room == nullptr )
	{
		ape_console_warning_( "Unable to generate lightmap, no valid camera!\n" );
		return;
	}

	ape_editor_light_generate_( room );
}
