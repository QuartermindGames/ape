/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 *
 * Purpose: World management functions.
 */

#include <plcore/pl_filesystem.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_mesh.h>

#include "yin.h"
#include "world.h"
#include "actor.h"

#include "common/node.h"

#include "client/renderer/renderer.h"
#include "client/renderer/material.h"

typedef struct WorldFace
{
	PLVector3 normal;

	Material *material;
	// todo: reduce the below to transform matrix???
	float     materialAngle;
	PLVector2 materialOffset;
	PLVector2 materialScale;

	unsigned int vertices[ WORLD_FACE_MAX_SIDES ];
	uint8_t      numVertices;

	uint8_t flags; /* portal, mirror, skip etc. */

	WorldSector *parentSector;
	WorldFace *  targetSectorFace; /* if portal */

	PLCollisionAABB bounds;
} WorldFace;

typedef struct WorldMesh
{
	char id[ WORLD_PROP_TAG_LENGTH ];

	Material **  materials;
	unsigned int numMaterials;

	PLGVertex *  vertices;
	unsigned int numVertices;

	WorldFace *  faces;
	unsigned int numFaces;

	PLCollisionAABB bounds;

	PLLinkedListNode *node;

	MemRefCnt mem;
} WorldMesh;

typedef struct WorldObject
{
	WorldMesh *mesh;// pointer to mesh in worldMeshes list
	PLMatrix4  transform;

	WorldObjectCollisionType collisionType;
	union
	{
		const WorldMesh *      collisionMesh;
		const PLCollisionAABB *collisionBounds;
	} collisionPtr;
} WorldObject;

typedef struct WorldSector
{
	char id[ WORLD_PROP_TAG_LENGTH ];

	WorldMesh *mesh;

	WorldObject *staticObjects;
	unsigned int numStaticObjects;

	PLLinkedList *actors;// Actors currently in this sector

	PLCollisionAABB bounds;
} WorldSector;

typedef struct World
{
	WorldMesh *  meshes;
	unsigned int numMeshes;

	WorldSector *sectors;
	unsigned int numSectors;

	/* additional generic properties */
	NLNode *globalProperties;
} World;

static PLLinkedList *worlds;
static PLLinkedList *worldMeshes;

static void W_DeserializeIdentifierTag( NLNode *node, char *dest )
{
	dest[ WORLD_PROP_TAG_LENGTH ] = '\0';

	const char *id = NL_GetStrByName( node, "id", NULL );
	if ( id == NULL )
	{
		PlGenerateUniqueIdentifier( dest, WORLD_PROP_TAG_LENGTH - 1 );
		return;
	}

	strncpy( dest, id, WORLD_PROP_TAG_LENGTH - 1 );
}

static void W_WorldMeshCleanupCB( void *userData )
{
	WorldMesh *worldMesh = ( WorldMesh * ) userData;

	PlDestroyLinkedListNode( worldMeshes, worldMesh->node );
}

void W_DeserializeMaterials( NLNode *meshNode, WorldMesh *meshPtr )
{
	NLNode *materialsList = NL_GetChildByName( meshNode, "materials" );
	if ( materialsList == NULL )
	{
		PrintWarn( "No materials for mesh: %s!\n", meshPtr->id );
		return;
	}

	meshPtr->numMaterials = NL_GetNumOfChildren( materialsList );
	meshPtr->materials    = globalSystem.CAlloc( meshPtr->numMaterials, sizeof( Material * ), true );
	NLNode *materialNode  = NL_GetFirstChild( materialsList );
	for ( unsigned int i = 0; i < meshPtr->numMaterials; ++i )
	{
		if ( materialNode == NULL )
		{
			PrintWarn( "Hit an invalid material index!\n" );
			meshPtr->numMaterials = i;
			break;
		}

		char materialPath[ PL_SYSTEM_MAX_PATH ];
		NL_GetStr( materialNode, materialPath, sizeof( materialPath ) );
		meshPtr->materials[ i ] = RM_CacheMaterial( materialPath, CACHE_GROUP_WORLD, true );
		materialNode            = NL_GetNextChild( materialNode );
	}
}

void W_DeserializeVertices( NLNode *meshNode, WorldMesh *meshPtr )
{
	NLNode *verticesList = NL_GetChildByName( meshNode, "vertices" );
	if ( verticesList == NULL )
	{
		PrintWarn( "No vertices for mesh: %s!\n", meshPtr->id );
		return;
	}

	meshPtr->numVertices = NL_GetNumOfChildren( verticesList );
	meshPtr->vertices    = globalSystem.CAlloc( meshPtr->numVertices, sizeof( PLGVertex ), true );
	NLNode *vertexNode   = NL_GetFirstChild( verticesList );
	for ( unsigned int i = 0; i < meshPtr->numVertices; ++i )
	{
		if ( vertexNode == NULL )
		{
			PrintWarn( "Hit an invalid vertex index!\n" );
			meshPtr->numVertices = i;
			break;
		}

		NL_DS_DeserializeVertex( vertexNode, &meshPtr->vertices[ i ] );
	}
}

void W_DeserializeFaces( NLNode *meshNode, WorldMesh *meshPtr )
{
	NLNode *facesList = NL_GetChildByName( meshNode, "faces" );
	if ( facesList == NULL )
	{
		PrintWarn( "No faces for mesh: %s!\n", meshPtr->id );
		return;
	}

	meshPtr->numFaces = NL_GetNumOfChildren( facesList );
	meshPtr->faces    = globalSystem.CAlloc( meshPtr->numFaces, sizeof( WorldFace ), true );
	NLNode *faceNode  = NL_GetFirstChild( facesList );
	for ( unsigned int i = 0; i < meshPtr->numFaces; ++i )
	{
		if ( faceNode == NULL )
		{
			PrintWarn( "Hit an invalid face index!\n" );
			meshPtr->numFaces = i;
			break;
		}

		int materialIndex = NL_GetI32ByName( faceNode, "materialIndex", -1 );
		if ( materialIndex >= 0 && materialIndex < meshPtr->numMaterials )
			meshPtr->faces[ i ].material = meshPtr->materials[ materialIndex ];

		meshPtr->faces[ i ].materialAngle = NL_GetF32ByName( faceNode, "materialAngle", 0.0f );

		NLNode *n;
		if ( ( n = NL_GetChildByName( faceNode, "materialOffset" ) ) != NULL )
			NL_DS_DeserializeVector2( n, &meshPtr->faces[ i ].materialOffset );
		if ( ( n = NL_GetChildByName( faceNode, "materialScale" ) ) != NULL )
			NL_DS_DeserializeVector2( n, &meshPtr->faces[ i ].materialScale );

		if ( ( n = NL_GetChildByName( faceNode, "vertices" ) ) != NULL )
		{
			meshPtr->faces[ i ].numVertices = NL_GetNumOfChildren( n );
			if ( meshPtr->faces[ i ].numVertices >= WORLD_FACE_MAX_SIDES )
			{
				PrintWarn( "Too many vertices for face: %d!\n", i );
				meshPtr->faces[ i ].numVertices = WORLD_FACE_MAX_SIDES;
			}

			if ( meshPtr->faces[ i ].numVertices > 0 )
				NL_GetUI32Array( n, meshPtr->faces[ i ].vertices, meshPtr->faces[ i ].numVertices );
		}

		meshPtr->faces[ i ].flags = NL_GetI32ByName( faceNode, "flags", 0 );

		faceNode = NL_GetNextChild( faceNode );
	}
}

static WorldMesh *W_DeserializeWorldMesh( NLNode *meshNode, WorldMesh *meshPtr )
{
	W_DeserializeIdentifierTag( meshNode, meshPtr->id );
	W_DeserializeMaterials( meshNode, meshPtr );
	W_DeserializeVertices( meshNode, meshPtr );
	W_DeserializeFaces( meshNode, meshPtr );

	Mem_SetupReferenceInstance( &meshPtr->mem, W_WorldMeshCleanupCB, meshPtr );

	return meshPtr;
}

static WorldMesh *W_GetWorldMeshForTag( const char *tag )
{
	PLLinkedListNode *node = PlGetFirstNode( worldMeshes );
	while ( node != NULL )
	{
		WorldMesh *mesh = PlGetLinkedListNodeUserData( node );
		if ( strcmp( tag, mesh->id ) == 0 )
			return mesh;

		node = PlGetNextLinkedListNode( node );
	}

	return NULL;
}

static void W_DeserializeSector( World *world, NLNode *sectorNode, WorldSector *sectorPtr )
{
	W_DeserializeIdentifierTag( sectorNode, sectorPtr->id );

	const char *tag;
	tag = NL_GetStrByName( sectorNode, "mesh", NULL );
	if ( tag == NULL )
		PrintWarn( "Sector without body!\n" );
	else
	{
		sectorPtr->mesh = W_GetWorldMeshForTag( tag );
		if ( sectorPtr->mesh == NULL )
			PrintWarn( "Failed to get mesh for sector: %s\n", tag );
	}

	NLNode *staticObjectList = NL_GetChildByName( sectorNode, "staticObjects" );
	if ( staticObjectList != NULL )
	{
		sectorPtr->numStaticObjects = NL_GetNumOfChildren( staticObjectList );
		sectorPtr->staticObjects    = globalSystem.CAlloc( sectorPtr->numStaticObjects, sizeof( WorldObject ), true );
		NLNode *c                   = NL_GetFirstChild( staticObjectList );
		for ( unsigned int i = 0; i < sectorPtr->numStaticObjects; ++i )
		{
			if ( c == NULL )
			{
				PrintWarn( "Hit an invalid object index: %d\n", i );
				sectorPtr->numStaticObjects = i;
				break;
			}

			tag = NL_GetStrByName( c, "mesh", NULL );
			if ( tag == NULL )
				PrintWarn( "Object %d without body!\n", i );
			else
			{
				sectorPtr->staticObjects[ i ].mesh = W_GetWorldMeshForTag( tag );
				if ( sectorPtr->staticObjects[ i ].mesh == NULL )
					PrintWarn( "Failed to get mesh for object: %s\n", tag );
			}

			NLNode *matrix = NL_GetChildByName( c, "transform" );
			if ( matrix != NULL )
				NL_DS_DeserializeMatrix4( matrix, &sectorPtr->staticObjects[ i ].transform );

			c = NL_GetNextChild( c );
		}
	}

	NLNode *actorList = NL_GetChildByName( sectorNode, "actors" );
	if ( actorList != NULL )
	{
		NLNode *c = NL_GetFirstChild( actorList );
		while ( c != NULL )
		{
			const char *actorId = NL_GetStrByName( c, "id", NULL );
			if ( actorId == NULL )
			{
				PrintWarn( "Failed to get id for actor!\n" );
				c = NL_GetNextChild( c );
				continue;
			}

			Actor *actor = Act_SpawnActorById( actorId, c );
			if ( actor != NULL )
				Act_SetWorldSector( actor, sectorPtr );

			c = NL_GetNextChild( c );
		}
	}
}

static World *W_DeserializeWorld( NLNode *in, World *out )
{
	Print( "Deserializing world\n" );

	int version = NL_GetI32ByName( in, "version", -1 );
	if ( version == -1 )
	{
		PrintWarn( "Failed to find world version!\n" );
		return NULL;
	}
	else if ( version > WORLD_VERSION )
	{
		PrintWarn( "Unsupported world version: %d\n", version );
		return NULL;
	}

	NLNode *propertyList = NL_GetChildByName( in, "properties" );
	if ( propertyList != NULL )
		out->globalProperties = NL_CopyNode( propertyList );

	NLNode *meshList = NL_GetChildByName( in, "meshes" );
	if ( meshList != NULL )
	{
		out->numMeshes = NL_GetNumOfChildren( meshList );
		out->meshes    = globalSystem.CAlloc( out->numMeshes, sizeof( WorldMesh ), true );
		NLNode *c      = NL_GetFirstChild( meshList );
		for ( unsigned int i = 0; i < out->numMeshes; ++i )
		{
			if ( c == NULL )
			{
				PrintWarn( "Hit an invalid mesh index: %d\n", i );
				out->numMeshes = i;
				break;
			}

			W_DeserializeWorldMesh( c, &out->meshes[ i ] );
		}
	}

	NLNode *sectorList = NL_GetChildByName( in, "sectors" );
	if ( sectorList != NULL )
	{
		out->numSectors = NL_GetNumOfChildren( sectorList );
		out->sectors    = globalSystem.CAlloc( out->numSectors, sizeof( WorldSector ), true );
		NLNode *c       = NL_GetFirstChild( sectorList );
		for ( unsigned int i = 0; i < out->numSectors; ++i )
		{
			if ( c == NULL )
			{
				PrintWarn( "Hit an invalid sector Index: %d\n", i );
				out->numSectors = i;
				break;
			}

			W_DeserializeSector( out, c, &out->sectors[ i ] );
		}
	}
	else
		PrintWarn( "No sectors specified for world!\n" );

	return out;
}

WorldMesh *W_LoadWorldMesh( const char *path )
{
	NLNode *node = NL_LoadFile( path, "mesh" );
	if ( node == NULL )
	{
		PrintWarn( "Failed to load mesh: %s\n", path );
		return NULL;
	}

	WorldMesh *mesh = globalSystem.MAlloc( sizeof( WorldMesh ), true );
	if ( W_DeserializeWorldMesh( node, mesh ) == NULL )
	{
		W_ReleaseWorldMesh( mesh );
		mesh = NULL;
	}

	return mesh;
}

WorldMesh *W_CacheWorldMesh( const char *path )
{
}

World *W_LoadWorld( const char *path )
{
	NLNode *node = NL_LoadFile( path, "world" );
	if ( node == NULL )
	{
		PrintWarn( "Failed to load world: %s\n", path );
		return NULL;
	}

	World *world = globalSystem.MAlloc( sizeof( World ), true );
	if ( W_DeserializeWorld( node, world ) == NULL )
	{
		W_DestroyWorld( world );
		world = NULL;
	}

	return world;
}

void W_ReleaseWorldMesh( WorldMesh *worldMesh )
{
	if ( worldMesh == NULL )
		return;

	Mem_ReleaseReference( &worldMesh->mem );
}

void W_DestroyWorld( World *world )
{
	if ( world == NULL )
		return;

	for ( unsigned int i = 0; i < world->numSectors; ++i )
		globalSystem.Free( world->sectors[ i ].subMeshes );

	globalSystem.Free( world->sectors );

	for ( unsigned int i = 0; i < world->numMeshes; ++i )
		W_ReleaseWorldMesh( &world->meshes[ i ] );

	globalSystem.Free( world->meshes );
	globalSystem.Free( world );
}

/**
 * Fetch the normal for the specified face.
 */
PLVector3 W_GetFaceNormal( const WorldFace *face )
{
	return face->normal;
}

/**
 * Fetch the origin point of the face in world-coordinates.
 */
PLVector3 W_GetFaceOrigin( const WorldFace *face )
{
	return face->bounds.absOrigin;
}

/**
 * Fetch the flags specified for the face.
 */
uint8_t W_GetFaceFlags( const WorldFace *face )
{
	return face->flags;
}

/****************************************
 * SECTOR
 ****************************************/

static unsigned int GetNumOfFaceTriangles( const WorldFace *face )
{
	if ( face->numVertices < 3 )
		return 0;

	return face->numVertices - 2;
}

static unsigned int *ConvertFaceToTriangles( const WorldFace *face, unsigned int *numTriangles )
{
	*numTriangles = GetNumOfFaceTriangles( face );
	if ( *numTriangles == 0 )
		return NULL;

	unsigned int *indices = pl_malloc( sizeof( unsigned int ) * ( *numTriangles * 3 ) );
	unsigned int *index   = indices;
	for ( unsigned int i = 1; i + 1 < face->numVertices; ++i )
	{
		index[ 0 ] = 0;
		index[ 1 ] = i;
		index[ 2 ] = i + 1;
		index += 3;
	}

	return indices;
}

/**
 * This crudely tries to determine the sector by an origin point.
 * Should only be used for vague lookup.
 */
static WorldSector *W_GetSectorByAbsOrigin( World *world, const PLVector3 *absOrigin )
{
	for ( unsigned int i = 0; i < world->numSectors; ++i )
	{
		WorldSector *sector = &world->sectors[ i ];
		if ( !PlIsPointIntersectingAabb( &sector->bounds, *absOrigin ) )
			continue;

		return sector;
	}

	return NULL;
}

WorldMesh *W_GetMeshForSector( WorldSector *sector )
{
	return sector->mesh;
}

WorldFace *W_GetFacesForSector( WorldSector *sector, uint32_t *numFaces )
{
	*numFaces = sector->mesh->numFaces;
	return sector->mesh->faces;
}

static WorldSector **GetVisibleSectors( World *world, WorldSector *originSector, const PLGCamera *camera, unsigned int *numSectors )
{
	CVar( "world.drawSectors", drawSectors );
	if ( drawSectors != NULL && !drawSectors->b_value )
		return NULL;

	WorldSector **visibleSectors = globalSystem.CAlloc( world->numSectors, sizeof( WorldSector * ), true );

	/* todo
	 *  1. find cur sector player is within
	 *  2. test whether or not player can see portal
	 *  3. get portal destination, do 2. again but from portal
	 */

	WorldMesh *sectorMesh = originSector->mesh;
	for ( unsigned int i = 0; i < sectorMesh->numFaces; ++i )
	{
		if ( !( sectorMesh->faces[ i ].flags & WORLD_FACE_FLAG_PORTAL ) && !( sectorMesh->faces[ i ].flags & WORLD_FACE_FLAG_MIRROR ) )
			continue;

		if ( !PlgIsBoxInsideView( camera, &sectorMesh->faces[ i ].bounds ) )
			continue;
	}
}

static WorldMesh **GetVisibleSubMeshesForSector( WorldSector *sector, const PLGCamera *camera, unsigned int *numMeshes )
{
	CVar( "world.drawSubMeshes", drawSubMeshes );
	if ( drawSubMeshes != NULL && !drawSubMeshes->b_value )
		return NULL;

	WorldMesh **visibleMeshes = globalSystem.CAlloc( sector->numStaticObjects, sizeof( WorldMesh * ), true );

	// Go through and generate a list of visible meshes within the sector
	for ( unsigned int i = 0; i < sector->numStaticObjects; ++i )
	{
		PLCollisionAABB bounds = sector->staticObjects[ i ].mesh->bounds;
		bounds.origin          = PlGetMatrix4Translation( &sector->staticObjects[ i ].transform );
		if ( !PlgIsBoxInsideView( camera, &sector->staticObjects[ i ].mesh->bounds ) )
			continue;

		visibleMeshes[ *numMeshes ] = sector->mesh;
		numMeshes++;
	}

	// Shrink the array down before passing it back
	visibleMeshes = globalSystem.ReAlloc( visibleMeshes, sizeof( WorldMesh * ) * *numMeshes, true );
	return visibleMeshes;
}

static PLGMesh *triangleMesh = NULL;
static void     DrawSector( WorldSector *sector, PLGCamera *camera, bool simple )
{
	unsigned int numVisibleMeshes;
	WorldMesh ** visibleMeshes = GetVisibleSubMeshesForSector( sector, camera, &numVisibleMeshes );

	unsigned int numFaces;
	WorldFace *  faces = W_GetFacesForSector( sector, &numFaces );
	if ( faces == NULL || numFaces == 0 )
	{
		PrintWarn( "Invalid number of faces in sector!\n" );
		return;
	}

	CVar( "world.forceSimple", forceSimple );
	if ( simple || forceSimple->b_value )
	{
		for ( unsigned int j = 0; j < numFaces; ++j )
		{
			WorldFace *face     = &faces[ j ];
			face->bounds.origin = PLVector3( 0.0f, 0.0f, 0.0f );

			/* check the face is actually visible */
			if ( !PlgIsBoxInsideView( camera, &face->bounds ) )
				continue;

			for ( unsigned int k = 0; k < face->numVertices; ++k )
			{
				PLGVertex *  vertex = &sector->mesh->vertices[ face->vertices[ k ] ];
				unsigned int v      = PlgAddMeshVertex( triangleMesh, vertex->position, vertex->normal, vertex->colour, vertex->st[ 0 ] );
				/* this shit is generated earlier in the process, and right now I'm not sure if it's
				 * appropriate to add to AddMeshVertex */
				triangleMesh->vertices[ v ].tangent   = vertex->tangent;
				triangleMesh->vertices[ v ].bitangent = vertex->bitangent;
			}

			PLGVertex vertices[ WORLD_FACE_MAX_SIDES ];
			memset( vertices, 0, sizeof( PLGVertex ) * WORLD_FACE_MAX_SIDES );

			unsigned int  numTriangles;
			unsigned int *indices  = ConvertFaceToTriangles( face, &numTriangles );
			unsigned int *curIndex = indices;
			for ( unsigned int k = 0; k < numTriangles; ++k )
			{
				PlgAddMeshTriangle( triangleMesh,
				                    curIndex[ 0 ] + triangleMesh->num_verts - face->numVertices,
				                    curIndex[ 1 ] + triangleMesh->num_verts - face->numVertices,
				                    curIndex[ 2 ] + triangleMesh->num_verts - face->numVertices );
				curIndex += 3;
			}
			globalSystem.Free( indices );

			g_gfxPerfStats.numFacesDrawn++;
		}

		if ( triangleMesh->num_triangles == 0 )
			return;

		Material *material = RM_CacheMaterial( "materials/engine/simple.mat", CACHE_GROUP_STATIC, true );
		RM_DrawMesh( material, triangleMesh );
		return;
	}

	/* batch everything by material */
	for ( unsigned int i = 0; i < world->numMaterials; ++i )
	{
		for ( unsigned int j = 0; j < numFaces; ++j )
		{
			WorldFace *face = &faces[ j ];
			if ( face->material != world->materials[ i ] )
				continue;

			face->bounds.origin = PLVector3( 0.0f, 0.0f, 0.0f );

			/* check the face is actually visible */
			if ( !PlgIsBoxInsideView( camera, &face->bounds ) )
				continue;

			for ( unsigned int k = 0; k < face->numVertices; ++k )
			{
				PLGVertex *  vertex = &world->vertices[ face->vertices[ k ] ];
				unsigned int v      = PlgAddMeshVertex( triangleMesh, vertex->position, vertex->normal, vertex->colour, vertex->st[ 0 ] );
				/* this shit is generated earlier in the process, and right now I'm not sure if it's
				 * appropriate to add to AddMeshVertex */
				triangleMesh->vertices[ v ].tangent   = vertex->tangent;
				triangleMesh->vertices[ v ].bitangent = vertex->bitangent;
			}

			PLGVertex vertices[ WORLD_FACE_MAX_SIDES ];
			memset( vertices, 0, sizeof( PLGVertex ) * WORLD_FACE_MAX_SIDES );

			unsigned int  numTriangles;
			unsigned int *indices  = ConvertFaceToTriangles( face, &numTriangles );
			unsigned int *curIndex = indices;
			for ( unsigned int k = 0; k < numTriangles; ++k )
			{
				PlgAddMeshTriangle( triangleMesh,
				                    curIndex[ 0 ] + triangleMesh->num_verts - face->numVertices,
				                    curIndex[ 1 ] + triangleMesh->num_verts - face->numVertices,
				                    curIndex[ 2 ] + triangleMesh->num_verts - face->numVertices );
				curIndex += 3;
			}
			globalSystem.Free( indices );

			g_gfxPerfStats.numFacesDrawn++;
		}

		if ( triangleMesh->num_triangles == 0 )
			continue;

		RM_DrawMesh( world->materials[ i ], triangleMesh );
	}
}

/****************************************
 * WORLD
 ****************************************/

static void W_Debug_DrawSectorVolumes( World *world, PLGCamera *camera )
{
	WorldSector *originSector = W_GetSectorByAbsOrigin( world, &camera->position );
	if ( originSector == NULL )
		return;

	unsigned int  numVisibleSectors;
	WorldSector **visibleSectors = GetVisibleSectors( world, originSector, camera, &numVisibleSectors );

	srand( numVisibleSectors );

	for ( unsigned int i = 0; i < numVisibleSectors; ++i )
	{
		WorldSector *sector = visibleSectors[ i ];
		PlgDrawBoundingVolume( &sector->bounds, PLColourRGB( rand() % 255, rand() % 255, rand() % 255 ) );
	}
}

void W_Draw( World *world, PLGCamera *camera )
{
	PROFILE_START( PROFILE_DRAW_MAP );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	CVar( "world.drawSectorVolumes", drawSectorVolumes );

	if ( drawSectorVolumes != NULL && drawSectorVolumes->b_value )
		W_Debug_DrawSectorVolumes( world, camera );
	else
	{
		WorldSector *originSector = W_GetSectorByAbsOrigin( world, &camera->position );
		if ( originSector != NULL )
		{
			unsigned int  numVisibleSectors;
			WorldSector **visibleSectors = GetVisibleSectors( world, originSector, camera, &numVisibleSectors );
			for ( unsigned int i = 0; i < numVisibleSectors; ++i )
				DrawSector( visibleSectors[ i ], camera, false );

			globalSystem.Free( visibleSectors );
		}
	}

	PlPopMatrix();

	PROFILE_END( PROFILE_DRAW_MAP );
}
