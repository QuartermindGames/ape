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

static void draw_room_wireframe( ApeWorld *world, ApeWorldRoom *room )
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

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
	PlgSetTexture( NULL, 0 );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &world->root->transform );

	PlgImmBegin( PLG_MESH_LINES );
	if ( camera->room == NULL || ape_config_.world.showAllRooms )
	{
		// Just go over the initial children to determine if they're rooms
		PLLinkedListNode *node = PlGetFirstNode( world->root->children );
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

	PlPopMatrix();
}

static bool face_is_facing_light( const ApeWorldFace *face, const ApeLight *light )
{
	PLVector3 lightDir = PlNormalizeVector3( PlSubtractVector3( face->origin, light->position ) );
	if ( PlVector3DotProduct( face->normal, lightDir ) >= 0 )
	{
		return true;
	}

	return false;
}

static bool is_light_not_shadowing_face( const ApeMaterial *material, const ApeWorldFace *face, ApeLight *light )
{
	if ( light == NULL || ( ape_light_get_shadow_type( light ) != SS_APE_LIGHT_SHADOW_TYPE_DYNAMIC ) )
	{
		return false;
	}

	return ( ss_arl_material_shadows_enabled( material ) && !face_is_facing_light( face, light ) );
}

static void draw_room_submesh( PLGMesh *mesh, ApeMaterial *material, unsigned int materialIndex, ApeLight *light )
{
	mesh->numSubMeshes = numSubMeshes[ materialIndex ];
	mesh->firstSubMeshes = firstSubMeshes[ materialIndex ];
	mesh->subMeshes = subMeshes[ materialIndex ];

	ApeLightPointerArray lights;
	lights[ 0 ] = light;
	ss_arl_material_draw( material, mesh, lights, ( lights[ 0 ] != NULL ) ? 1 : 0 );

	mesh->numSubMeshes = numSubMeshes[ materialIndex ] = 0;
}

static void draw_room( ApeWorld *world, ApeWorldRoom *room, ApeCamera *camera, bool skipPortals, ApeLight *light, bool ambienceOnly )
{
	if ( PlIsVectorArrayEmpty( room->faces ) )
	{
		return;
	}

	if ( !PlgIsBoxInsideView( camera->internal, &room->bounds ) && !ape_config_.renderer.skipRoomCull )
	{
		return;
	}

	if ( ape_config_.world.showRoomVolumes )
	{
		PlgSetShaderProgram( ss_arl_shader_get_default( APE_SHADER_DEFAULT_VERTEX ) );
		PLColour colour = PlColourF32ToU8( &room->colour );
		PlgDrawBoundingVolume( &room->bounds, &colour );
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

	unsigned int numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );
	for ( unsigned int i = 0, offset = 0; i < numFaces; ++i )
	{
		if ( faces[ i ]->materialIndex < 0 )
		{
			continue;
		}

		unsigned int materialIndex = faces[ i ]->materialIndex;
		ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, materialIndex );
		assert( material != NULL );

		assert( numSubMeshes[ materialIndex ] < MAX_SUB_MESHES );
		if ( numSubMeshes[ materialIndex ] >= MAX_SUB_MESHES )
		{
			PRINT_WARNING( "Hit submesh limit for draw, will squeeze into another batch!\n" );
			break;
		}

		if ( ape_config_.renderer.showFaceBounds )
		{
			PlgSetShaderProgram( ss_arl_shader_get_default( APE_SHADER_DEFAULT_VERTEX ) );
			PlgDrawBoundingVolume( &faces[ i ]->bounds, &PL_COLOUR_WHITE );
		}

		unsigned int numVertices = PlGetNumLinkedListNodes( faces[ i ]->edgeLoop );

		if ( is_light_not_shadowing_face( material, faces[ i ], light ) )
		{
			offset += numVertices;
			continue;
		}

		if ( PlgIsBoxInsideView( camera->internal, &faces[ i ]->bounds ) )
		{
			subMeshes[ materialIndex ][ numSubMeshes[ materialIndex ] ] = numVertices;
			firstSubMeshes[ materialIndex ][ numSubMeshes[ materialIndex ] ] = offset;
			numSubMeshes[ materialIndex ]++;

			ape_rendererPerformance_.numFacesDrawn++;
		}

		offset += numVertices;
	}

	for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->materials ); ++i )
	{
		if ( numSubMeshes[ i ] == 0 )
			continue;

		ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, i );
		assert( material != NULL );

		draw_room_submesh( room->mesh, material, i, ambienceOnly ? NULL : light );
	}

	if ( light != NULL )
		world->ambience = oldAmbience;
}

static const float F_INFINITY = 100.0f;

static PLVector3 get_projection( const ApeLight *light, const PLVector3 *origin )
{
	if ( light->type != APE_LIGHT_TYPE_SUN )
	{
		return PlNormalizeVector3( PlSubtractVector3( *origin, light->position ) );
	}

	return PlNormalizeVector3( ( PLVector3 ){ light->position.x, light->position.y, light->position.z } );
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

static void draw_room_stencil_shadow_volumes( ApeWorldRoom *room, const ApeLight *light )
{
	ApeMaterial *shadowMaterial = ss_arl_get_default_material( SS_ARL_MATERIAL_DEFAULT_SHADOW );
	assert( shadowMaterial != NULL );

	unsigned int numFaces;
	ApeWorldFace **faces = ape_world_room_get_faces_( room, &numFaces );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLES );
	unsigned int numIndices = 0;
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		if ( faces[ i ]->material == NULL || !ss_arl_material_shadows_enabled( faces[ i ]->material ) )
			continue;

		if ( face_is_facing_light( faces[ i ], light ) )
			continue;

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

	ss_arl_material_draw( shadowMaterial, mesh, NULL, 0 );
}

static void draw_room_stencil_shadow_pass( ApeWorldRoom *room, ApeCamera *camera, ApeLight *light )
{
	if ( light == NULL )
		return;

	if ( PlIsVectorArrayEmpty( room->faces ) )
	{
		return;
	}

	if ( !PlgIsBoxInsideView( camera->internal, &room->bounds ) && !ape_config_.renderer.skipRoomCull )
	{
		return;
	}

	if ( !room->isDetail )
	{
		unsigned int numDetailRooms = PlGetNumVectorArrayElements( room->detailRooms );
		ApeWorldRoom **detailRooms = ( ApeWorldRoom ** ) PlGetVectorArrayData( room->detailRooms );
		for ( unsigned int j = 0; j < numDetailRooms; ++j )
			draw_room_stencil_shadow_volumes( detailRooms[ j ], light );
	}

	draw_room_stencil_shadow_volumes( room, light );
}

void ape_world_draw_stencil_shadows( ApeWorld *world, ApeCamera *camera, ApeLight *light )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

	if ( camera->room == NULL || ape_config_.world.showAllRooms )
	{
		for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			assert( room != NULL );
			if ( room->isDetail )
			{
				continue;
			}

			draw_room_stencil_shadow_pass( room, camera, light );
		}
	}
	else
	{
		draw_room_stencil_shadow_pass( camera->room, camera, light );
	}

	PlPopMatrix();
}

void ape_world_draw( ApeWorld *world, ApeCamera *camera, ApeLight *light, bool ambienceOnly )
{
	assert( ( camera != NULL ) && ( world != NULL ) );
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
	PlLoadMatrix( &world->root->transform );

	if ( camera->room == NULL || ape_config_.world.showAllRooms )
	{
		if ( world->rooms != NULL )
		{
			for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
			{
				ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
				assert( room != NULL );
				if ( room->isDetail )
				{
					continue;
				}

				draw_room( world, room, camera, true, light, ambienceOnly );
			}
		}
	}
	else
	{
		draw_room( world, camera->room, camera, ape_config_.world.skipPortals, light, ambienceOnly );
	}

	PlPopMatrix();
}
