// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_filesystem.h>
#include <plgraphics/plg_mesh.h>

#include "ape_private.h"
#include "world.h"
#include "client/renderer/renderer.h"
#include "game/game_interface.h"
#include "editor/editor.h"

void ape_world_set_global_defaults( ApeWorld *level )
{
	level->ambience = WORLD_DEFAULT_AMBIENCE;
	level->clearColour = WORLD_DEFAULT_CLEARCOLOUR;
}

void ape_world_set_ambience( ApeWorld *world, const PLColourF32 *ambience ) { world->ambience = *ambience; }
void ape_world_set_clear_colour( ApeWorld *world, const PLColourF32 *colour ) { world->clearColour = *colour; }
void ape_world_set_fog_colour( ApeWorld *world, const PLColourF32 *colour ) { world->fogColour = *colour; }

ApeWorld *ape_world_create( void )
{
	ApeWorld *world = PL_NEW( ApeWorld );

	ape_world_node_setup_header( &world->header, APE_WORLD_NODE_TYPE_ROOT );

	world->globalProperties = nd_branch_push_back_object( NULL, "properties" );
	nd_branch_push_back_float32_array( world->globalProperties, "ambience", ( const float * ) &WORLD_DEFAULT_AMBIENCE, 4 );
	nd_branch_push_back_float32_array( world->globalProperties, "clearColour", ( const float * ) &WORLD_DEFAULT_CLEARCOLOUR, 4 );

	world->meshes = PlCreateVectorArray( 0 );
	world->entities = PlCreateVectorArray( 0 );
	world->lights = PlCreateVectorArray( 0 );
	world->entitySpawns = PlCreateLinkedList();

	world->root = ape_world_node_create( NULL, "root", APE_WORLD_NODE_TYPE_ROOT, world );

	ape_world_set_global_defaults( world );

	return world;
}

static unsigned int get_total_verts_for_room( ApeWorldRoom *room, bool detail )
{
	// determine the total number of vertices

	unsigned int numVerts = 0;
	for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->faces ); ++j )
	{
		ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
		assert( face != NULL );
		if ( face == NULL )
		{
			continue;
		}

		numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
	}

	if ( detail )
	{
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
		{
			ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
			assert( detailRoom != NULL );
			if ( detailRoom == NULL )
				continue;

			for ( unsigned int k = 0; k < PlGetNumVectorArrayElements( detailRoom->faces ); ++k )
			{
				ApeWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
				assert( face != NULL );
				if ( face == NULL )
					continue;

				numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
			}
		}
	}

	return numVerts;
}

static unsigned int get_total_faces_for_room( ApeWorldRoom *room, bool detail )
{
	unsigned int numFaces = PlGetNumVectorArrayElements( room->faces );

	if ( detail )
	{
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
		{
			ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
			assert( detailRoom != NULL );
			if ( detailRoom == NULL )
			{
				continue;
			}

			numFaces += PlGetNumVectorArrayElements( detailRoom->faces );
		}
	}

	return numFaces;
}

static void cache_room_mesh( const ApeWorld *world, ApeWorldRoom *room )
{
	if ( room->mesh == NULL )
	{
		room->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_FAN, PLG_DRAW_STATIC, 0, get_total_verts_for_room( room, true ) );
		assert( room->mesh != NULL );
		if ( room->mesh == NULL )
		{
			PRINT_WARNING( "Failed to create mesh for room: %s\n", PlGetError() );
			return;
		}
	}

	PlgClearMesh( room->mesh );

	unsigned int numFaces = get_total_faces_for_room( room, false );
	for ( unsigned int j = 0; j < numFaces; ++j )
	{
		ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
		assert( face != NULL );
		if ( face == NULL || face->materialIndex < 0 )
			continue;

		PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
		while ( faceVertexNode != NULL )
		{
			ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
			assert( vertex->u != NULL );

			PLColour colour = PlColourF32ToU8( &room->colour );
			PlgAddMeshVertex( room->mesh,
			                  &vertex->u->position,
			                  &vertex->normal,
			                  &colour,
			                  &vertex->uv );

			faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
		}
	}

	for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
	{
		ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
		assert( detailRoom != NULL );
		if ( detailRoom == NULL )
			continue;

		numFaces = PlGetNumVectorArrayElements( detailRoom->faces );
		for ( unsigned int k = 0; k < numFaces; ++k )
		{
			ApeWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
			assert( face != NULL );
			if ( face == NULL || face->materialIndex < 0 )
				continue;

			PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
			while ( faceVertexNode != NULL )
			{
				ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
				assert( vertex->u != NULL );

				PLColour colour = PlColourF32ToU8( &room->colour );
				PlgAddMeshVertex( room->mesh,
				                  &vertex->u->position,
				                  &vertex->normal,
				                  &colour,
				                  &vertex->uv );

				faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
			}
		}
	}

	//PlgGenerateMeshNormals( room->mesh, true );
	PlgGenerateVertexTangentBasis( room->mesh->vertices, room->mesh->num_verts );

	PlgUploadMesh( room->mesh );

	room->isMeshCached = true;
}

ApeWorld *ape_world_load( const char *path )
{
	NdBranch *root = nd_load_file( path, "world" );
	if ( root == NULL )
	{
		PRINT_WARNING( "Failed to load world: %s\n", nd_get_error_message() );
		return NULL;
	}

	ApeWorld *world = ape_world_deserialize_( root );
	if ( world == NULL )
	{
		PRINT_WARNING( "Failed to load level (%s)!\n", path );
	}

	nd_branch_destroy( root );

	if ( world != NULL )
	{
		// Create cached room geometry
		for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			assert( room != NULL );
			if ( room == NULL || room->isDetail || room->isMeshCached )
				continue;

			cache_room_mesh( world, room );
		}
	}

	return world;
}

bool ape_world_save( ApeWorld *world, const char *path )
{
	world->lastSaveTime = time( NULL );

	NdBranch *root = nd_branch_push_back_object( NULL, "world" );

	ape_world_serialize_( world, root );
	snprintf( world->path, sizeof( world->path ), "%s", path );

	if ( !nd_write_file( path, root, ND_FILE_BINARY ) )
	{
		PRINT_WARNING( "Failed to save world (%s): %s\n", path, nd_get_error_message() );
		return false;
	}

	return true;
}

void ape_world_destroy( ApeWorld *level )
{
	if ( level == NULL )
		return;

	ape_clear_camera_visibility_lists_();

	if ( level->materials != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( level->materials ); ++i )
		{
			ApeMaterial *material = PlGetVectorArrayElementAt( level->materials, i );
			if ( material == NULL )
				continue;

			ss_arl_material_release( material );
			material = NULL;
		}
		PlDestroyVectorArray( level->materials );
		level->materials = NULL;
	}

	if ( level->rooms != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( level->rooms ); ++i )
		{
			ApeWorldRoom *room = PlGetVectorArrayElementAt( level->rooms, i );
			if ( room == NULL )
				continue;

			ape_world_room_destroy( room );
			room = NULL;
		}
		PlDestroyVectorArray( level->rooms );
		level->rooms = NULL;
	}

	PlDestroyVectorArrayEx( level->portals, PlFree );

	if ( level->vertices != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( level->vertices ); ++i )
		{
			ApeWorldVertex *vertex = PlGetVectorArrayElementAt( level->vertices, i );
			if ( vertex == NULL )
				continue;

			PlDestroyVectorArray( vertex->adjacentFaces );
			PL_DELETE( vertex );
		}
		PlDestroyVectorArray( level->vertices );
		level->vertices = NULL;
	}

	PlDestroyVectorArrayEx( level->lights, PlFree );
}

void ape_world_spawn_entities_( ApeWorld *world )
{
	PLLinkedListNode *node = PlGetFirstNode( world->entitySpawns );
	while ( node != NULL )
	{
		ApeWorldEntity *worldEntity = ( ApeWorldEntity * ) PlGetLinkedListNodeUserData( node );
		ape_entity_create( worldEntity->className, worldEntity->properties );
		node = PlGetNextLinkedListNode( node );
	}
}

void ape_world_attach_light( ApeWorld *world, ApeLight *light )
{
	assert( light->world == NULL );
	if ( light->world != NULL )
	{
		PRINT_WARNING( "Light is already associated with a world!\n" );
		return;
	}

	PlPushBackVectorArrayElement( world->lights, light );
	light->world = world;
}

void ape_world_attach_node( ApeWorld *self, ApeWorldNode *node )
{
	if ( node->parent != NULL )
	{
		PlDestroyLinkedListNode( node->parentListNode );
	}

	node->parent = self->root;
	node->parentListNode = PlInsertLinkedListNode( self->root->children, node );
}

/****************************************
 * Global World Properties
 ****************************************/

NdBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName )
{
	if ( world->globalProperties == NULL )
		return NULL;

	return nd_branch_get_child_by_name( world->globalProperties, propertyName );
}

/****************************************
 * SECTOR
 ****************************************/

/**
 * This crudely tries to determine the sector by an origin point.
 * Should only be used for vague lookup.
 */
ApeWorldRoom *ape_world_get_room_at_position( ApeWorld *world, const PLVector3 *position )
{
	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
	{
		ApeWorldRoom *room = ( ApeWorldRoom * ) PlGetVectorArrayElementAt( world->rooms, i );
		if ( !PlIsPointIntersectingAabb( &room->bounds, *position ) )
			continue;

		return room;
	}

	return NULL;
}

void ape_register_world_console_variables_( void )
{
	PlRegisterConsoleVariable( "world.skipDraw", "Toggle rendering of world.", "false", PL_VAR_BOOL, &ape_config_.world.skipDraw, NULL, false );
	PlRegisterConsoleVariable( "world.skipPortals", "Toggle display of rooms visible through portals.", "false", PL_VAR_BOOL, &ape_config_.world.skipPortals, NULL, false );
	PlRegisterConsoleVariable( "world.showAllRooms", "Toggle rendering of all rooms.", "false", PL_VAR_BOOL, &ape_config_.world.showAllRooms, NULL, false );
	PlRegisterConsoleVariable( "world.showRoomVolumes", "Toggle rendering of room volumes.", "false", PL_VAR_BOOL, &ape_config_.world.showRoomVolumes, NULL, false );
	PlRegisterConsoleVariable( "world.showPortals", "Toggles the display of portals.", "false", PL_VAR_BOOL, &ape_config_.world.showPortals, NULL, false );
	PlRegisterConsoleVariable( "world.sortLights", "Sort lights before drawing world.", "false", PL_VAR_BOOL, &ape_config_.world.sortLights, NULL, false );
}

void ape_tick_client_world_( void )
{
	ape_build_camera_visibility_lists_();
}
