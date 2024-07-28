// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_filesystem.h>
#include <plgraphics/plg_mesh.h>

#include "ape_private.h"
#include "world.h"
#include "client/renderer/renderer.h"
#include "game/game_public.h"
#include "yin/core_game.h"

void ape_world_set_global_defaults( ApeWorld *level )
{
	level->ambience    = WORLD_DEFAULT_AMBIENCE;
	level->clearColour = WORLD_DEFAULT_CLEARCOLOUR;
}

void ape_world_set_ambience( ApeWorld *world, const PLColourF32 *ambience ) { world->ambience = *ambience; }
void ape_world_set_clear_colour( ApeWorld *world, const PLColourF32 *colour ) { world->clearColour = *colour; }
void ape_world_set_fog_colour( ApeWorld *world, const PLColourF32 *colour ) { world->fogColour = *colour; }

ApeWorld *ape_create_world( void )
{
	ApeWorld *world = PL_NEW( ApeWorld );
	ape_world_node_setup_( &world->base, nullptr, APE_WORLD_NODE_TYPE_ROOT, &pl_vecOrigin3, &pl_vecOrigin3 );

	world->globalProperties = acm_branch_push_back_object( nullptr, "properties" );
	acm_branch_push_back_float32_array( world->globalProperties, "ambience", ( const float * ) &WORLD_DEFAULT_AMBIENCE, 4 );
	acm_branch_push_back_float32_array( world->globalProperties, "clearColour", ( const float * ) &WORLD_DEFAULT_CLEARCOLOUR, 4 );

	world->meshes       = PlCreateVectorArray( 0 );
	world->entitySpawns = PlCreateLinkedList();

	ape_world_set_global_defaults( world );

	ape_gameInterface->requestCallbackMethod( APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD, world );

	return world;
}

static unsigned int get_total_verts_for_room( ApeRoom *room, bool detail )
{
	// determine the total number of vertices

	unsigned int numVerts = 0;
	for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->faces ); ++j )
	{
		ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
		assert( face != nullptr );
		if ( face == nullptr )
		{
			continue;
		}

		numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
	}

	if ( detail )
	{
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
		{
			ApeRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
			assert( detailRoom != nullptr );
			if ( detailRoom == nullptr )
				continue;

			for ( unsigned int k = 0; k < PlGetNumVectorArrayElements( detailRoom->faces ); ++k )
			{
				ApeWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
				assert( face != nullptr );
				if ( face == nullptr )
					continue;

				numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
			}
		}
	}

	return numVerts;
}

static unsigned int get_total_faces_for_room( ApeRoom *room, bool detail )
{
	unsigned int numFaces = PlGetNumVectorArrayElements( room->faces );

	if ( detail )
	{
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
		{
			ApeRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
			assert( detailRoom != nullptr );
			if ( detailRoom == nullptr )
			{
				continue;
			}

			numFaces += PlGetNumVectorArrayElements( detailRoom->faces );
		}
	}

	return numFaces;
}

static void cache_room_mesh( const ApeWorld *world, ApeRoom *room )
{
	if ( room->mesh == nullptr )
	{
		room->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_FAN, PLG_DRAW_STATIC, 0, get_total_verts_for_room( room, true ) );
		if ( room->mesh == nullptr )
		{
			ape_warning_( "Failed to create mesh for room: %s\n", PlGetError() );
			return;
		}
	}

	PlgClearMesh( room->mesh );

	unsigned int numFaces = get_total_faces_for_room( room, false );
	for ( unsigned int j = 0; j < numFaces; ++j )
	{
		ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
		assert( face != nullptr );
		if ( face->materialIndex < 0 )
		{
			continue;
		}

		PLColour faceColour = {
		        .r = ( rand() % 200 ) + 55,
		        .g = ( rand() % 200 ) + 55,
		        .b = ( rand() % 200 ) + 55,
		        .a = 255,
		};

		PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
		while ( faceVertexNode != nullptr )
		{
			ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
			assert( vertex->u != nullptr );

			PlgAddMeshVertex( room->mesh,
			                  &vertex->u->position,
			                  &vertex->normal,
			                  &faceColour,
			                  &vertex->uv );

			faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
		}
	}

	for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
	{
		ApeRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
		assert( detailRoom != nullptr );

		numFaces = PlGetNumVectorArrayElements( detailRoom->faces );
		for ( unsigned int k = 0; k < numFaces; ++k )
		{
			ApeWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
			assert( face != nullptr );
			if ( face->materialIndex < 0 )
			{
				continue;
			}

			PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
			while ( faceVertexNode != nullptr )
			{
				ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
				assert( vertex->u != nullptr );

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
	AcmBranch *root = acm_load_file( path, "world" );
	if ( root == nullptr )
	{
		ape_warning_( "Failed to load world: %s\n", acm_get_error_message() );
		return nullptr;
	}

	ApeWorld *world = ape_world_deserialize_( root );
	if ( world == nullptr )
	{
		ape_warning_( "Failed to load level (%s)!\n", path );
	}

	acm_branch_destroy( root );

	if ( world != nullptr )
	{
		// Create cached room geometry
		for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			ApeRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			assert( room != nullptr );
			if ( room->isDetail || room->isMeshCached )
			{
				continue;
			}

			cache_room_mesh( world, room );
		}
	}

	return world;
}

bool ape_world_save( ApeWorld *self, const char *path )
{
	AcmBranch *root = acm_branch_push_back_object( nullptr, "world" );

	ape_world_serialize_( self, root );
	snprintf( self->path, sizeof( self->path ), "%s", path );

	if ( !acm_write_file( path, root, ND_FILE_BINARY ) )
	{
		ape_warning_( "Failed to save world (%s): %s\n", path, acm_get_error_message() );
		return false;
	}

	return true;
}

void ape_world_destroy_( void *data )
{
	ApeWorld *self = ( ApeWorld * ) data;
	if ( self == nullptr )
	{
		return;
	}

	ape_clear_camera_visibility_lists_();

	if ( self->materials != nullptr )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( self->materials ); ++i )
		{
			ApeMaterial *material = PlGetVectorArrayElementAt( self->materials, i );
			if ( material == nullptr )
			{
				continue;
			}

			ape_material_release( material );
			material = nullptr;
		}
		PlDestroyVectorArray( self->materials );
		self->materials = nullptr;
	}

	PlDestroyVectorArray( self->rooms );
	self->rooms = nullptr;

	if ( self->vertices != nullptr )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( self->vertices ); ++i )
		{
			ApeWorldVertex *vertex = PlGetVectorArrayElementAt( self->vertices, i );
			if ( vertex == nullptr )
				continue;

			PlDestroyVectorArray( vertex->adjacentFaces );
			PL_DELETE( vertex );
		}
		PlDestroyVectorArray( self->vertices );
		self->vertices = nullptr;
	}
}

void ape_world_spawn_entities_( ApeWorld *world )
{
	PLLinkedListNode *node = PlGetFirstNode( world->entitySpawns );
	while ( node != nullptr )
	{
		ApeWorldEntity *worldEntity = ( ApeWorldEntity * ) PlGetLinkedListNodeUserData( node );
		ape_create_entity( worldEntity->className, worldEntity->properties, nullptr );
		node = PlGetNextLinkedListNode( node );
	}
}

/****************************************
 * Global World Properties
 ****************************************/

AcmBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName )
{
	if ( world->globalProperties == nullptr )
	{
		return nullptr;
	}

	return acm_branch_get_child_by_name( world->globalProperties, propertyName );
}

/****************************************
 * SECTOR
 ****************************************/

/**
 * This crudely tries to determine the sector by an origin point.
 * Should only be used for vague, but fast, lookup.
 */
ApeRoom *ape_world_get_room_at_position( ApeWorld *world, const PLVector3 *position )
{
	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
	{
		ApeRoom *room = ( ApeRoom * ) PlGetVectorArrayElementAt( world->rooms, i );
		if ( !PlIsPointIntersectingAabb( &room->base.bounds, *position ) )
		{
			continue;
		}

		return room;
	}

	return nullptr;
}

void ape_register_world_console_variables_( void )
{
	PlRegisterConsoleVariable( "world.skipDraw", "Toggle rendering of world.", "false", PL_VAR_BOOL, &ape_config_.world.skipDraw, nullptr, false );
	PlRegisterConsoleVariable( "world.skipPortals", "Toggle display of rooms visible through portals.", "false", PL_VAR_BOOL, &ape_config_.world.skipPortals, nullptr, false );
	PlRegisterConsoleVariable( "world.showAllRooms", "Toggle rendering of all rooms.", "false", PL_VAR_BOOL, &ape_config_.world.showAllRooms, nullptr, false );
	PlRegisterConsoleVariable( "world.showRoomVolumes", "Toggle rendering of room volumes.", "false", PL_VAR_BOOL, &ape_config_.world.showRoomVolumes, nullptr, false );
	PlRegisterConsoleVariable( "world.showPortals", "Toggles the display of portals.", "false", PL_VAR_BOOL, &ape_config_.world.showPortals, nullptr, false );
	PlRegisterConsoleVariable( "world.sortLights", "Sort lights before drawing world.", "false", PL_VAR_BOOL, &ape_config_.world.sortLights, nullptr, false );
	PlRegisterConsoleVariable( "world.showNodeVolumes", "Shows bounding volumes of all nodes currently in the scene.", "false", PL_VAR_BOOL, &ape_config_.world.showNodeVolumes, nullptr, false );
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_initialize_world_()
{
}

void ape_shutdown_world_()
{
}

ApeRoom *ape_world_get_first_room_( ApeWorld *self )
{
	PLLinkedListNode *childNode = PlGetFirstNode( self->base.children );
	while ( childNode != nullptr )
	{
		ApeWorldNode *worldNode = PlGetLinkedListNodeUserData( childNode );
		if ( worldNode->type != APE_WORLD_NODE_TYPE_ROOM )
		{
			childNode = PlGetNextLinkedListNode( childNode );
			continue;
		}

		return ( ApeRoom * ) worldNode;
	}

	return nullptr;
}
