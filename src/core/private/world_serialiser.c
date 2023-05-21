// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "core_private.h"
#include "world.h"

#include "client/renderer/renderer_material.h"

static void SerialiseFace( const OgeWorldFace *face, const OgeWorldMesh *mesh, YNNodeBranch *root, const char *name )
{
	YNNodeBranch *node = YnNode_PushBackObject( root, name );
	YnNode_PushBackString( node, "material", ogeMaterial_GetPath( face->material ) );
	YnNode_PushBackF32( node, "materialAngle", face->materialAngle );
	NL_DS_SerializeVector2( node, "materialOffset", &face->materialOffset );
	NL_DS_SerializeVector2( node, "materialScale", &face->materialScale );
	YnNode_PushBackI8( node, "flags", ( int8_t ) face->flags );
	NL_DS_SerializeVector3( node, "normal", &face->normal );
}

static void SerialiseFaces( const OgeWorldMesh *mesh, YNNodeBranch *root )
{
#if 0
	NLNode *faceListNode = NL_PushBackObjArray( root, "faces" );
	for ( unsigned int i = 0; i < mesh->numFaces; ++i )
	{
		SerialiseFace( &mesh->faces[ i ], mesh, faceListNode, NULL );
	}
#endif
}

static void SerialiseMesh( const OgeWorldMesh *mesh, YNNodeBranch *root, const char *name )
{
	YNNodeBranch *meshNode = YnNode_PushBackObject( root, name );
	if ( *mesh->id != '\0' )
		YnNode_PushBackString( meshNode, "id", mesh->id );

	NL_DS_SerializeCollisionAABB( meshNode, "bounds", &mesh->bounds );

	SerialiseFaces( mesh, meshNode );
}

static void SerialiseMeshes( const OgeWorld *world, YNNodeBranch *root )
{
	YNNodeBranch *meshListNode = YnNode_PushBackObjectArray( root, "meshes" );

	unsigned int numMeshes = PlGetNumVectorArrayElements( world->meshes );
	for ( unsigned int i = 0; i < numMeshes; ++i )
	{
		SerialiseMesh( ( OgeWorldMesh * ) PlGetVectorArrayElementAt( world->meshes, i ), meshListNode, NULL );
	}
}

static void SerialiseSectors( const OgeWorld *world, YNNodeBranch *root )
{
	YNNodeBranch *sectorListNode = YnNode_PushBackObjectArray( root, "sectors" );
	for ( unsigned int i = 0; i < world->numSectors; ++i )
	{
		YNNodeBranch *sectorNode = YnNode_PushBackObject( sectorListNode, NULL );

		if ( *world->sectors[ i ].id != '\0' )
			YnNode_PushBackString( sectorNode, "id", world->sectors[ i ].id );

		if ( world->sectors[ i ].mesh != NULL && *world->sectors[ i ].mesh->id != '\0' )
			YnNode_PushBackString( sectorNode, "meshId", world->sectors[ i ].mesh->id );

		NL_DS_SerializeCollisionAABB( sectorNode, "bounds", &world->sectors[ i ].bounds );
	}
}

void YnCore_WorldSerialiser_Begin( const OgeWorld *world, YNNodeBranch *root )
{
	YnNode_PushBackI32( root, "version", YN_CORE_WORLD_VERSION );
	YnNode_PushBackBranch( root, world->globalProperties );

	SerialiseMeshes( world, root );
	SerialiseSectors( world, root );
}
