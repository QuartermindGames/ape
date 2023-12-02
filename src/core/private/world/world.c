// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>
#include <plgraphics/plg_mesh.h>

#include "ape_private.h"
#include "world.h"
#include "client/renderer/renderer.h"
#include "game/game_interface.h"

void ss_acl_level_set_global_defaults( ApeWorld *level )
{
	level->ambience = WORLD_DEFAULT_AMBIENCE;
	level->clearColour = WORLD_DEFAULT_CLEARCOLOUR;
}

void ss_acl_level_set_ambience( ApeWorld *world, const PLColourF32 *ambience ) { world->ambience = *ambience; }
void ss_acl_level_set_clear_colour( ApeWorld *world, const PLColourF32 *colour ) { world->clearColour = *colour; }
void ss_acl_level_set_fog_colour( ApeWorld *world, const PLColourF32 *colour ) { world->fogColour = *colour; }

ApeWorld *ss_acl_level_create( void )
{
	ApeWorld *level = PL_NEW( ApeWorld );

	level->globalProperties = ndPushBackObject( NULL, "properties" );
	ndPushBackF32Array( level->globalProperties, "ambience", ( const float * ) &WORLD_DEFAULT_AMBIENCE, 4 );
	ndPushBackF32Array( level->globalProperties, "clearColour", ( const float * ) &WORLD_DEFAULT_CLEARCOLOUR, 4 );

	level->meshes = PlCreateVectorArray( 0 );
	level->entities = PlCreateVectorArray( 0 );
	level->lights = PlCreateVectorArray( 0 );
	level->entitySpawns = PlCreateLinkedList();

	ss_acl_level_set_global_defaults( level );

	return level;
}

static unsigned int get_total_verts_for_room( SSAclWorldRoom *room, bool detail )
{
	// determine the total number of vertices

	unsigned int numVerts = 0;
	for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->faces ); ++j )
	{
		SSAclWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
		assert( face != NULL );
		if ( face == NULL )
			continue;

		numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
	}

	if ( detail )
	{
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
		{
			SSAclWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
			assert( detailRoom != NULL );
			if ( detailRoom == NULL )
				continue;

			for ( unsigned int k = 0; k < PlGetNumVectorArrayElements( detailRoom->faces ); ++k )
			{
				SSAclWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
				assert( face != NULL );
				if ( face == NULL )
					continue;

				numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
			}
		}
	}

	return numVerts;
}

static unsigned int get_total_faces_for_room( SSAclWorldRoom *room, bool detail )
{
	unsigned int numFaces = PlGetNumVectorArrayElements( room->faces );

	if ( detail )
	{
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( room->detailRooms ); ++j )
		{
			SSAclWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
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

static void cache_room_mesh( const ApeWorld *world, SSAclWorldRoom *room )
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
		SSAclWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
		assert( face != NULL );
		if ( face == NULL || face->materialIndex < 0 )
			continue;

		PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
		while ( faceVertexNode != NULL )
		{
			SSAclWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
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
		SSAclWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, j );
		assert( detailRoom != NULL );
		if ( detailRoom == NULL )
			continue;

		numFaces = PlGetNumVectorArrayElements( detailRoom->faces );
		for ( unsigned int k = 0; k < numFaces; ++k )
		{
			SSAclWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
			assert( face != NULL );
			if ( face == NULL || face->materialIndex < 0 )
				continue;

			PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
			while ( faceVertexNode != NULL )
			{
				SSAclWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
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

ApeWorld *ss_acl_level_load( const char *path )
{
	const char *extension = PlGetFileExtension( path );
	if ( extension == NULL )
	{
		PRINT_WARNING( "Invalid world name (%s)!\n", path );
		return NULL;
	}

	ApeWorld *world;
	if ( pl_strcasecmp( extension, ".rfl" ) == 0 )
		world = ss_acl_world_load_rfl_file_( path );
	else
	{
		NdBranch *root = ndLoadFile( path, "world" );
		if ( root == NULL )
		{
			PRINT_WARNING( "Failed to load world: %s\n", ndGetErrorMessage() );
			return NULL;
		}

		world = ss_acl_world_deserialize_( root );
		if ( world == NULL )
			PRINT_WARNING( "Failed to load level (%s)!\n", path );

		ndDestroyBranch( root );
	}

	if ( world != NULL )
	{
		// Create cached room geometry
		for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			SSAclWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			assert( room != NULL );
			if ( room == NULL || room->isDetail || room->isMeshCached )
				continue;

			cache_room_mesh( world, room );
		}
	}

	return world;
}

bool ss_acl_world_save( ApeWorld *world, const char *path )
{
	world->lastSaveTime = time( NULL );

	NdBranch *root = ndPushBackObject( NULL, "world" );

	ss_acl_world_serialize_( world, root );
	snprintf( world->path, sizeof( world->path ), "%s", path );

	if ( !ndWriteFile( path, root, ND_FILE_BINARY ) )
	{
		PRINT_WARNING( "Failed to save world (%s): %s\n", path, ndGetErrorMessage() );
		return false;
	}

	return true;
}

void acl_level_destroy( ApeWorld *level )
{
	if ( level == NULL )
		return;

	ss_arl_clear_camera_visibility_lists_();

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
			SSAclWorldRoom *room = PlGetVectorArrayElementAt( level->rooms, i );
			if ( room == NULL )
				continue;

			ss_acl_room_destroy( room );
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

void ss_acl_world_spawn_entities_( ApeWorld *world )
{
	PLLinkedListNode *node = PlGetFirstNode( world->entitySpawns );
	while ( node != NULL )
	{
		ApeWorldEntity *worldEntity = ( ApeWorldEntity * ) PlGetLinkedListNodeUserData( node );
		ss_acl_entity_create( worldEntity->className, worldEntity->properties );
		node = PlGetNextLinkedListNode( node );
	}
}

void acl_level_attach_entity( ApeWorld *world, SS_Acl_Entity *entity )
{
	assert( entity->world == NULL );
	if ( entity->world != NULL )
	{
		PRINT_WARNING( "Entity is already associated with a world!\n" );
		return;
	}

	PlPushBackVectorArrayElement( world->entities, entity );
	entity->world = world;
}

void ape_level_attach_light( ApeWorld *world, SSArlLight *light )
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

/****************************************
 * Global World Properties
 ****************************************/

NdBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName )
{
	if ( world->globalProperties == NULL )
		return NULL;

	return ndGetChildByName( world->globalProperties, propertyName );
}

/****************************************
 * SECTOR
 ****************************************/

/**
 * This crudely tries to determine the sector by an origin point.
 * Should only be used for vague lookup.
 */
SSAclWorldRoom *ss_acl_level_get_room_at_position( ApeWorld *world, const PLVector3 *position )
{
	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
	{
		SSAclWorldRoom *room = ( SSAclWorldRoom * ) PlGetVectorArrayElementAt( world->rooms, i );
		if ( !PlIsPointIntersectingAabb( &room->bounds, *position ) )
			continue;

		return room;
	}

	return NULL;
}

static void level_save_command( unsigned int argc, char **argv )
{
	ApeWorld *world = ss_game_get_current_world();
	if ( world == NULL )
	{
		PRINT_WARNING( "No active world, can't save!\n" );
		return;
	}

	const char *dataPath = comGetDataDirectory();

	NdBranch *root = ndPushBackObject( NULL, "world" );

	ss_acl_world_serialize_( world, root );
}

void ss_acl_register_level_console_variables_( void )
{
	PlRegisterConsoleVariable( "skip_level_draw", "Toggle rendering of world.", "false", PL_VAR_BOOL, &ape_config_.level.skipDraw, NULL, false );
	PlRegisterConsoleVariable( "world/skipPortals", "Toggle display of rooms visible through portals.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showAllRooms", "Toggle rendering of all rooms.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "show_room_volumes", "Toggle rendering of room volumes.", "false", PL_VAR_BOOL, &ape_config_.level.showRoomVolumes, NULL, false );
	PlRegisterConsoleVariable( "show_portals", "Toggles the display of portals.", "false", PL_VAR_BOOL, &ape_config_.level.showPortals, NULL, false );
	PlRegisterConsoleVariable( "sort_lights", "Sort lights before drawing world.", "true", PL_VAR_BOOL, &ape_config_.level.sortLights, NULL, false );

	PlRegisterConsoleCommand( "level_save", "Save the current level with the specified name.", 1, level_save_command );
}

void ss_acl_level_client_tick_( void )
{
	ss_arl_build_camera_visibility_lists_();
}

void ss_acl_level_get_player_start( const ApeWorld *level, PLVector3 *position, PLMatrix3 *orientation )
{
	*position = level->startPosition;
	*orientation = level->startOrientation;
}
