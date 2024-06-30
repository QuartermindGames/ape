// Copyright © 2020-2024 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "renderer.h"
#include "world/world.h"

//TODO: eventually we should do away with this
#define MAX_MATERIALS_PER_PASS 256
#define MAX_SUB_MESHES         8192
static int subMeshes[ MAX_MATERIALS_PER_PASS ][ MAX_SUB_MESHES ];
static int firstSubMeshes[ MAX_MATERIALS_PER_PASS ][ MAX_SUB_MESHES ];
static int numSubMeshes[ MAX_MATERIALS_PER_PASS ];

static void draw_face_wireframe( ApeWorld *world, ApeWorldFace *face )
{
	unsigned int numVertices;
	ApeWorldFaceVertex **vertices = ( ApeWorldFaceVertex ** ) PlGetVectorArrayDataEx( face->vertices, &numVertices );
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		ApeWorldVertex *a = vertices[ i ]->u;
		PlgImmPushVertex( a->position.x, a->position.y, a->position.z );
		if ( face->portal != NULL )
		{
			PlgImmColour( 255, 0, 255, 255 );
		}
		else
		{
			PlgImmColour( 255, 255, 255, 255 );
		}

		ApeWorldVertex *b = ( ( i + 1 ) < numVertices ) ? vertices[ i + 1 ]->u : vertices[ 0 ]->u;
		PlgImmPushVertex( b->position.x, b->position.y, b->position.z );
		if ( face->portal != NULL )
		{
			PlgImmColour( 255, 0, 255, 255 );
		}
		else
		{
			PlgImmColour( 255, 255, 255, 255 );
		}
	}
}

static void draw_room_wireframe( ApeWorld *world, ApeRoom *room )
{
	unsigned int numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );
	for ( unsigned int j = 0; j < numFaces; ++j )
	{
		draw_face_wireframe( world, faces[ j ] );
	}
}

/**
 * World is drawn using polygons, rather than straight up triangles,
 * so to more accurately display it in wireframe, we'll need to render
 * it in such a mode ourselves. This is mostly for the sake of the
 * editor.
 */
void ape_world_draw_wireframe( ApeWorld *world, ApeCamera *camera )
{
	assert( ( camera != NULL ) && ( world != NULL ) );

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgSetTexture( NULL, 0 );

	//PlMatrixMode( PL_MODELVIEW_MATRIX );
	//PlPushMatrix();
	//PlLoadMatrix( &world->root->transform );

	PlgImmBegin( PLG_MESH_LINES );
	if ( camera->room == NULL || ape_config_.world.showAllRooms )
	{
		// Just go over the initial children to determine if they're rooms
		PLLinkedListNode *node = PlGetFirstNode( world->base.children );
		while ( node != NULL )
		{
			ApeWorldNode *worldNode = PlGetLinkedListNodeUserData( node );
			if ( worldNode->type == APE_WORLD_NODE_TYPE_ROOM )
			{
				draw_room_wireframe( world, worldNode->data );
			}

			node = PlGetNextLinkedListNode( node );
		}
	}
	else
	{
		draw_room_wireframe( world, camera->room );
	}
	PlgImmDraw();

	//PlPopMatrix();
}

static void draw_room_submesh( PLGMesh *mesh, ApeMaterial *material, unsigned int materialIndex, ApeLight *light )
{
	mesh->numSubMeshes = numSubMeshes[ materialIndex ];
	mesh->firstSubMeshes = firstSubMeshes[ materialIndex ];
	mesh->subMeshes = subMeshes[ materialIndex ];

	ApeLightPointerArray lights;
	lights[ 0 ] = light;
	ape_material_draw( material, mesh, lights, ( lights[ 0 ] != NULL ) ? 1 : 0 );

	mesh->numSubMeshes = numSubMeshes[ materialIndex ] = 0;
}

static void draw_room_faces( ApeWorld *world, ApeRoom *room, ApeCamera *camera, ApeLight *light, bool drawAlpha )
{
	unsigned int numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );

	unsigned int numVertices;
	for ( unsigned int i = 0, offset = 0; i < numFaces; ++i, offset += numVertices )
	{
		numVertices = PlGetNumLinkedListNodes( faces[ i ]->edgeLoop );
		if ( faces[ i ]->materialIndex < 0 )
		{
			continue;
		}

		unsigned int materialIndex = faces[ i ]->materialIndex;
		ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, materialIndex );
		if ( material == NULL )
		{
			continue;
		}

		if ( ( drawAlpha && !ape_material_is_blended( material ) ) || ( !drawAlpha && ape_material_is_blended( material ) ) )
		{
			continue;
		}

		assert( numSubMeshes[ materialIndex ] < MAX_SUB_MESHES );
		if ( numSubMeshes[ materialIndex ] >= MAX_SUB_MESHES )
		{
			PRINT_WARNING( "Hit submesh limit for draw, will squeeze into another batch!\n" );
			break;
		}

		if ( ape_config_.renderer.showFaceBounds )
		{
#pragma message "TODO: update this to use material system!!!"
			PlgSetShaderProgram( ape_get_default_shader( APE_SHADER_DEFAULT_VERTEX )->internal );
			PlgDrawBoundingVolume( &faces[ i ]->bounds, &PL_COLOUR_WHITE );
		}

		PLCollisionPlane plane = { .normal = faces[ i ]->normal, .origin = faces[ i ]->origin };
		if ( light != nullptr && ape_light_test_plane_shadow( light, material, &plane ) )
		{
			continue;
		}

		if ( PlgIsBoxInsideView( camera->internal, &faces[ i ]->bounds ) )
		{
			subMeshes[ materialIndex ][ numSubMeshes[ materialIndex ] ] = numVertices;
			firstSubMeshes[ materialIndex ][ numSubMeshes[ materialIndex ] ] = offset;
			numSubMeshes[ materialIndex ]++;

			ape_rendererPerformance_.numFacesDrawn++;
		}
	}
}

static void draw_room( ApeWorld *world, ApeRoom *room, ApeCamera *camera, ApeLight *light, bool ambienceOnly )
{
	if ( PlIsVectorArrayEmpty( room->faces ) )
	{
		return;
	}

	if ( !PlgIsBoxInsideView( camera->internal, &room->base.bounds ) && !ape_config_.renderer.skipRoomCull )
	{
		return;
	}

	if ( ( !ambienceOnly && light == NULL ) /*|| ( light != NULL && !PlIsPointIntersectingAabb( &room->bounds, light->position ) )*/ )
	{
		return;
	}

	PLColourF32 oldAmbience;
	if ( light != NULL )
	{
		oldAmbience = world->ambience;
		world->ambience = PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	// solid surfaces
	draw_room_faces( world, room, camera, light, false );
	for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->materials ); ++i )
	{
		if ( numSubMeshes[ i ] == 0 )
		{
			continue;
		}

		ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, i );
		assert( material != NULL );

		draw_room_submesh( room->mesh, material, i, ambienceOnly ? NULL : light );
	}

	// transparent surfaces
	draw_room_faces( world, room, camera, light, true );
	for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->materials ); ++i )
	{
		if ( numSubMeshes[ i ] == 0 )
		{
			continue;
		}

		ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, i );
		assert( material != NULL );

		draw_room_submesh( room->mesh, material, i, ambienceOnly ? NULL : light );
	}

	if ( light != NULL )
	{
		world->ambience = oldAmbience;
	}
}

static const float F_INFINITY = 10000.0f;

static PLVector3 get_projection( const ApeLight *light, const PLVector3 *origin )
{
	if ( light->type != APE_LIGHT_TYPE_SUN )
	{
		return PlNormalizeVector3( PlSubtractVector3( *origin, light->base.position ) );
	}

	return PlNormalizeVector3( light->base.position );
}

static void draw_stencil_shadow_cap( const ApeWorldFace *face, const ApeLight *light, bool start, unsigned int *indices )
{
	unsigned int numVertices = PlGetNumLinkedListNodes( face->edgeLoop );
	PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
		assert( vertex->u != NULL );
		//TODO: yes yes, all this bollocks should be in a vertex shader...
		PLVector3 projDirection = start ? pl_vecOrigin3 : get_projection( light, &vertex->u->position );
		indices[ i ] = PlgImmPushVertex( vertex->u->position.x + ( projDirection.x * F_INFINITY ),
		                                 vertex->u->position.y + ( projDirection.y * F_INFINITY ),
		                                 vertex->u->position.z + ( projDirection.z * F_INFINITY ) );
#if 1// for debugging
		PlgImmColour( start ? 255 : 0, start ? 0 : 255, 255, 255 );
#endif
		faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
	}

	for ( unsigned int i = 1; i + 1 < numVertices; ++i )
	{
		PlgImmPushTriangle( indices[ 0 ], indices[ start ? i : ( i + 1 ) ], indices[ start ? ( i + 1 ) : i ] );
	}
}

static void draw_room_stencil_shadow_volumes( ApeRoom *room, const ApeLight *light )
{
	ApeMaterial *shadowMaterial = ss_arl_get_default_material( SS_ARL_MATERIAL_DEFAULT_SHADOW );
	assert( shadowMaterial != NULL );

	unsigned int numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLES );
	unsigned int numIndices = 0;
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		if ( faces[ i ]->material == NULL || !ape_material_shadows_enabled( faces[ i ]->material ) )
			continue;

		if ( ape_light_test_plane( light, &( PLCollisionPlane ){ .normal = faces[ i ]->normal, .origin = faces[ i ]->origin } ) )
		{
			continue;
		}

		// There's probably a more efficient way of doing this,
		// but let's go ahead and store all the indices into a dynamic array
		unsigned int numVertices = PlGetNumLinkedListNodes( faces[ i ]->edgeLoop );
		numIndices += ( numVertices * 2 );// * 2 for edges
		static unsigned int *indices = NULL;
		static unsigned int maxIndices = 0;
		if ( indices == NULL )
		{
			maxIndices = ( numIndices * numFaces );
			indices = PL_NEW_( unsigned int, maxIndices );
		}
		else if ( numIndices > maxIndices )
		{
			maxIndices = numIndices + 16;
			indices = PL_REALLOCA( indices, sizeof( unsigned int ) * maxIndices );
		}

		unsigned int *fl = &indices[ numIndices - ( numVertices * 2 ) ];
		draw_stencil_shadow_cap( faces[ i ], light, false, fl );
		unsigned int *sl = &indices[ numIndices - numVertices ];
		draw_stencil_shadow_cap( faces[ i ], light, true, sl );

		// Now produce the edges
		for ( int j = 0; j < numVertices; j++ )
		{
			PlgImmPushTriangle( fl[ j ], fl[ ( j + 1 ) % numVertices ], sl[ j ] );
			PlgImmPushTriangle( sl[ j ], fl[ ( j + 1 ) % numVertices ], sl[ ( j + 1 ) % numVertices ] );
		}
	}

	ape_material_draw( shadowMaterial, mesh, NULL, 0 );
}

static void draw_room_stencil_shadow_pass( ApeRoom *room, ApeCamera *camera, ApeLight *light )
{
	if ( light == NULL )
		return;

	if ( PlIsVectorArrayEmpty( room->faces ) )
	{
		return;
	}

	if ( !PlgIsBoxInsideView( camera->internal, &room->base.bounds ) && !ape_config_.renderer.skipRoomCull )
	{
		return;
	}

	if ( !room->isDetail )
	{
		unsigned int numDetailRooms = PlGetNumVectorArrayElements( room->detailRooms );
		ApeRoom **detailRooms = ( ApeRoom ** ) PlGetVectorArrayData( room->detailRooms );
		for ( unsigned int j = 0; j < numDetailRooms; ++j )
		{
			draw_room_stencil_shadow_volumes( detailRooms[ j ], light );
		}
	}

	draw_room_stencil_shadow_volumes( room, light );
}

void ape_world_draw_stencil_shadows( ApeWorld *world, ApeCamera *camera, ApeLight *light )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	for ( unsigned int i = 0; i < camera->visibility.numRooms; ++i )
	{
		PlLoadMatrix( &camera->visibility.rooms[ i ].transform );

		draw_room_stencil_shadow_pass( camera->visibility.rooms[ i ].room, camera, light );
	}

	PlPopMatrix();
}

void ape_world_draw( ApeWorld *world, ApeCamera *camera, ApeLight *light, bool ambienceOnly )
{
	if ( camera == NULL || world == NULL )
	{
		return;
	}

	if ( ambienceOnly && ape_config_.renderer.skipAmbience )
	{
		return;
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	for ( unsigned int i = 0; i < camera->visibility.numRooms; ++i )
	{
		PlLoadMatrix( &camera->visibility.rooms[ i ].transform );

		ApeRoom *room = camera->visibility.rooms[ i ].room;
		draw_room( world, room, camera, light, ambienceOnly );
	}

#pragma message "Decide how we're going to do this..."
#if 0
	unsigned int numWorldNodes;
	ApeWorldNode **worldNodes = ape_camera_get_visible_nodes_( camera, &numWorldNodes );
	for ( unsigned int i = 0; i < numWorldNodes; ++i )
	{
		if ( !worldNodes[ i ]->classType->drawFunction )
		{
			continue;
		}

		worldNodes[ i ]->classType->drawFunction();
	}
#endif

	PlPopMatrix();
}
