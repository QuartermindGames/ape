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

	PLGMesh *drawMesh; /* what actually gets rendered */

	PLLinkedListNode *node;

	MEMReference mem;
} WorldMesh;

typedef struct WorldObject
{
	WorldMesh *mesh; /* pointer to mesh in worldMeshes list */

	SGTransform transform;

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

	PLVector4 ambience;
	PLVector4 sunColour;
	PLVector3 sunPosition;

	/* additional generic properties */
	NLNode *globalProperties;
} World;

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

/**
 * Free the mesh from memory.
 */
static void W_CB_DestroyWorldMesh( void *userData )
{
	WorldMesh *worldMesh = ( WorldMesh * ) userData;

	PlgDestroyMesh( worldMesh->drawMesh );

	PlDestroyLinkedListNode( worldMeshes, worldMesh->node );
}

/**
 * Deserialise a mesh from the given node.
 */
static WorldMesh *W_DeserializeWorldMesh( NLNode *meshNode, WorldMesh *meshPtr )
{
	W_DeserializeIdentifierTag( meshNode, meshPtr->id );
	W_DeserializeMaterials( meshNode, meshPtr );
	W_DeserializeVertices( meshNode, meshPtr );
	W_DeserializeFaces( meshNode, meshPtr );

	meshPtr->drawMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, meshPtr->numFaces, meshPtr->numVertices );
	if ( meshPtr->drawMesh == NULL )
		PrintError( "Failed to create internal mesh for world mesh!\n" );

	MEM_SetupReferenceInstance( "world", &meshPtr->mem, W_CB_DestroyWorldMesh, meshPtr );
	MEM_AddReference( &meshPtr->mem );

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

			NLNode *transform;
			if ( ( transform = NL_GetChildByName( c, "translation" ) ) != NULL )
				NL_DS_DeserializeVector3( transform, &sectorPtr->staticObjects[ i ].transform.translation );
			if ( ( transform = NL_GetChildByName( c, "scale" ) ) != NULL )
				NL_DS_DeserializeVector3( transform, &sectorPtr->staticObjects[ i ].transform.scale );
			if ( ( transform = NL_GetChildByName( c, "rotation" ) ) != NULL )
				NL_DS_DeserializeVector4( transform, ( PLVector4 * ) &sectorPtr->staticObjects[ i ].transform.rotation );

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
	{
		out->globalProperties = NL_CopyNode( propertyList );

		/* set some of the global defaults */

		NLNode *childProperty;
		if ( ( childProperty = NL_GetChildByName( out->globalProperties, "ambience" ) ) != NULL )
			NL_DS_DeserializeVector4( childProperty, &out->ambience );
		else
			out->ambience = PLVector4( 0.4f, 0.4f, 0.4f, 1.0f );

		if ( ( childProperty = NL_GetChildByName( out->globalProperties, "sunColour" ) ) != NULL )
			NL_DS_DeserializeVector4( childProperty, &out->sunColour );
		else
			out->sunColour = PLVector4( 1.0f, 1.0f, 1.0f, 1.25f );

		if ( ( childProperty = NL_GetChildByName( out->globalProperties, "sunPosition" ) ) != NULL )
			NL_DS_DeserializeVector3( childProperty, &out->sunPosition );
		else
			out->sunPosition = PLVector3( 0.5f, -1.0f, 0.5f );
	}

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
		W_DestroyWorld( NULL );
		world = NULL;
	}

	return world;
}

WorldMesh *W_CacheWorldMesh( const char *path )
{
}

void W_ReleaseWorldMesh( WorldMesh *worldMesh )
{
	if ( worldMesh == NULL )
		return;

	MEM_ReleaseReference( &worldMesh->mem );
}

/**
 * Clears the current assigned mesh and all static
 * objects for the given sector.
 */
static void W_ClearSector( WorldSector *sector )
{
	for ( unsigned int i = 0; i < sector->numStaticObjects; ++i )
		W_ReleaseWorldMesh( sector->staticObjects[ i ].mesh );

	globalSystem.Free( sector->staticObjects );

	W_ReleaseWorldMesh( sector->mesh );
}

void W_DestroyWorld( World *world )
{
	if ( world == NULL )
		return;

	for ( unsigned int i = 0; i < world->numSectors; ++i )
		W_ClearSector( &world->sectors[ i ] );

	globalSystem.Free( world->sectors );

	for ( unsigned int i = 0; i < world->numMeshes; ++i )
		W_ReleaseWorldMesh( &world->meshes[ i ] );

	globalSystem.Free( world->meshes );
	globalSystem.Free( world );
}

NLNode *W_GetWorldProperty( World *world, const char *propertyName )
{
	return NL_GetChildByName( world->globalProperties, propertyName );
}

PLVector4 W_GetAmbience( World *world ) { return world->ambience; }
PLVector4 W_GetSunColour( World *world ) { return world->sunColour; }
PLVector3 W_GetSunPosition( World *world ) { return world->sunPosition; }

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
static WorldSector *W_GetSectorByGlobalOrigin( World *world, const PLVector3 *globalOrigin )
{
	for ( unsigned int i = 0; i < world->numSectors; ++i )
	{
		WorldSector *sector = &world->sectors[ i ];
		if ( !PlIsPointIntersectingAabb( &sector->bounds, *globalOrigin ) )
			continue;

		return sector;
	}

	PrintWarn( "Failed to find sector by origin, returning first!\n" );

	return &world->sectors[ 0 ];
}

/**
 * Get the primary mesh for the given sector, this
 * is essentially the sector's body.
 */
WorldMesh *W_GetMeshForSector( WorldSector *sector )
{
	return sector->mesh;
}

WorldFace *W_GetFacesForSector( WorldSector *sector, uint32_t *numFaces )
{
	*numFaces = sector->mesh->numFaces;
	return sector->mesh->faces;
}

static WorldSector **GetVisibleSectors( World *world, WorldSector *originSector, const Camera *camera, unsigned int *numSectors )
{
	CVar( "world.drawSectors", drawSectors );
	if ( drawSectors != NULL && !drawSectors->b_value )
		return NULL;

	WorldSector **visibleSectors = globalSystem.CAlloc( world->numSectors, sizeof( WorldSector * ), true );

	/* todo
	 *  2. test whether or not player can see portal
	 *  3. get portal destination, do 2. again but from portal
	 */

	WorldMesh *sectorMesh = originSector->mesh;
	for ( unsigned int i = 0; i < sectorMesh->numFaces; ++i )
	{
		if ( !( sectorMesh->faces[ i ].flags & WORLD_FACE_FLAG_PORTAL ) && !( sectorMesh->faces[ i ].flags & WORLD_FACE_FLAG_MIRROR ) )
			continue;

		if ( !PlgIsBoxInsideView( camera->internal, &sectorMesh->faces[ i ].bounds ) )
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
		bounds.origin          = sector->staticObjects[ i ].transform.translation;
		if ( !PlgIsBoxInsideView( camera, &sector->staticObjects[ i ].mesh->bounds ) )
			continue;

		visibleMeshes[ *numMeshes ] = sector->mesh;
		numMeshes++;
	}

	// Shrink the array down before passing it back
	visibleMeshes = globalSystem.ReAlloc( visibleMeshes, sizeof( WorldMesh * ) * *numMeshes, true );
	return visibleMeshes;
}

static void DrawSector( WorldSector *sector, Camera *camera, bool simple )
{
#if 0
	unsigned int numVisibleMeshes;
	WorldMesh ** visibleMeshes = GetVisibleSubMeshesForSector( sector, camera, &numVisibleMeshes );
#endif

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
			if ( !PlgIsBoxInsideView( camera->internal, &face->bounds ) )
				continue;

			for ( unsigned int k = 0; k < face->numVertices; ++k )
			{
				PLGVertex *  vertex = &sector->mesh->vertices[ face->vertices[ k ] ];
				unsigned int v      = PlgAddMeshVertex( sector->mesh->drawMesh, vertex->position, vertex->normal, vertex->colour, vertex->st[ 0 ] );
				/* this shit is generated earlier in the process, and right now I'm not sure if it's
				 * appropriate to add to AddMeshVertex */
				sector->mesh->drawMesh->vertices[ v ].tangent   = vertex->tangent;
				sector->mesh->drawMesh->vertices[ v ].bitangent = vertex->bitangent;
			}

			PLGVertex vertices[ WORLD_FACE_MAX_SIDES ];
			memset( vertices, 0, sizeof( PLGVertex ) * WORLD_FACE_MAX_SIDES );

			unsigned int  numTriangles;
			unsigned int *indices  = ConvertFaceToTriangles( face, &numTriangles );
			unsigned int *curIndex = indices;
			for ( unsigned int k = 0; k < numTriangles; ++k )
			{
				PlgAddMeshTriangle( sector->mesh->drawMesh,
				                    curIndex[ 0 ] + sector->mesh->drawMesh->num_verts - face->numVertices,
				                    curIndex[ 1 ] + sector->mesh->drawMesh->num_verts - face->numVertices,
				                    curIndex[ 2 ] + sector->mesh->drawMesh->num_verts - face->numVertices );
				curIndex += 3;
			}
			globalSystem.Free( indices );

			g_gfxPerfStats.numFacesDrawn++;
		}

		if ( sector->mesh->drawMesh->num_triangles == 0 )
			return;

		Material *material = RM_CacheMaterial( "materials/engine/simple.mat", 0, true );
		RM_DrawMesh( material, sector->mesh->drawMesh );
		return;
	}

#if 0
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
#endif
}

/****************************************
 * RENDERING
 ****************************************/


/**
 * Draw scrolling clouds.
 */
void W_DrawSky( Camera *camera )
{
#if 0
	static Material *skyMaterial = NULL;
	if ( skyMaterial == NULL )
	{
		skyMaterial = RM_CacheMaterial( "materials/sky/cloudlayer00.mat", CACHE_GROUP_WORLD, true );
		if ( skyMaterial == NULL )
			PrintError( "Failed to load cloud layer!\n" );
	}

	static PLGVertex vertices[] = {
	        { .position = { 10.0f, 100.f, 100.0f },
					.colour   = PL_COLOUR_WHITE }, /* top right */
			{ .position = PLVector3( 10.0f, 200.0f, 200.0f ),
					.colour   = PLColourA( 0 ) }, /* top right far */
			{ .position = PLVector3( 10.0f, 100.0f, -100.0f ),
					.colour   = PL_COLOUR_WHITE }, /* lower right */
			{ .position = PLVector3( 10.0f, 200.0f, -200.0f ),
					.colour   = PLColourA( 0 ) }, /* lower right far */
			{ .position = PLVector3( 10.0f, -100.0f, -100.0f ),
					.colour   = PL_COLOUR_WHITE }, /* lower left */
			{ .position = PLVector3( 10.0f, -200.0f, -200.0f ),
					.colour   = PLColourA( 0 ) }, /* lower left far */
			{ .position = PLVector3( 10.0f, -100.0f, 100.0f ),
					.colour   = PL_COLOUR_WHITE }, /* top left */
			{ .position = PLVector3( 10.0f, -200.0f, 200.0f ),
					.colour   = PLColourA( 0 ) } }; /* top left far */
	static unsigned int indices[][ 3 ] = {
			/* corners */
			{ 2, 1, 0 },
			{ 3, 1, 2 },
			{ 4, 3, 2 },
			{ 5, 3, 4 },
			{ 6, 5, 4 },
			{ 7, 5, 6 },
			{ 0, 7, 6 },
			{ 1, 7, 0 },
			/* middle */
			{ 4, 2, 0 },
			{ 6, 4, 0 },
	};

	static PLGMesh *skyMesh = NULL;
	if ( skyMesh == NULL )
	{
		skyMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, plArrayElements( indices ), plArrayElements( vertices ) );
		if ( skyMesh == NULL )
			PrintError( "Failed to create sky mesh!\nPL: %s\n", PlGetError() );

		for ( unsigned int i = 0, curIndex = 0; i < plArrayElements( indices ); ++i )
			PlgSetMeshTrianglePosition( skyMesh, &curIndex, indices[ i ][ 0 ], indices[ i ][ 1 ], indices[ i ][ 2 ] );

		for ( unsigned int i = 0; i < plArrayElements( vertices ); ++i )
		{
			PlgSetMeshVertexPosition( skyMesh, i, PLVector3( vertices[ i ].position.y, vertices[ i ].position.x, vertices[ i ].position.z ) );
			PlgSetMeshVertexColour( skyMesh, i, vertices[ i ].colour );
		}
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );
	PlgSetDepthMask( false );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlTranslateMatrix( PLVector3( camera->position.x, camera->position.y + 10.0f, camera->position.z ) );

	/* todo: do this in shader... */
	PLVector2 skyOffset;
	skyOffset.x = Engine_GetNumTicks() / 1000.0f;
	skyOffset.y = Engine_GetNumTicks() / 1000.0f;
	PlgGenerateTextureCoordinates( skyMesh->vertices, skyMesh->num_verts, skyOffset, PLVector2( 0.75f, 0.75f ) );

	RM_DrawMesh( skyMaterial, skyMesh );

	/* todo: do this in shader... */
	skyOffset.x = ( Engine_GetNumTicks() / 100.0f ) * -1;
	skyOffset.y = Engine_GetNumTicks() / 100.0f;
	PlgGenerateTextureCoordinates( skyMesh->vertices, skyMesh->num_verts, skyOffset, PLVector2( 0.45f, 0.45f ) );

	RM_DrawMesh( skyMaterial, skyMesh );

	PlPopMatrix();

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgSetDepthMask( true );
#endif
}

static void W_Debug_DrawSectorVolumes( World *world, WorldSector *originSector, Camera *camera )
{
	if ( originSector == NULL )
		originSector = W_GetSectorByGlobalOrigin( world, &camera->internal->position );

	unsigned int  numVisibleSectors;
	WorldSector **visibleSectors = GetVisibleSectors( world, originSector, camera, &numVisibleSectors );

	srand( numVisibleSectors );

	for ( unsigned int i = 0; i < numVisibleSectors; ++i )
	{
		WorldSector *sector = visibleSectors[ i ];
		PlgDrawBoundingVolume( &sector->bounds, PLColourRGB( rand() % 255, rand() % 255, rand() % 255 ) );
	}
}

void W_Draw( Camera *camera, World *world, WorldSector *originSector )
{
	PROFILE_START( PROFILE_DRAW_WORLD );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	W_DrawSky( camera );

	if ( world != NULL )
	{
		CVar( "world.drawSectorVolumes", drawSectorVolumes );
		if ( drawSectorVolumes != NULL && drawSectorVolumes->b_value )
			W_Debug_DrawSectorVolumes( world, originSector, camera );
		else
		{
			if ( originSector == NULL )
				originSector = W_GetSectorByGlobalOrigin( world, &camera->internal->position );

			if ( originSector != NULL )
			{
				unsigned int  numVisibleSectors;
				WorldSector **visibleSectors = GetVisibleSectors( world, originSector, camera, &numVisibleSectors );
				for ( unsigned int i = 0; i < numVisibleSectors; ++i )
					DrawSector( visibleSectors[ i ], camera, false );

				globalSystem.Free( visibleSectors );
			}
		}
	}

	PlPopMatrix();

	PROFILE_END( PROFILE_DRAW_WORLD );
}
