// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>
#include <plgraphics/plg_mesh.h>

#include "core_private.h"
#include "world.h"
#include "legacy/actor.h"

#include <yin/node.h>

#include "client/renderer/renderer.h"

void YnCore_World_SetupGlobalDefaults( OgeWorld *world )
{
	world->ambience    = WORLD_DEFAULT_AMBIENCE;
	world->sunColour   = WORLD_DEFAULT_SUNCOLOUR;
	world->sunPosition = WORLD_DEFAULT_SUNPOSITION;
	world->clearColour = WORLD_DEFAULT_CLEARCOLOUR;
}

OgeWorld *YnCore_World_Create( void )
{
	OgeWorld *world = PlMAllocA( sizeof( OgeWorld ) );

	world->globalProperties = ndPushBackObject( NULL, "properties" );
	ndPushBackF32Array( world->globalProperties, "ambience", ( const float * ) &WORLD_DEFAULT_AMBIENCE, 4 );
	ndPushBackF32Array( world->globalProperties, "clearColour", ( const float * ) &WORLD_DEFAULT_CLEARCOLOUR, 4 );
	//NL_PushBackStrArray( world->globalProperties, "skyMaterials", ( const char ** ) WORLD_DEFAULT_SKY, 1 );

	world->meshes   = PlCreateVectorArray( 0 );
	world->entities = PlCreateLinkedList();

	return world;
}

OgeWorld *YnCore_World_LoadFromNode( NdBranch *root )
{
	OgeWorld *world = YnCore_World_Create();
	if ( world != NULL && YnCore_WorldDeserialiser_Begin( root, world ) == NULL )
	{
		YnCore_World_Destroy( world );
		world = NULL;
	}

	return world;
}

OgeWorld *YnCore_World_Load( const char *path )
{
	NdBranch *root = ndLoadFile( path, "world" );
	if ( root == NULL )
	{
		PRINT_WARNING( "Failed to load world (%s): %s\n", path, ndGetErrorMessage() );
		return NULL;
	}

	OgeWorld *world = YnCore_World_LoadFromNode( root );
	if ( world != NULL )
	{
		snprintf( world->path, sizeof( world->path ), "%s", path );
	}

	ndDestroyBranch( root );

	return world;
}

bool YnCore_World_Save( OgeWorld *world, const char *path )
{
	world->lastSaveTime = time( NULL );

	NdBranch *root = ndPushBackObject( NULL, "world" );

	YnCore_WorldSerialiser_Begin( world, root );
	snprintf( world->path, sizeof( world->path ), "%s", path );

	if ( !ndWriteFile( path, root, ND_FILE_BINARY ) )
	{
		PRINT_WARNING( "Failed to save world (%s): %s\n", path, ndGetErrorMessage() );
		return false;
	}

	return true;
}

/**
 * Clears the current assigned mesh and all static
 * objects for the given sector.
 */
static void ClearSector( OgeWorldSector *sector )
{
	for ( unsigned int i = 0; i < sector->numStaticObjects; ++i )
		YnCore_WorldMesh_Release( sector->staticObjects[ i ].mesh );

	PlFree( sector->staticObjects );

	YnCore_WorldMesh_Release( sector->mesh );
}

static void DestroyWorldEntities( OgeWorld *world )
{
	PLLinkedListNode *node = PlGetFirstNode( world->entities );
	while ( node != NULL )
	{
		PL_DELETE( PlGetLinkedListNodeUserData( node ) );
		node = PlGetNextLinkedListNode( node );
	}
}

void YnCore_World_Destroy( OgeWorld *world )
{
	if ( world == NULL )
		return;

	for ( unsigned int i = 0; i < world->numSectors; ++i )
		ClearSector( &world->sectors[ i ] );

	PlFree( world->sectors );

	unsigned int numMeshes = PlGetNumVectorArrayElements( world->meshes );
	for ( unsigned int i = 0; i < numMeshes; ++i )
		YnCore_WorldMesh_Release( ( OgeWorldMesh * ) PlGetVectorArrayElementAt( world->meshes, i ) );

	DestroyWorldEntities( world );

	PlDestroyVectorArray( world->meshes );
	PlFree( world );
}

PLLinkedList *YnCore_World_GetLights( const OgeWorld *world )
{
	for ( unsigned int i = 0; i < world->numSectors; ++i )
	{
	}

	return NULL;
}

PLLinkedList *YnCore_World_GetSectorLights( const OgeWorldSector *sector )
{
	return sector->lights;
}

void YnCore_World_SpawnEntities( OgeWorld *world )
{
	PLLinkedListNode *node = PlGetFirstNode( world->entities );
	while ( node != NULL )
	{
		OgeWorldEntity *worldEntity = ( OgeWorldEntity * ) PlGetLinkedListNodeUserData( node );
		YnCore_EntityManager_CreateEntityFromPrefab( worldEntity->entityTemplate->name );
		node = PlGetNextLinkedListNode( node );
	}
}

/****************************************
 * Global World Properties
 ****************************************/

NdBranch *YnCore_World_GetProperty( OgeWorld *world, const char *propertyName )
{
	if ( world->globalProperties == NULL )
		return NULL;

	return ndGetChildByName( world->globalProperties, propertyName );
}

PLColourF32 YnCore_World_GetAmbience( OgeWorld *world ) { return world->ambience; }
PLColourF32 YnCore_World_GetSunColour( OgeWorld *world ) { return world->sunColour; }
PLVector3 YnCore_World_GetSunPosition( OgeWorld *world ) { return world->sunPosition; }

/****************************************
 ****************************************/

uint64_t YnCore_World_GetLastSaveTime( const OgeWorld *world )
{
	return world->lastSaveTime;
}

/**
 * Fetch the normal for the specified face.
 */
PLVector3 YnCore_WorldFace_GetNormal( const OgeWorldFace *face )
{
	return face->normal;
}

/**
 * Fetch the origin point of the face in world-coordinates.
 */
PLVector3 YnCore_WorldFace_GetOrigin( const OgeWorldFace *face )
{
	return face->bounds.absOrigin;
}

/**
 * Fetch the flags specified for the face.
 */
uint8_t YnCore_WorldFace_GetFlags( const OgeWorldFace *face )
{
	return face->flags;
}

/****************************************
 * SECTOR
 ****************************************/

OgeLight *YnCore_WorldSector_GetVisibleLights( OgeWorldSector *sector, unsigned int *numLights )
{
	// TODO: for now we're just going to return this static list...
	static OgeLight lights[] = {
	        {
             .position = { 10.0f, 10.0f, 10.0f },
             .colour   = { 1.0f, 0.0f, 0.0f, 16.0f },
             .radius   = 16.0f,
	         },
	};

	*numLights = PL_ARRAY_ELEMENTS( lights );
	return lights;
}

/**
 * This crudely tries to determine the sector by an origin point.
 * Should only be used for vague lookup.
 */
OgeWorldSector *YnCore_World_GetSectorByGlobalOrigin( OgeWorld *world, const PLVector3 *globalOrigin )
{
	for ( unsigned int i = 0; i < world->numSectors; ++i )
	{
		OgeWorldSector *sector = &world->sectors[ i ];
		if ( !PlIsPointIntersectingAabb( &sector->bounds, *globalOrigin ) )
			continue;

		return sector;
	}

	return &world->sectors[ 0 ];
}

const char *YnCore_World_GetPath( const OgeWorld *world )
{
	return world->path;
}

/**
 * Get the primary mesh for the given sector, this
 * is essentially the sector's body.
 */
OgeWorldMesh *YnCore_WorldSector_GetMesh( OgeWorldSector *sector )
{
	return sector->mesh;
}

OgeWorldFace **YnCore_WorldSector_GetMeshFaces( OgeWorldSector *sector, uint32_t *numFaces )
{
	if ( sector->mesh == NULL )
	{
		*numFaces = 0;
		return NULL;
	}

	*numFaces               = PlGetNumLinkedListNodes( sector->mesh->faces );
	OgeWorldFace **faces = PL_NEW_( OgeWorldFace *, *numFaces );

	PLLinkedListNode *faceNode = PlGetFirstNode( sector->mesh->faces );
	for ( unsigned int i = 0; i < *numFaces; ++i )
	{
		faces[ i ] = ( OgeWorldFace * ) PlGetLinkedListNodeUserData( faceNode );
		faceNode   = PlGetNextLinkedListNode( faceNode );
	}

	return faces;
}

static OgeWorldSector **GetVisibleSectors( OgeWorld *world, OgeWorldSector *originSector, const OgeCamera *camera, unsigned int *numSectors )
{
	PL_GET_CVAR( "world.drawSectors", drawSectors );
	if ( drawSectors != NULL && !drawSectors->b_value )
		return NULL;

	OgeWorldSector **visibleSectors = PlCAllocA( world->numSectors, sizeof( OgeWorldSector * ) );

	/* we'll assume the sector we're in is visible (seems like a safe assumption) */
	*numSectors                     = 0;
	visibleSectors[ *numSectors++ ] = originSector;

	/* todo
	 *  2. test whether or not player can see portal
	 *  3. get portal destination, do 2. again but from portal
	 */

#if 0
	WorldMesh *sectorMesh = originSector->mesh;
	if ( sectorMesh != NULL )
	{
		for ( unsigned int i = 0; i < sectorMesh->numFaces; ++i )
		{
			if ( !( sectorMesh->faces[ i ].flags & WORLD_FACE_FLAG_PORTAL ) && !( sectorMesh->faces[ i ].flags & WORLD_FACE_FLAG_MIRROR ) )
			{
				continue;
			}

			if ( !PlgIsBoxInsideView( camera->internal, &sectorMesh->faces[ i ].bounds ) )
			{
				continue;
			}
		}
	}
#endif

	return visibleSectors;
}

static OgeWorldMesh **GetVisibleSubMeshesForSector( OgeWorldSector *sector, const PLGCamera *camera, unsigned int *numMeshes )
{
	PL_GET_CVAR( "world.drawSubMeshes", drawSubMeshes );
	if ( drawSubMeshes != NULL && !drawSubMeshes->b_value )
		return NULL;

	OgeWorldMesh **visibleMeshes = PlCAlloc( sector->numStaticObjects, sizeof( OgeWorldMesh * ), true );

	// Go through and generate a list of visible meshes within the sector
	for ( unsigned int i = 0; i < sector->numStaticObjects; ++i )
	{
		PLCollisionAABB *bounds = &sector->staticObjects[ i ].mesh->bounds;
		bounds->origin          = sector->staticObjects[ i ].transform.translation;
		if ( !PlgIsBoxInsideView( camera, bounds ) )
			continue;

		visibleMeshes[ *numMeshes ] = sector->mesh;
		numMeshes++;
	}

	// Shrink the array down before passing it back
	visibleMeshes = PlReAlloc( visibleMeshes, sizeof( OgeWorldMesh * ) * *numMeshes, true );
	return visibleMeshes;
}

/**
 * This is a little bit silly, but we're considering mirrors as a valid portal too...
 */
bool YnCore_World_IsFacePortal( const OgeWorldFace *face )
{
	return ( ( face->flags & WORLD_FACE_FLAG_MIRROR ) || ( face->flags & WORLD_FACE_FLAG_PORTAL ) );
}

OgeWorldSector *ogeWorld_GetSectorByNum( OgeWorld *world, int sectorNum )
{
	if ( sectorNum < 0 || sectorNum >= world->numSectors )
		return NULL;

	return &world->sectors[ sectorNum ];
}
