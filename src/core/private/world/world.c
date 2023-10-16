// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>
#include <plgraphics/plg_mesh.h>

#include "ape_private.h"
#include "world.h"
#include "client/renderer/renderer.h"

void acl_level_set_global_defaults( ApeWorld *world )
{
	world->ambience = WORLD_DEFAULT_AMBIENCE;
	world->clearColour = WORLD_DEFAULT_CLEARCOLOUR;
}

void acl_level_set_ambience( ApeWorld *world, const PLColourF32 *ambience ) { world->ambience = *ambience; }
void acl_level_set_clear_colour( ApeWorld *world, const PLColourF32 *colour ) { world->clearColour = *colour; }

ApeWorld *acl_level_create( void )
{
	ApeWorld *world = PL_NEW( ApeWorld );

	world->globalProperties = ndPushBackObject( NULL, "properties" );
	ndPushBackF32Array( world->globalProperties, "ambience", ( const float * ) &WORLD_DEFAULT_AMBIENCE, 4 );
	ndPushBackF32Array( world->globalProperties, "clearColour", ( const float * ) &WORLD_DEFAULT_CLEARCOLOUR, 4 );

	world->meshes = PlCreateVectorArray( 0 );
	world->entities = PlCreateVectorArray( 0 );
	world->lights = PlCreateVectorArray( 0 );
	world->entitySpawns = PlCreateLinkedList();

	acl_level_set_global_defaults( world );

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

ApeWorld *acl_level_load( const char *path )
{
	ApeWorld *level = acl_level_load_file( path );
	if ( level == NULL )
	{
		PRINT_WARNING( "Failed to load level (%s)!\n", path );
		return NULL;
	}

	// Create cached room geometry
	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( level->rooms ); ++i )
	{
		ApeWorldRoom *room = PlGetVectorArrayElementAt( level->rooms, i );
		assert( room != NULL );
		if ( room == NULL || room->isDetail || room->isMeshCached )
			continue;

		cache_room_mesh( level, room );
	}

	arl_sky_clear_layers();
	arl_sky_add_layer( WORLD_DEFAULT_SKY );
	arl_sky_add_layer( WORLD_DEFAULT_SKY );
	arl_sky_add_layer( WORLD_DEFAULT_SKY );

	return level;
}

bool acl_level_save( ApeWorld *world, const char *path )
{
	world->lastSaveTime = time( NULL );

	NdBranch *root = ndPushBackObject( NULL, "world" );

	apeSerializeWorld( world, root );
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

	apeFlushWorldVisibilityLists_();

	if ( level->materials != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( level->materials ); ++i )
		{
			ApeMaterial *material = PlGetVectorArrayElementAt( level->materials, i );
			if ( material == NULL )
				continue;

			ar_material_release( material );
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

			acl_room_destroy( room );
			room = NULL;
		}
		PlDestroyVectorArray( level->rooms );
		level->rooms = NULL;
	}

	if ( level->portals != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( level->portals ); ++i )
		{
			ApeWorldPortal *portal = PlGetVectorArrayElementAt( level->portals, i );
			if ( portal == NULL )
				continue;

			PL_DELETE( portal );
			portal = NULL;
		}
		PlDestroyVectorArray( level->portals );
		level->portals = NULL;
	}

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

	if ( level->lights != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( level->lights ); ++i )
		{
			ApeLight *light = PlGetVectorArrayElementAt( level->lights, i );
			if ( light == NULL )
				continue;

			PL_DELETE( light );
		}

		PlDestroyVectorArray( level->lights );
		level->lights = NULL;
	}
}

void acl_level_spawn_entities( ApeWorld *world )
{
	PLLinkedListNode *node = PlGetFirstNode( world->entitySpawns );
	while ( node != NULL )
	{
		ApeWorldEntity *worldEntity = ( ApeWorldEntity * ) PlGetLinkedListNodeUserData( node );
		acl_entity_create( worldEntity->className, worldEntity->properties );
		node = PlGetNextLinkedListNode( node );
	}
}

void acl_level_attach_entity( ApeWorld *world, ApeEntity *entity )
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

/****************************************
 * Global World Properties
 ****************************************/

NdBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName )
{
	if ( world->globalProperties == NULL )
	{
		return NULL;
	}

	return ndGetChildByName( world->globalProperties, propertyName );
}

/****************************************
 * SECTOR
 ****************************************/

/**
 * This crudely tries to determine the sector by an origin point.
 * Should only be used for vague lookup.
 */
ApeWorldRoom *apeGetRoomAtPosition( ApeWorld *world, const PLVector3 *position )
{
	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
	{
		ApeWorldRoom *room = ( ApeWorldRoom * ) PlGetVectorArrayElementAt( world->rooms, i );
		if ( !PlIsPointIntersectingAabb( &room->bounds, *position ) )
		{
			continue;
		}

		return room;
	}

	return NULL;
}

static void level_save_command( unsigned int argc, char **argv )
{
	ApeWorld *world = acl_level_get_current();
	if ( world == NULL )
	{
		PRINT_WARNING( "No active world, can't save!\n" );
		return;
	}

	const char *dataPath = comGetDataDirectory();

	NdBranch *root = ndPushBackObject( NULL, "world" );

	apeSerializeWorld( world, root );
}

void apeRegisterWorldConsole_( void )
{
	PlRegisterConsoleVariable( "world/skipDraw", "Toggle rendering of world.", "false", PL_VAR_BOOL, &ape_config_.level.skipDraw, NULL, false );
	PlRegisterConsoleVariable( "world/skipPortals", "Toggle display of rooms visible through portals.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showAllRooms", "Toggle rendering of all rooms.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showRoomVolumes", "Toggle rendering of room volumes.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showPortals", "Toggles the display of portals.", "false", PL_VAR_BOOL, &ape_config_.level.showPortals, NULL, false );
	PlRegisterConsoleVariable( "world/sortLights", "Sort lights before drawing world.", "true", PL_VAR_BOOL, &ape_config_.level.sortLights, NULL, false );

	PlRegisterConsoleCommand( "level_save", "Save the current level with the specified name.", 1, level_save_command );
}

void acl_level_client_tick_( void )
{
	acl_level_build_visibility_lists_();
}

void acl_level_get_player_start( const ApeWorld *world, PLVector3 *position, PLMatrix3 *orientation )
{
	*position = world->startPosition;
	*orientation = world->startOrientation;
}
