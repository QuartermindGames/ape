// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "core_private.h"
#include "world.h"

#include "client/renderer/renderer_material.h"

static void SerialiseFace( const OgeWorldFace *face, const OgeWorldMesh *mesh, NdBranch *root, const char *name )
{
	NdBranch *node = ndPushBackObject( root, name );
	ndPushBackString( node, "material", ogeMaterial_GetPath( face->material ) );
	ndPushBackF32( node, "materialAngle", face->materialAngle );
	ndDS_SerializeVector2( node, "materialOffset", &face->materialOffset );
	ndDS_SerializeVector2( node, "materialScale", &face->materialScale );
	ndPushBackI8( node, "flags", ( int8_t ) face->flags );
	ndDS_SerializeVector3( node, "normal", &face->normal );
}

static void SerialiseFaces( const OgeWorldMesh *mesh, NdBranch *root )
{
#if 0
	NLNode *faceListNode = NL_PushBackObjArray( root, "faces" );
	for ( unsigned int i = 0; i < mesh->numFaces; ++i )
	{
		SerialiseFace( &mesh->faces[ i ], mesh, faceListNode, NULL );
	}
#endif
}

static void SerialiseMesh( const OgeWorldMesh *mesh, NdBranch *root, const char *name )
{
	NdBranch *meshNode = ndPushBackObject( root, name );
	if ( *mesh->id != '\0' )
		ndPushBackString( meshNode, "id", mesh->id );

	ndDS_SerializeCollisionAABB( meshNode, "bounds", &mesh->bounds );

	SerialiseFaces( mesh, meshNode );
}

static void SerialiseMeshes( const OgeWorld *world, NdBranch *root )
{
	NdBranch *meshListNode = ndPushBackObjectArray( root, "meshes" );

	unsigned int numMeshes = PlGetNumVectorArrayElements( world->meshes );
	for ( unsigned int i = 0; i < numMeshes; ++i )
	{
		SerialiseMesh( ( OgeWorldMesh * ) PlGetVectorArrayElementAt( world->meshes, i ), meshListNode, NULL );
	}
}

static void SerialiseSectors( const OgeWorld *world, NdBranch *root )
{
	NdBranch *sectorListNode = ndPushBackObjectArray( root, "sectors" );
	for ( unsigned int i = 0; i < world->numSectors; ++i )
	{
		NdBranch *sectorNode = ndPushBackObject( sectorListNode, NULL );

		if ( *world->sectors[ i ].id != '\0' )
			ndPushBackString( sectorNode, "id", world->sectors[ i ].id );

		if ( world->sectors[ i ].mesh != NULL && *world->sectors[ i ].mesh->id != '\0' )
			ndPushBackString( sectorNode, "meshId", world->sectors[ i ].mesh->id );

		ndDS_SerializeCollisionAABB( sectorNode, "bounds", &world->sectors[ i ].bounds );
	}
}

void YnCore_WorldSerialiser_Begin( const OgeWorld *world, NdBranch *root )
{
	ndPushBackI32( root, "version", YN_CORE_WORLD_VERSION );
	ndPushBackBranch( root, world->globalProperties );

	SerialiseMeshes( world, root );
	SerialiseSectors( world, root );
}
