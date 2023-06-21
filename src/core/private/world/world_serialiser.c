// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "ape_private.h"
#include "world.h"

#include "client/renderer/renderer_material.h"

static void SerialiseFace( const ApeWorldFace *face, const ApeWorldMesh *mesh, NdBranch *root, const char *name )
{
	NdBranch *node = ndPushBackObject( root, name );
	ndPushBackString( node, "material", apeGetMaterialPath( face->material ) );
	ndPushBackF32( node, "materialAngle", face->materialAngle );
	ndDS_SerializeVector2( node, "materialOffset", &face->materialOffset );
	ndDS_SerializeVector2( node, "materialScale", &face->materialScale );
	ndPushBackI8( node, "flags", ( int8_t ) face->flags );
	ndDS_SerializeVector3( node, "normal", &face->normal );
}

static void SerialiseFaces( const ApeWorldMesh *mesh, NdBranch *root )
{
#if 0
	NLNode *faceListNode = NL_PushBackObjArray( root, "faces" );
	for ( unsigned int i = 0; i < mesh->numFaces; ++i )
	{
		SerialiseFace( &mesh->faces[ i ], mesh, faceListNode, NULL );
	}
#endif
}

static void SerialiseMesh( const ApeWorldMesh *mesh, NdBranch *root, const char *name )
{
	NdBranch *meshNode = ndPushBackObject( root, name );
	if ( *mesh->id != '\0' )
		ndPushBackString( meshNode, "id", mesh->id );

	ndDS_SerializeCollisionAABB( meshNode, "bounds", &mesh->bounds );

	SerialiseFaces( mesh, meshNode );
}

static void SerialiseMeshes( const ApeWorld *world, NdBranch *root )
{
	NdBranch *meshListNode = ndPushBackObjectArray( root, "meshes" );

	unsigned int numMeshes = PlGetNumVectorArrayElements( world->meshes );
	for ( unsigned int i = 0; i < numMeshes; ++i )
	{
		SerialiseMesh( ( ApeWorldMesh * ) PlGetVectorArrayElementAt( world->meshes, i ), meshListNode, NULL );
	}
}

void apeSerializeWorld( const ApeWorld *world, NdBranch *root )
{
	ndPushBackI32( root, "version", APE_WORLD_VERSION );
	ndPushBackBranch( root, world->globalProperties );

	SerialiseMeshes( world, root );
}
