// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>
#include <plgraphics/plg_mesh.h>

#include "ape_private.h"
#include "world.h"
#include "client/renderer/renderer.h"

void acl_world_set_global_defaults( ApeWorld *world )
{
	world->ambience = WORLD_DEFAULT_AMBIENCE;
	world->clearColour = WORLD_DEFAULT_CLEARCOLOUR;
}

void acl_world_set_ambience( ApeWorld *world, const PLColourF32 *ambience ) { world->ambience = *ambience; }
void acl_world_set_clear_colour( ApeWorld *world, const PLColourF32 *colour ) { world->clearColour = *colour; }

ApeWorld *ape_world_create( void )
{
	ApeWorld *world = PL_NEW( ApeWorld );

	world->globalProperties = ndPushBackObject( NULL, "properties" );
	ndPushBackF32Array( world->globalProperties, "ambience", ( const float * ) &WORLD_DEFAULT_AMBIENCE, 4 );
	ndPushBackF32Array( world->globalProperties, "clearColour", ( const float * ) &WORLD_DEFAULT_CLEARCOLOUR, 4 );

	world->meshes = PlCreateVectorArray( 0 );
	world->entities = PlCreateVectorArray( 0 );
	world->lights = PlCreateVectorArray( 0 );
	world->entitySpawns = PlCreateLinkedList();

	acl_world_set_global_defaults( world );

	return world;
}

static unsigned int GetTotalVertsForRoom( ApeWorldRoom *room, bool detail )
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
			{
				continue;
			}

			for ( unsigned int k = 0; k < PlGetNumVectorArrayElements( detailRoom->faces ); ++k )
			{
				ApeWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
				assert( face != NULL );
				if ( face == NULL )
				{
					continue;
				}

				numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
			}
		}
	}

	return numVerts;
}

static unsigned int GetTotalFacesForRoom( ApeWorldRoom *room, bool detail )
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

static void SetupRoomSubMeshes( const ApeWorld *world, ApeWorldRoom *room )
{
	unsigned int numMaterials = PlGetNumVectorArrayElements( world->materials );
}

static void CacheRoomMesh( const ApeWorld *world, ApeWorldRoom *room )
{
	if ( room->mesh == NULL )
	{
		room->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_FAN, PLG_DRAW_STATIC, 0, GetTotalVertsForRoom( room, true ) );
		assert( room->mesh != NULL );
		if ( room->mesh == NULL )
		{
			PRINT_WARNING( "Failed to create mesh for room: %s\n", PlGetError() );
			return;
		}
	}
	else
	{
		PlgClearMesh( room->mesh );
	}

	SetupRoomSubMeshes( world, room );

	unsigned int total = 0;
	unsigned int numFaces = GetTotalFacesForRoom( room, false );
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

ApeWorld *apeLoadWorld( const char *path )
{
	ApeWorld *world = NULL;
	const char *ext = PlGetFileExtension( path );
	if ( ext != NULL && pl_strcasecmp( ext, "n" ) == 0 )
	{
		NdBranch *root = ndLoadFile( path, "world" );
		if ( root == NULL )
		{
			PRINT_WARNING( "Failed to load world: %s\n", ndGetErrorMessage() );
			return NULL;
		}

		world = ape_world_create();
		if ( acl_world_deserialize( world, root ) == NULL )
		{
			ape_world_destroy( world );
			world = NULL;
		}

		ndDestroyBranch( root );
	}
	else// todo: old world i/o crud...
	{
		PLFile *file = PlOpenFile( path, false );
		if ( file == NULL )
		{
			PRINT_WARNING( "Failed to load world: %s\n", PlGetError() );
			return NULL;
		}

		world = apeParseRFWorld_( file );

		PlCloseFile( file );
	}

	// Create cached room geometry
	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
	{
		ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
		assert( room != NULL );
		if ( room == NULL || room->isDetail || room->isMeshCached )
		{
			continue;
		}

		CacheRoomMesh( world, room );
	}

	arl_sky_clear_layers_();
	arl_sky_add_layer_( WORLD_DEFAULT_SKY );
	arl_sky_add_layer_( WORLD_DEFAULT_SKY );
	arl_sky_add_layer_( WORLD_DEFAULT_SKY );

	return world;
}

bool apeSaveWorld( ApeWorld *world, const char *path )
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

void ape_world_destroy( ApeWorld *world )
{
	if ( world == NULL )
	{
		return;
	}

	apeFlushWorldVisibilityLists_();

	if ( world->materials != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->materials ); ++i )
		{
			ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, i );
			if ( material == NULL )
			{
				continue;
			}
			ar_material_release( material );
			material = NULL;
		}
		PlDestroyVectorArray( world->materials );
		world->materials = NULL;
	}

	if ( world->rooms != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			if ( room == NULL )
			{
				continue;
			}
			apeDestroyWorldRoom( room );
			room = NULL;
		}
		PlDestroyVectorArray( world->rooms );
		world->rooms = NULL;
	}

	if ( world->portals != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->portals ); ++i )
		{
			ApeWorldPortal *portal = PlGetVectorArrayElementAt( world->portals, i );
			if ( portal == NULL )
			{
				continue;
			}
			PL_DELETE( portal );
			portal = NULL;
		}
		PlDestroyVectorArray( world->portals );
		world->portals = NULL;
	}

	if ( world->vertices != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->vertices ); ++i )
		{
			ApeWorldVertex *vertex = PlGetVectorArrayElementAt( world->vertices, i );
			if ( vertex == NULL )
			{
				continue;
			}
			PlDestroyVectorArray( vertex->adjacentFaces );
			PL_DELETE( vertex );
		}
		PlDestroyVectorArray( world->vertices );
		world->vertices = NULL;
	}

	if ( world->lights != NULL )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->lights ); ++i )
		{
			ApeLight *light = PlGetVectorArrayElementAt( world->lights, i );
			if ( light == NULL )
			{
				continue;
			}

			PL_DELETE( light );
		}

		PlDestroyVectorArray( world->lights );
		world->lights = NULL;
	}
}

void apeSpawnWorldEntities( ApeWorld *world )
{
	PLLinkedListNode *node = PlGetFirstNode( world->entitySpawns );
	while ( node != NULL )
	{
		ApeWorldEntity *worldEntity = ( ApeWorldEntity * ) PlGetLinkedListNodeUserData( node );
		apeCreateEntity( worldEntity->className, worldEntity->properties );
		node = PlGetNextLinkedListNode( node );
	}
}

void ape_world_attach_entity( ApeWorld *world, ApeEntity *entity )
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

static void WorldSaveCallback( unsigned int argc, char **argv )
{
	ApeWorld *world = acl_world_get_current();
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
	PlRegisterConsoleVariable( "world/skipDraw", "Toggle rendering of world.", "false", PL_VAR_BOOL, &ape_config_.world.skipDraw, NULL, false );
	PlRegisterConsoleVariable( "world/skipPortals", "Toggle display of rooms visible through portals.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showAllRooms", "Toggle rendering of all rooms.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showRoomVolumes", "Toggle rendering of room volumes.", "false", PL_VAR_BOOL, NULL, NULL, false );
	PlRegisterConsoleVariable( "world/showPortals", "Toggles the display of portals.", "false", PL_VAR_BOOL, &ape_config_.world.showPortals, NULL, false );
	PlRegisterConsoleVariable( "world/sortLights", "Sort lights before drawing world.", "true", PL_VAR_BOOL, &ape_config_.world.sortLights, NULL, false );

	PlRegisterConsoleCommand( "world/save", "Save the current world with the specified name.", 1, WorldSaveCallback );
}

void apeTickClientWorld_( void )
{
	apeBuildWorldVisibiltyLists_();
}

void apeGetPlayerStart( const ApeWorld *world, PLVector3 *position, PLMatrix3 *orientation )
{
	*position = world->startPosition;
	*orientation = world->startOrientation;
}
