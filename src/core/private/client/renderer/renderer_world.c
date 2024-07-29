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

static void draw_face_wireframe( ApeWorldFace *face )
{
	uint                 numVertices;
	ApeWorldFaceVertex **vertices = ( ApeWorldFaceVertex ** ) PlGetVectorArrayDataEx( face->vertices, &numVertices );
	for ( uint i = 0; i < numVertices; ++i )
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
	uint           numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );
	for ( uint j = 0; j < numFaces; ++j )
	{
		draw_face_wireframe( faces[ j ] );
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
				draw_room_wireframe( world, ( ApeRoom * ) worldNode );
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

static void draw_room_submesh( PLGMesh *mesh, ApeMaterial *material, uint materialIndex, ApeLight *light )
{
	mesh->numSubMeshes   = numSubMeshes[ materialIndex ];
	mesh->firstSubMeshes = firstSubMeshes[ materialIndex ];
	mesh->subMeshes      = subMeshes[ materialIndex ];

	ApeLightPointerArray lights;
	lights[ 0 ] = light;
	ape_material_draw( material, mesh, lights );

	mesh->numSubMeshes = numSubMeshes[ materialIndex ] = 0;
}

#if 0
static bool build_shadow_display_list( ApeRoom *room, const ApeLight *light )
{
	uint           numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );

	bool shadowTest = false;
	for ( uint i = 0, offset = 0, numVertices; i < numFaces; ++i, offset += numVertices )
	{
		numVertices = PlGetNumLinkedListNodes( faces[ i ]->edgeLoop );
		if ( faces[ i ]->materialIndex < 0 )
		{
			continue;
		}

		assert( numSubMeshes[ 0 ] < MAX_SUB_MESHES );
		if ( numSubMeshes[ 0 ] >= MAX_SUB_MESHES )
		{
			PRINT_WARNING( "Hit submesh limit for draw, will squeeze into another batch!\n" );
			break;
		}

		if ( !shadowTest && ape_light_test_plane( light, &( PLCollisionPlane ){ .origin = faces[ i ]->origin, .normal = faces[ i ]->normal } ) )
		{
			shadowTest = true;
		}

		subMeshes[ 0 ][ numSubMeshes[ 0 ] ]      = numVertices;
		firstSubMeshes[ 0 ][ numSubMeshes[ 0 ] ] = offset;
		numSubMeshes[ 0 ]++;

		ape_rendererPerformance_.numFacesDrawn++;
	}

	return shadowTest;
}
#endif

static void build_display_lists( ApeWorld *world, ApeRoom *room, ApeCamera *camera, ApeLight *light, bool drawAlpha )
{
	uint           numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );

	uint numVertices;
	for ( uint i = 0, offset = 0; i < numFaces; ++i, offset += numVertices )
	{
		numVertices = PlGetNumLinkedListNodes( faces[ i ]->edgeLoop );
		if ( faces[ i ]->materialIndex < 0 )
		{
			continue;
		}

		uint         materialIndex = faces[ i ]->materialIndex;
		ApeMaterial *material      = PlGetVectorArrayElementAt( world->materials, materialIndex );
		if ( material == NULL )
		{
			continue;
		}

		if ( drawAlpha != ape_material_is_blended( material ) )
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
			PlgDrawBoundingVolume( &( PLCollisionAABB ){ .origin = faces[ i ]->origin, .mins = { -0.1f, -0.1f, -0.1f }, .maxs = { 0.1f, 0.1f, 0.1f } }, &PL_COLOUR_BLUE );
		}

#if 0// ditched for speed...
		PLCollisionPlane plane = { .normal = faces[ i ]->normal, .origin = faces[ i ]->origin };
		if ( light != nullptr && ape_light_test_plane_shadow( light, material, &plane ) )
		{
			continue;
		}
#endif

		if ( PlgIsBoxInsideView( camera->internal, &faces[ i ]->bounds ) )
		{
			subMeshes[ materialIndex ][ numSubMeshes[ materialIndex ] ]      = numVertices;
			firstSubMeshes[ materialIndex ][ numSubMeshes[ materialIndex ] ] = offset;
			numSubMeshes[ materialIndex ]++;

			ape_rendererPerformance_.numFacesDrawn++;
		}
	}
}

static void draw_room( ApeWorld *world, ApeRoom *room, ApeCamera *camera, ApeLight *light, bool ambienceOnly, bool alpha )
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
		oldAmbience     = world->ambience;
		world->ambience = PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	build_display_lists( world, room, camera, light, alpha );
	for ( uint i = 0; i < PlGetNumVectorArrayElements( world->materials ); ++i )
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

static void draw_stencil_shadow_cap( const ApeWorldFace *face, const ApeLight *light, bool start, uint *indices )
{
	uint              numVertices    = PlGetNumLinkedListNodes( face->edgeLoop );
	PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
	for ( uint i = 0; i < numVertices; ++i )
	{
		ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
		assert( vertex->u != NULL );
		//TODO: yes yes, all this bollocks should be in a vertex shader...
		PLVector3 projDirection = start ? pl_vecOrigin3 : get_projection( light, &vertex->u->position );
		indices[ i ]            = PlgImmPushVertex( vertex->u->position.x + ( projDirection.x * F_INFINITY ),
		                                            vertex->u->position.y + ( projDirection.y * F_INFINITY ),
		                                            vertex->u->position.z + ( projDirection.z * F_INFINITY ) );
#if 1// for debugging
		PlgImmColour( start ? 255 : 0, start ? 0 : 255, 255, 255 );
#endif
		faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
	}

	for ( uint i = 1; i + 1 < numVertices; ++i )
	{
		PlgImmPushTriangle( indices[ 0 ], indices[ start ? i : ( i + 1 ) ], indices[ start ? ( i + 1 ) : i ] );
	}
}

static void draw_room_stencil_shadow_volumes( ApeRoom *room, ApeLight *light )
{
	ApeMaterial *shadowMaterial = ss_arl_get_default_material( SS_ARL_MATERIAL_DEFAULT_SHADOW );
	assert( shadowMaterial != NULL );

#if 0 // entirely done with vertex shader...

	if ( !build_shadow_display_list( room, light ) )
	{
		return;
	}

	draw_room_submesh( room->mesh, shadowMaterial, 0, light );

#else // slower CPU route...

	uint           numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );

	PLGMesh *mesh       = PlgImmBegin( PLG_MESH_TRIANGLES );
	uint     numIndices = 0;
	for ( uint i = 0; i < numFaces; ++i )
	{
		if ( faces[ i ]->material == NULL || !ape_material_shadows_enabled( faces[ i ]->material ) )
		{
			continue;
		}

		PLCollisionPlane plane = ( PLCollisionPlane ){ .normal = faces[ i ]->normal, .origin = faces[ i ]->origin };
		if ( ape_light_test_plane( light, &plane ) )
		{
			continue;
		}

		// There's probably a more efficient way of doing this,
		// but let's go ahead and store all the indices into a dynamic array
		uint numVertices = PlGetNumLinkedListNodes( faces[ i ]->edgeLoop );
		numIndices += ( numVertices * 2 );// * 2 for edges
		static uint *indices    = NULL;
		static uint  maxIndices = 0;
		if ( indices == NULL )
		{
			maxIndices = ( numIndices * numFaces );
			indices    = PL_NEW_( uint, maxIndices );
		}
		else if ( numIndices > maxIndices )
		{
			maxIndices = numIndices + 16;
			indices    = PL_REALLOCA( indices, sizeof( uint ) * maxIndices );
		}

		uint *fl = &indices[ numIndices - ( numVertices * 2 ) ];
		draw_stencil_shadow_cap( faces[ i ], light, false, fl );
		uint *sl = &indices[ numIndices - numVertices ];
		draw_stencil_shadow_cap( faces[ i ], light, true, sl );

		// Now produce the edges
		for ( int j = 0; j < numVertices; j++ )
		{
			PlgImmPushTriangle( fl[ j ], fl[ ( j + 1 ) % numVertices ], sl[ j ] );
			PlgImmPushTriangle( sl[ j ], fl[ ( j + 1 ) % numVertices ], sl[ ( j + 1 ) % numVertices ] );
		}
	}

	// if num indices are zero, probably didn't hit any faces...
	if ( numIndices == 0 )
	{
		return;
	}

	ape_material_draw( shadowMaterial, mesh, NULL );

#endif
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
		uint      numDetailRooms = PlGetNumVectorArrayElements( room->detailRooms );
		ApeRoom **detailRooms    = ( ApeRoom    **) PlGetVectorArrayData( room->detailRooms );
		for ( uint j = 0; j < numDetailRooms; ++j )
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

	for ( uint i = 0; i < camera->visibility.numRooms; ++i )
	{
		PlLoadMatrix( &camera->visibility.rooms[ i ].transform );

		draw_room_stencil_shadow_pass( camera->visibility.rooms[ i ].room, camera, light );
	}

	PlPopMatrix();
}

void ape_world_draw( ApeWorld *world, ApeCamera *camera, ApeLight *light, bool ambienceOnly, bool alpha )
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

	for ( uint i = 0; i < camera->visibility.numRooms; ++i )
	{
		PlLoadMatrix( &camera->visibility.rooms[ i ].transform );
		draw_room( world, camera->visibility.rooms[ i ].room, camera, light, ambienceOnly, alpha );
	}

#pragma message "Decide how we're going to do this..."
#if 0
	uint numWorldNodes;
	ApeWorldNode **worldNodes = ape_camera_get_visible_nodes_( camera, &numWorldNodes );
	for ( uint i = 0; i < numWorldNodes; ++i )
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
