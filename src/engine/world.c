/* Copyright (C) Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_filesystem.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_mesh.h>

#include "yin.h"
#include "world.h"

#include "common/CFWNode.h"

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

	uint8_t flags; /* portal, mirror, skip etc. */

	unsigned int parentSector;
	unsigned int targetSectorFace; /* if portal */

	PLCollisionAABB bounds;
} WorldFace;

typedef struct WorldSector {
	WorldFace *faces;
	unsigned int numFaces;

	PLCollisionAABB bounds;
} WorldSector;

typedef struct WorldProperty {
	char label[ WORLD_PROP_TAG_LENGTH ];
	NLPropertyType propType;
	NLPropertyData_U data;
} WorldProperty;

typedef struct WorldMesh {
	char identifier[ WORLD_PROP_TAG_LENGTH ];

	Material **materials;
	unsigned int numMaterials;

	PLGVertex *vertices;
	unsigned int numVertices;

	WorldFace *faces;
	unsigned int numFaces;
} WorldMesh;

typedef struct World {
	WorldMesh *meshes;
	unsigned int numMeshes;

	WorldSector *sectors;
	unsigned int numSectors;

	/* additional generic properties */
	WorldProperty *properties;
	unsigned int numProperties;
} World;

static float *DS_DeserializeVector( NLNode *in, float *out, uint8_t numElements ) {
	u_assert( numElements != 0 && numElements < 4 );
	const char *elements[] = { "x", "y", "z", "w" };
	for ( uint8_t i = 0; i < numElements; ++i ) {
		out[ i ] = NL_GetF32ByName( in, elements[ i ], 0.0f );
	}

	return out;
}

static PLVector2 *DS_DeserializeVector2( NLNode *in, PLVector2 *out ) { return ( PLVector2 * ) DS_DeserializeVector( in, ( float * ) out, 2 ); }
static PLVector3 *DS_DeserializeVector3( NLNode *in, PLVector3 *out ) { return ( PLVector3 * ) DS_DeserializeVector( in, ( float * ) out, 3 ); }
static PLVector4 *DS_DeserializeVector4( NLNode *in, PLVector4 *out ) { return ( PLVector4 * ) DS_DeserializeVector( in, ( float * ) out, 4 ); }
static PLQuaternion *DS_DeserializeQuaternion( NLNode *in, PLQuaternion *out ) { return ( PLQuaternion * ) DS_DeserializeVector( in, ( float * ) out, 4 ); }

static PLColour *DS_DeserializeColour( NLNode *in, PLColour *out ) {
	const char *elements[] = { "r", "g", "b", "a" };
	for ( uint8_t i = 0; i < 4; ++i ) {
		PlColourIndex( out, i ) = NL_GetI32ByName( in, elements[ i ], 255 );
	}

	return out;
}

static PLGVertex *DS_DeserializeVertex( NLNode *in, PLGVertex *out ) {
	NLNode *n;
	if ( ( n = NL_GetChildByName( in, "position" ) ) != NULL ) {
		DS_DeserializeVector3( n, &out->position );
	}
	if ( ( n = NL_GetChildByName( in, "colour" ) ) != NULL ) {
		DS_DeserializeColour( n, &out->colour );
	}
	if ( ( n = NL_GetChildByName( in, "normal" ) ) != NULL ) {
		DS_DeserializeVector3( n, &out->normal );
	}
	if ( ( n = NL_GetChildByName( in, "tangent" ) ) != NULL ) {
		DS_DeserializeVector3( n, &out->tangent );
	}
	if ( ( n = NL_GetChildByName( in, "bitangent" ) ) != NULL ) {
		DS_DeserializeVector3( n, &out->bitangent );
	}
	if ( ( n = NL_GetChildByName( in, "uv" ) ) != NULL ) {
		DS_DeserializeVector2( n, &out->st[ 0 ] );
	}
	return out;
}

void W_DeserializeMaterials( NLNode *meshNode, WorldMesh *meshPtr ) {
	NLNode *materialsList = NL_GetChildByName( meshNode, "materials" );
	if ( materialsList == NULL ) {
		PrintWarn( "No materials for mesh: %s!\n", meshPtr->identifier );
		return;
	}

	meshPtr->numMaterials = NL_GetNumOfChildren( materialsList );
	meshPtr->materials = globalSystem.CAlloc( meshPtr->numMaterials, sizeof( Material * ), true );
	NLNode *materialNode = NL_GetFirstChild( materialsList );
	for ( unsigned int i = 0; i < meshPtr->numMaterials; ++i ) {
		if ( materialNode == NULL ) {
			PrintWarn( "Hit an invalid material index!\n" );
			meshPtr->numMaterials = i;
			break;
		}

		const char *materialPath = NL_GetStr( materialNode );
		u_assert( materialPath != NULL );
		meshPtr->materials[ i ] = RM_CacheMaterial( materialPath, CACHE_GROUP_WORLD, true );
		materialNode = NL_GetNextChild( materialNode );
	}
}

void W_DeserializeVertices( NLNode *meshNode, WorldMesh *meshPtr ) {
	NLNode *verticesList = NL_GetChildByName( meshNode, "vertices" );
	if ( verticesList == NULL ) {
		PrintWarn( "No vertices for mesh: %s!\n", meshPtr->identifier );
		return;
	}

	meshPtr->numVertices = NL_GetNumOfChildren( verticesList );
	meshPtr->vertices = globalSystem.CAlloc( meshPtr->numVertices, sizeof( PLGVertex ), true );
	NLNode *vertexNode = NL_GetFirstChild( verticesList );
	for ( unsigned int i = 0; i < meshPtr->numVertices; ++i ) {
		if ( vertexNode == NULL ) {
			PrintWarn( "Hit an invalid vertex index!\n" );
			meshPtr->numVertices = i;
			break;
		}

		DS_DeserializeVertex( vertexNode, &meshPtr->vertices[ i ] );
	}
}

void W_DeserializeFaces( NLNode *meshNode, WorldMesh *meshPtr ) {
	NLNode *facesList = NL_GetChildByName( meshNode, "faces" );
	if ( facesList == NULL ) {
		PrintWarn( "No faces for mesh: %s!\n", meshPtr->identifier );
		return;
	}

	meshPtr->numFaces = NL_GetNumOfChildren( facesList );
	meshPtr->faces = globalSystem.CAlloc( meshPtr->numFaces, sizeof( WorldFace ), true );
	NLNode *faceNode = NL_GetFirstChild( facesList );
	for ( unsigned int i = 0; i < meshPtr->numFaces; ++i ) {
		if ( faceNode == NULL ) {
			PrintWarn( "Hit an invalid face index!\n" );
			meshPtr->numFaces = i;
			break;
		}

		int materialIndex = NL_GetI32ByName( faceNode, "materialIndex", -1 );
		if ( materialIndex >= 0 && materialIndex < meshPtr->numMaterials ) {
			meshPtr->faces[ i ].material = meshPtr->materials[ materialIndex ];
		}
		meshPtr->faces[ i ].materialAngle = NL_GetF32ByName( faceNode, "materialAngle", 0.0f );
		NLNode *n;
		if ( ( n = NL_GetChildByName( faceNode, "materialOffset" ) ) != NULL ) {
			DS_DeserializeVector2( n, &meshPtr->faces[ i ].materialOffset );
		}
		if ( ( n = NL_GetChildByName( faceNode, "materialScale" ) ) != NULL ) {
			DS_DeserializeVector2( n, &meshPtr->faces[ i ].materialScale );
		}

		if ( ( n = NL_GetChildByName( faceNode, "vertices" ) ) != NULL ) {
			meshPtr->faces[ i ].numVertices = NL_GetNumOfChildren( n );
			if ( meshPtr->faces[ i ].numVertices >= WORLD_FACE_MAX_SIDES ) {
				PrintWarn( "Too many vertices for face: %d!\n", i );
				meshPtr->faces[ i ].numVertices = WORLD_FACE_MAX_SIDES;
			}

			if ( meshPtr->faces[ i ].numVertices > 0 ) {
				NL_GetI32Array( n, meshPtr->faces[ i ].vertices );
			}
		}

		meshPtr->faces[ i ].flags = NL_GetI32ByName( faceNode, "flags", 0 );

		faceNode = NL_GetNextChild( faceNode );
	}
}

void W_DeserializeMesh( NLNode *meshNode, WorldMesh *meshPtr ) {
	const char *identifier = NL_GetStrByName( meshNode, "identifier", NULL );
	if ( identifier == NULL ) {
		PlGenerateUniqueIdentifier( meshPtr->identifier, sizeof( meshPtr->identifier ) );
	} else {
		snprintf( meshPtr->identifier, sizeof( meshPtr->identifier ), "%s", identifier );
	}

	W_DeserializeMaterials( meshNode, meshPtr );
	W_DeserializeVertices( meshNode, meshPtr );
	W_DeserializeFaces( meshNode, meshPtr );
}

static World *W_DeserializeWorld( NLNode *in, World *out ) {
	Print( "Deserializing world\n" );

	NLNode *propertyList = NL_GetChildByName( in, "properties" );
	if ( propertyList != NULL ) {
		out->numProperties = NL_GetNumOfChildren( propertyList );
		out->properties = globalSystem.CAlloc( out->numProperties, sizeof( WorldProperty ), true );
		NLNode *c = NL_GetFirstChild( propertyList );
		for ( unsigned int i = 0; i < out->numProperties; ++i ) {
			if ( c == NULL ) {
				PrintWarn( "Hit an invalid property index!\n" );
				out->numProperties = i;
				break;
			}

			// TODO

			c = NL_GetNextChild( c );
		}
	}

	NLNode *meshList = NL_GetChildByName( in, "meshes" );
	if ( meshList != NULL ) {
		out->numMeshes = NL_GetNumOfChildren( meshList );
		out->meshes = globalSystem.CAlloc( out->numMeshes, sizeof( WorldMesh ), true );
		NLNode *c = NL_GetFirstChild( meshList );
		for ( unsigned int i = 0; i < out->numMeshes; ++i ) {
			if ( c == NULL ) {
				PrintWarn( "Hit an invalid mesh index: %d\n", i );
				out->numMeshes = i;
				break;
			}

			W_DeserializeMesh( c, &out->meshes[ i ] );
		}
	}
}

World *W_LoadWorld( const char *path ) {
	NLNode *node = NL_LoadFile( path, "world" );
	if ( node == NULL ) {
		PrintWarn( "Failed to load world: %s\n", path );
		return NULL;
	}

	World *world = globalSystem.MAlloc( sizeof( World ), true );
	W_DeserializeWorld( node, world );
}

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

	CVar( "world.forceSimple", forceSimple );

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
