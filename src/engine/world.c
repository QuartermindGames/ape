/* Copyright (C) Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_filesystem.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_mesh.h>

#include "yin.h"
#include "world.h"

#include "common/node.h"

#include "renderer/renderer.h"
#include "renderer/material.h"

typedef struct WorldFace {
	PLVector3 normal;

	Material *material;
	float materialAngle;
	PLVector2 materialOffset;
	PLVector2 materialScale;

	unsigned int vertices[ WORLD_FACE_MAX_SIDES ];
	uint8_t numVertices;

	PLCollisionAABB bounds;

	uint8_t flags; /* portal, mirror, skip etc. */

	unsigned int parentSector;
	unsigned int targetSectorFace; /* if portal */
} WorldFace;

typedef struct WorldSector {
	WorldFace *faces;
	unsigned int numFaces;

	PLCollisionAABB bounds;
} WorldSector;

typedef struct WorldProperty {
	char label[ WORLD_PROP_TAG_LENGTH ];
	char value[ WORLD_PROP_VALUE_LENGTH ];
} WorldProperty;

typedef struct World {
	struct Material **materials;
	unsigned int numMaterials;

	PLGVertex *vertices;
	unsigned int numVertices;

	WorldSector *sectors;
	unsigned int numSectors;

	/* additional generic properties */
	WorldProperty *properties;
	unsigned int numProperties;
} World;

const char *W_GetGlobalPropertyValue( const World *world, const char *label ) {
	for ( unsigned int i = 0; i < world->numProperties; ++i ) {
		if ( pl_strcasecmp( world->properties[ i ].label, label ) != 0 ) {
			continue;
		}

		return world->properties[ i ].value;
	}

	return NULL;
}

static PLGMesh *triangleMesh = NULL;

/**
 * Fetch the normal for the specified face.
 */
PLVector3 W_GetFaceNormal( const WorldFace *face ) {
	return face->normal;
}

/**
 * Fetch the origin point of the face in world-coordinates.
 */
PLVector3 W_GetFaceOrigin( const WorldFace *face ) {
	return face->bounds.absOrigin;
}

/**
 * Fetch the flags specified for the face.
 */
uint8_t W_GetFaceFlags( const WorldFace *face ) {
	return face->flags;
}

/****************************************
 * SECTOR
 ****************************************/

static unsigned int GetNumOfFaceTriangles( const WorldFace *face ) {
	if ( face->numVertices < 3 ) {
		return 0;
	}

	return face->numVertices - 2;
}

static unsigned int *ConvertFaceToTriangles( const WorldFace *face, unsigned int *numTriangles ) {
	*numTriangles = GetNumOfFaceTriangles( face );
	if ( *numTriangles == 0 ) {
		return NULL;
	}

	unsigned int *indices = pl_malloc( sizeof( unsigned int ) * ( *numTriangles * 3 ) );
	unsigned int *index = indices;
	for ( unsigned int i = 1; i + 1 < face->numVertices; ++i ) {
		index[ 0 ] = 0;
		index[ 1 ] = i;
		index[ 2 ] = i + 1;
		index += 3;
	}

	return indices;
}

void W_DrawSector( World *world, unsigned int sectorId, PLGCamera *camera, bool simple ) {
	unsigned int numFaces;
	WorldFace *faces = W_GetFacesForSector( sectorId, &numFaces );
	if ( faces == NULL || numFaces == 0 ) {
		PrintWarn( "Invalid number of faces in sector %d!\n", sectorId );
		return;
	}

	CVar( "world/force_simple", forceSimple );

	if ( simple || forceSimple->b_value ) {
		for ( unsigned int j = 0; j < numFaces; ++j ) {
			WorldFace *face = &faces[ j ];
			face->bounds.origin = PLVector3( 0.0f, 0.0f, 0.0f );

			/* check the face is actually visible */
			if ( !PlgIsBoxInsideView( camera, &face->bounds ) ) {
				continue;
			}

			for ( unsigned int k = 0; k < face->numVertices; ++k ) {
				PLGVertex *vertex = &world->vertices[ face->vertices[ k ] ];
				unsigned int v = PlgAddMeshVertex( triangleMesh, vertex->position, vertex->normal, vertex->colour, vertex->st[ 0 ] );
				/* this shit is generated earlier in the process, and right now I'm not sure if it's
				 * appropriate to add to AddMeshVertex */
				triangleMesh->vertices[ v ].tangent = vertex->tangent;
				triangleMesh->vertices[ v ].bitangent = vertex->bitangent;
			}

			PLGVertex vertices[ WORLD_FACE_MAX_SIDES ];
			memset( vertices, 0, sizeof( PLGVertex ) * WORLD_FACE_MAX_SIDES );

			unsigned int numTriangles;
			unsigned int *indices = ConvertFaceToTriangles( face, &numTriangles );
			unsigned int *curIndex = indices;
			for ( unsigned int k = 0; k < numTriangles; ++k ) {
				PlgAddMeshTriangle( triangleMesh,
				                   curIndex[ 0 ] + triangleMesh->num_verts - face->numVertices,
				                   curIndex[ 1 ] + triangleMesh->num_verts - face->numVertices,
				                   curIndex[ 2 ] + triangleMesh->num_verts - face->numVertices );
				curIndex += 3;
			}
			globalSystem.Free( indices );

			g_gfxPerfStats.numFacesDrawn++;
		}

		if ( triangleMesh->num_triangles == 0 ) {
			return;
		}

		Material *material = RM_CacheMaterial( "materials/engine/simple.mat", CACHE_GROUP_STATIC, true );
		RM_DrawMesh( material, triangleMesh );
		return;
	}

	/* batch everything by material */
	for ( unsigned int i = 0; i < world->numMaterials; ++i ) {
		for ( unsigned int j = 0; j < numFaces; ++j ) {
			WorldFace *face = &faces[ j ];
			if ( face->material != world->materials[ i ] ) {
				continue;
			}

			face->bounds.origin = PLVector3( 0.0f, 0.0f, 0.0f );

			/* check the face is actually visible */
			if ( !PlgIsBoxInsideView( camera, &face->bounds ) ) {
				continue;
			}

			for ( unsigned int k = 0; k < face->numVertices; ++k ) {
				PLGVertex *vertex = &world->vertices[ face->vertices[ k ] ];
				unsigned int v = PlgAddMeshVertex( triangleMesh, vertex->position, vertex->normal, vertex->colour, vertex->st[ 0 ] );
				/* this shit is generated earlier in the process, and right now I'm not sure if it's
				 * appropriate to add to AddMeshVertex */
				triangleMesh->vertices[ v ].tangent = vertex->tangent;
				triangleMesh->vertices[ v ].bitangent = vertex->bitangent;
			}

			PLGVertex vertices[ WORLD_FACE_MAX_SIDES ];
			memset( vertices, 0, sizeof( PLGVertex ) * WORLD_FACE_MAX_SIDES );

			unsigned int numTriangles;
			unsigned int *indices = ConvertFaceToTriangles( face, &numTriangles );
			unsigned int *curIndex = indices;
			for ( unsigned int k = 0; k < numTriangles; ++k ) {
				PlgAddMeshTriangle( triangleMesh,
				                   curIndex[ 0 ] + triangleMesh->num_verts - face->numVertices,
				                   curIndex[ 1 ] + triangleMesh->num_verts - face->numVertices,
				                   curIndex[ 2 ] + triangleMesh->num_verts - face->numVertices );
				curIndex += 3;
			}
			globalSystem.Free( indices );

			g_gfxPerfStats.numFacesDrawn++;
		}

		if ( triangleMesh->num_triangles == 0 ) {
			continue;
		}

		RM_DrawMesh( world->materials[ i ], triangleMesh );
	}
}

/****************************************
 * SECTOR
 ****************************************/

void W_Draw( World *world, PLGCamera *camera ) {
	PROFILE_START( PROFILE_DRAW_MAP );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();



	PlPopMatrix();

	PROFILE_END( PROFILE_DRAW_MAP );
}
