/* ======================================================================
 * PkgMan, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * Purpose: Convert models from PLY, STL or other into .node:model format.
 * ====================================================================*/

#include <plcore/pl_parse.h>

#include "pkgman.h"

/* ======================================================================
 * PLMModel > Node Conversion
 * ====================================================================*/

#if 0 /* todo: return to this... */
void MDL_OutlineVertexDescriptor( NLNode *parent, const PLGMesh *mesh )
{
    bool hasAlpha = false;
	bool hasColour = false;
	bool hasNormal = false;
    for ( unsigned int i = 0; i < mesh->num_verts; ++i )
    {
		if ( !hasAlpha && mesh->vertices[ i ].colour.a != 255 )
			hasAlpha = true;
		if ( !hasColour && (
		     mesh->vertices[ i ].colour.r != 255 &&
		     mesh->vertices[ i ].colour.g != 255 &&
		     mesh->vertices[ i ].colour.b != 255 ) )
			hasColour = true;
    }

    NLNode *vertexDescriptor = NL_PushBackObj( parent, "vertexDescriptor" );
    {
        /* position */
        NL_PushBackStr( parent, "posX", "float" );
        NL_PushBackStr( parent, "posY", "float" );
        NL_PushBackStr( parent, "posZ", "float" );

        /* normal */
        NL_PushBackStr( parent, "normX", "float" );
        NL_PushBackStr( parent, "normY", "float" );
        NL_PushBackStr( parent, "normZ", "float" );

        /* texture uv */
        NL_PushBackStr( parent, "u", "float" );
        NL_PushBackStr( parent, "v", "float" );

        /* colour */
		if ( hasColour )
		{
			NL_PushBackStr( parent, "r", "uint8" );
			NL_PushBackStr( parent, "g", "uint8" );
			NL_PushBackStr( parent, "b", "uint8" );
		}
        if ( hasAlpha )
            NL_PushBackStr( parent, "a", "uint8" );
    }
}
#endif

void MDL_SerializePlatformMesh( NLNode *parent, const PLGMesh *mesh )
{
	NLNode *node = NL_PushBackObj( parent, "mesh" );

	NL_PushBackI32( node, "materialIndex", mesh->materialIndex );

#if 0
    MDL_OutlineVertexDescriptor( node, mesh );
#endif

	NLNode *vertexArray = NL_PushBackObjArray( node, "vertices" );
	for ( uint32_t j = 0; j < mesh->num_verts; ++j )
	{
		NLNode *vertex = NL_PushBackObj( vertexArray, "vertex" );

		NLNode *vertexChild;
		vertexChild = NL_PushBackObj( vertex, "position" );
		{
			NL_PushBackF32( vertexChild, "x", mesh->vertices[ j ].position.x );
			NL_PushBackF32( vertexChild, "y", mesh->vertices[ j ].position.y );
			NL_PushBackF32( vertexChild, "z", mesh->vertices[ j ].position.z );
		}
		vertexChild = NL_PushBackObj( vertex, "textureCoords" );
		{
			NL_PushBackF32( vertexChild, "x", mesh->vertices[ j ].st[ 0 ].x );
			NL_PushBackF32( vertexChild, "y", mesh->vertices[ j ].st[ 0 ].y );
		}
		if ( !PlCompareVector3( &mesh->vertices[ j ].normal, &pl_vecOrigin3 ) )
		{
			vertexChild = NL_PushBackObj( vertex, "normal" );
			NL_PushBackF32( vertexChild, "x", mesh->vertices[ j ].normal.x );
			NL_PushBackF32( vertexChild, "y", mesh->vertices[ j ].normal.y );
			NL_PushBackF32( vertexChild, "z", mesh->vertices[ j ].normal.z );
		}
		if ( !PlCompareColour( mesh->vertices[ j ].colour, PLColour( 255, 255, 255, 255 ) ) )
		{
			vertexChild = NL_PushBackObj( vertex, "colour" );
			NL_PushBackI8( vertexChild, "r", mesh->vertices[ j ].colour.r );
			NL_PushBackI8( vertexChild, "g", mesh->vertices[ j ].colour.g );
			NL_PushBackI8( vertexChild, "b", mesh->vertices[ j ].colour.b );
			NL_PushBackI8( vertexChild, "a", mesh->vertices[ j ].colour.a );
		}
		if ( mesh->vertices[ j ].bone_weight != 0.0f )
		{
			NL_PushBackF32( vertex, "boneWeight", mesh->vertices[ j ].bone_weight );
			NL_PushBackI32( vertex, "boneIndex", mesh->vertices[ j ].bone_index );
		}
	}

	NLNode *triangleArray = NL_PushBackObjArray( node, "faces" );
	for ( uint32_t j = 0; j < mesh->num_indices; j += 3 )
	{
		NLNode *face = NL_PushBackObj( triangleArray, "face" );
		NL_PushBackI32Array( face, "indices", ( int32_t * ) ( mesh->indices + j ), 3 );
	}
}

NLNode *MDL_ConvertPlatformModelToNodeModel( const PLMModel *model )
{
	NLNode *root = NL_PushBackObj( NULL, "model" );

	NL_PushBackI8( root, "version", 1 );

	NLNode *materialArray = NL_PushBackStrArray( root, "materials", NULL, 0 );
	for ( unsigned int i = 0; i < model->numMaterials; ++i )
		NL_PushBackStr( materialArray, NULL, model->materials[ i ] );

	NLNode *meshArray = NL_PushBackObjArray( root, "meshes" );
	for ( uint8_t i = 0; i < model->numMeshes; ++i )
		MDL_SerializePlatformMesh( meshArray, model->meshes[ i ] );

	return root;
}
