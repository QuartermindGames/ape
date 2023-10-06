// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "renderer.h"
#include "world/world.h"
#include "legacy/actor.h"

#if 0
	PLLinkedListNode *faceNode = PlGetFirstNode( visiblePortals );
	while ( faceNode != NULL )
	{
		OgeWorldFace *face = PlGetLinkedListNodeUserData( faceNode );
		if ( face->isPortalClosed )
		{
			faceNode = PlGetNextLinkedListNode( faceNode );
			continue;
		}

		face->flags |= WORLD_FACE_FLAG_SKIP;
		if ( face->flags & WORLD_FACE_FLAG_MIRROR )
		{
			/* in the case of a mirror, both the target and target face
			 * are assumed to be the same as the mirror, so that keeps
			 * things pretty simple! */

			rendererState.depth++;
			rendererState.mirror = true;

#	if 0
			//PlMatrixMode( PL_MODELVIEW_MATRIX );

			//PLMatrix4 om = camera->internal->internal.proj;
			//PlMatrixMode( PL_PROJECTION_MATRIX );
			//PlPushMatrix();
			//PlLoadMatrix( &om );

			//PlInverseMatrix();

			// Inverse it
			//PlScaleMatrix( PlVector3( 1.0f, 1.0f, -1.0f ) );

			//PLMatrix4 im = PlInverseMatrix4( om );
			//PlMultiMatrix( &im );

			//PLVector3 in = PlInverseVector3( PlVector3( PL_RAD2DEG( face->normal.x ),
			//                                            PL_RAD2DEG( face->normal.y ),
			 //                                           PL_RAD2DEG( face->normal.z ) ) );
			//camera->internal->angles = PlAddVector3( camera->internal->angles, in );

			PLVector3 angles = pl_vecOrigin3;
			angles = PlAddVector3( angles, PlQuaternionToEuler( &PlQuaternion( 1.0f, 0.0f, 0.0f, face->normal.x ) ) );
			angles = PlAddVector3( angles, PlQuaternionToEuler( &PlQuaternion( 0.0f, 1.0f, 0.0f, face->normal.y ) ) );
			angles = PlAddVector3( angles, PlQuaternionToEuler( &PlQuaternion( 0.0f, 0.0f, 1.0f, face->normal.z ) ) );

			camera->internal->position = PlAddVector3( camera->internal->position, face->origin );
			camera->internal->angles.x += ( angles.x );
			camera->internal->angles.y += ( angles.y );
			camera->internal->angles.z += ( angles.z );

			PlgSetupCamera( camera->internal );

			// Override the matrix the above set for us
			//PlgSetProjectionMatrix( PlGetMatrix( PL_PROJECTION_MATRIX ) );

			DrawSector( NULL, sector, camera );

			PlPopMatrix();

			// Restore it
			PlgSetupCamera( camera->internal );
#	else
			PlMatrixMode( PL_MODELVIEW_MATRIX );

			int x, y;
			if ( ( fabsf( face->normal.x ) > fabsf( face->normal.y ) ) && ( fabsf( face->normal.x ) > fabsf( face->normal.z ) ) )
			{
				x = ( face->normal.x > 0.0 ) ? 1 : 2;
				y = ( face->normal.x > 0.0 ) ? 2 : 1;
			}
			else if ( ( fabsf( face->normal.z ) > fabsf( face->normal.x ) ) && ( fabsf( face->normal.z ) > fabsf( face->normal.y ) ) )
			{
				x = ( face->normal.z > 0.0 ) ? 0 : 1;
				y = ( face->normal.z > 0.0 ) ? 1 : 0;
			}
			else
			{
				x = ( face->normal.y > 0.0 ) ? 2 : 0;
				y = ( face->normal.y > 0.0 ) ? 0 : 2;
			}

			PlScaleMatrix( PlVector3( x == 0 ? -1.0f : 1.0f,
			                          y == 0 ? -1.0f : 1.0f,
			                          x == 0 && y == 0 ? -1.0f : 1.0f ) );

			PLVector3 normal = PlInverseVector3( face->normal );
			PLVector3 angles = pl_vecOrigin3;
			angles           = PlAddVector3( angles, PlQuaternionToEuler( &PlQuaternion( 1.0f, 0.0f, 0.0f, normal.x ) ) );
			angles           = PlAddVector3( angles, PlQuaternionToEuler( &PlQuaternion( 0.0f, 1.0f, 0.0f, normal.y ) ) );
			angles           = PlAddVector3( angles, PlQuaternionToEuler( &PlQuaternion( 0.0f, 0.0f, 1.0f, normal.z ) ) );
			//PlRotateMatrix( angles.x, 1.0f, 0.0f, 0.0f );
			//PlRotateMatrix( angles.y, 0.0f, 1.0f, 0.0f );
			//PlRotateMatrix( angles.z, 0.0f, 0.0f, 1.0f );

			DrawSector( NULL, sector, camera );

			PlPopMatrix();
#	endif

			rendererState.depth--;
			rendererState.mirror = false;
		}
		else
		{
			/* otherwise, in the case of an actual portal, we'll need
			 * to fetch the target sector and the target face...
			 * if these aren't set appropriately, then, well... */
		}
		face->flags &= ~WORLD_FACE_FLAG_SKIP;
#endif

/**
 * World is drawn using polygons, rather than straight up triangles,
 * so to more accuratly display it in wireframe, we'll need to render
 * it in such a mode ourselves. This is mostly for the sake of the
 * editor.
 */
void apeDrawWorldWireframe_( ApeWorld *world, ApeCamera *camera )
{
#if 0
	if ( world == NULL )
	{
		return;
	}

	PlgSetShaderProgram( oge_defaultShaderPrograms_[ OGE_SHADER_DEFAULT_VERTEX ] );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	PlgSetTexture( NULL, 0 );

	PlgImmBegin( PLG_MESH_LINES );
	for ( unsigned int i = 0; i < world->numRooms; ++i )
	{
		if ( world->rooms[ i ].mesh == NULL )
		{
			continue;
		}

		OgeWorldMesh *mesh     = world->rooms[ i ].mesh;
		PLLinkedListNode *node = PlGetFirstNode( mesh->faces );
		while ( node != NULL )
		{
			OgeWorldFace *face = PlGetLinkedListNodeUserData( node );
			for ( unsigned int j = 0; j < face->numVertices; ++j )
			{
				OgeWorldVertex *a = &mesh->vertices[ face->vertices[ j ] ];
				OgeWorldVertex *b = ( ( j + 1 ) < face->numVertices ) ? &mesh->vertices[ face->vertices[ j + 1 ] ] : &mesh->vertices[ face->vertices[ 0 ] ];

				PlgImmPushVertex( a->position.x, a->position.y, a->position.z );
				if ( face->targetSector != NULL )
				{
					PlgImmColour( 255, 0, 255, 255 );
				}
				else
				{
					PlgImmColour( 255, 255, 255, 255 );
				}

				PlgImmPushVertex( b->position.x, b->position.y, b->position.z );
				if ( face->targetSector != NULL )
				{
					PlgImmColour( 255, 0, 255, 255 );
				}
				else
				{
					PlgImmColour( 255, 255, 255, 255 );
				}
			}

			node = PlGetNextLinkedListNode( node );
		}
	}
	PlgImmDraw();

	PlgImmBegin( PLG_MESH_POINTS );
	PlgImmSetPrimitiveScale( 4.0f );
	for ( unsigned int i = 0; i < world->numRooms; ++i )
	{
		if ( world->rooms[ i ].mesh == NULL )
		{
			continue;
		}

		OgeWorldMesh *mesh     = world->rooms[ i ].mesh;
		PLLinkedListNode *node = PlGetFirstNode( mesh->faces );
		while ( node != NULL )
		{
			OgeWorldFace *face = PlGetLinkedListNodeUserData( node );
			for ( unsigned int j = 0; j < face->numVertices; ++j )
			{
				OgeWorldVertex *a = &mesh->vertices[ face->vertices[ j ] ];
				PlgImmPushVertex( a->position.x, a->position.y, a->position.z );
				PlgImmColour( 0, 255, 0, 255 );
			}

			node = PlGetNextLinkedListNode( node );
		}
	}
	PlgImmDraw();

	PlPopMatrix();
#endif
}

static void DrawRoom( ApeWorld *world, ApeWorldRoom *room, ApeCamera *camera, bool skipPortals, ApeLight *light, bool ambienceOnly )
{
	if ( PlIsVectorArrayEmpty( room->faces ) )
		return;

	if ( !PlgIsBoxInsideView( camera->internal, &room->bounds ) )
		return;

#if 0
	if ( !ambienceOnly )
	{
		if ( ape_config_.world.showPortals )
		{
			for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( room->faces ); ++i )
			{
				ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, i );
				assert( face != NULL );
				if ( face == NULL || face->portal == NULL )
					continue;

				ApeWorldPortal *portal = face->portal;
				PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
				PlgDrawBoundingVolume( &PlSetupCollisionAABB( face->origin, portal->mins, portal->maxs ), &PL_COLOURU8( 255, 255, 255, 255 ) );

				PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );

				PLLinkedListNode *node = PlGetFirstNode( face->edgeLoop );
				while ( node != NULL )
				{
					ApeWorldFaceVertex *vertex = ( ApeWorldFaceVertex * ) PlGetLinkedListNodeUserData( node );
					PlgImmPushVertex( vertex->u->position.x, vertex->u->position.y, vertex->u->position.z );
					PlgImmColour( 255, 0, 255, 255 );

					node = PlGetNextLinkedListNode( node );
				}

				apeDrawMesh( apeGetVertexMaterial(), mesh, NULL, 0 );
			}
		}

#	if 0
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( room->faces ); ++i )
		{
			ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, i );
			assert( face != NULL );
			if ( face == NULL )
				continue;

			PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
			PlgDrawLine( *PlGetMatrix( PL_MODELVIEW_MATRIX ),
			             face->origin, PL_COLOURU8( 0, 255, 0, 255 ),
			             PlAddVector3( face->origin, PlScaleVector3F( face->normal, 4.0f ) ), PL_COLOURU8( 255, 0, 0, 255 ) );

			PlgDrawBoundingVolume( &face->bounds, &PL_COLOURU8( 0, 0, 255, 255 ) );
		}
#	endif
	}
#endif

	if ( arl_rendererState_.passStage == APE_RENDERER_PASS_DEPTH )
	{
		room->mesh->numSubMeshes = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_ROOM ] ].numSubMeshes;
		room->mesh->firstSubMeshes = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_ROOM ] ].firstSubMeshes;
		room->mesh->subMeshes = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_ROOM ] ].subMeshes;
		apeDrawMesh( arl_material_get_default( APE_MATERIAL_DEFAULT_DEPTH ), room->mesh, NULL, 0 );

		room->mesh->numSubMeshes = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_DETAIL ] ].numSubMeshes;
		room->mesh->firstSubMeshes = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_DETAIL ] ].firstSubMeshes;
		room->mesh->subMeshes = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_DETAIL ] ].subMeshes;
		apeDrawMesh( arl_material_get_default( APE_MATERIAL_DEFAULT_DEPTH ), room->mesh, NULL, 0 );
	}
	else
	{
		if ( ( !ambienceOnly && light == NULL ) || ( light != NULL && !PlIsPointIntersectingAabb( &room->bounds, light->position ) ) )
			return;

		PLColourF32 oldAmbience;
		if ( light != NULL )
		{
			oldAmbience = world->ambience;
			world->ambience = PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f );
		}

		//if ( room->flags & APE_WORLD_ROOM_FLAG_AMBIENT )
		//	world->ambience = room->ambientLight;

		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->materials ); ++i )
		{
			if ( room->batches[ i ].numSubMeshes == 0 )
				continue;

			ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, i );
			assert( material != NULL );
			if ( material == NULL )
				continue;

			assert( room->batches[ i ].material == material );

			room->mesh->numSubMeshes = room->batches[ i ].numSubMeshes;
			room->mesh->firstSubMeshes = room->batches[ i ].firstSubMeshes;
			room->mesh->subMeshes = room->batches[ i ].subMeshes;

			ApeLightPointerArray lights;
			lights[ 0 ] = light;
			apeDrawMesh( material, room->mesh, lights, ambienceOnly ? 0 : 1 );
		}

		if ( light != NULL )
			world->ambience = oldAmbience;
	}
}

static const float F_INFINITY = 100.0f;

static void draw_room_stencil_shadow_volume( const ApeWorldFace *face, const ApeLight *light, const PLColour *colour )
{
	ApeMaterial *shadowMaterial = arl_material_get_default( APE_MATERIAL_DEFAULT_SHADOW );
	assert( shadowMaterial != NULL );
	if ( shadowMaterial == NULL )
		return;

	PLGMesh *mesh;
	PLLinkedListNode *faceVertexNode;

	PLVector3 projDirection;
	if ( light->type == APE_LIGHT_TYPE_SUN )
		projDirection = PlNormalizeVector3( ( PLVector3 ){ light->position.x, light->position.y, light->position.z } );

	// end cap
	mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
	faceVertexNode = PlGetLastNode( face->edgeLoop );
	while ( faceVertexNode != NULL )
	{
		ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
		assert( vertex->u != NULL );

		if ( light->type != APE_LIGHT_TYPE_SUN )
			projDirection = PlNormalizeVector3( PlSubtractVector3( vertex->u->position, light->position ) );

		PlgImmPushVertex( vertex->u->position.x + projDirection.x * F_INFINITY,
		                  vertex->u->position.y + projDirection.y * F_INFINITY,
		                  vertex->u->position.z + projDirection.z * F_INFINITY );
		PlgImmColour( 255, 0, 255, colour->a );

		faceVertexNode = PlGetPrevLinkedListNode( faceVertexNode );
	}
	apeDrawMesh( shadowMaterial, mesh, NULL, 0 );

	// start cap
	mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
	faceVertexNode = PlGetFirstNode( face->edgeLoop );
	while ( faceVertexNode != NULL )
	{
		ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
		assert( vertex->u != NULL );

		PlgImmPushVertex( vertex->u->position.x, vertex->u->position.y, vertex->u->position.z );
		PlgImmColour( 255, 0, 0, colour->a );

		faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
	}
	apeDrawMesh( shadowMaterial, mesh, NULL, 0 );

	mesh = PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );
	faceVertexNode = PlGetFirstNode( face->edgeLoop );
	while ( faceVertexNode != NULL )
	{
		ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
		assert( vertex->u != NULL );

		PlgImmPushVertex( vertex->u->position.x, vertex->u->position.y, vertex->u->position.z );
		PlgImmColour( colour->r, colour->g, colour->b, colour->a );

		if ( light->type != APE_LIGHT_TYPE_SUN )
			projDirection = PlNormalizeVector3( PlSubtractVector3( vertex->u->position, light->position ) );

		PlgImmPushVertex( vertex->u->position.x + projDirection.x * F_INFINITY,
		                  vertex->u->position.y + projDirection.y * F_INFINITY,
		                  vertex->u->position.z + projDirection.z * F_INFINITY );
		PlgImmColour( colour->r, colour->g, colour->b, colour->a );

		faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
		if ( faceVertexNode == NULL )
		{
			faceVertexNode = PlGetFirstNode( face->edgeLoop );
			vertex = PlGetLinkedListNodeUserData( faceVertexNode );
			PlgImmPushVertex( vertex->u->position.x, vertex->u->position.y, vertex->u->position.z );
			PlgImmColour( colour->r, colour->g, colour->b, colour->a );

			if ( light->type != APE_LIGHT_TYPE_SUN )
				projDirection = PlNormalizeVector3( PlSubtractVector3( vertex->u->position, light->position ) );

			PlgImmPushVertex( vertex->u->position.x + projDirection.x * F_INFINITY,
			                  vertex->u->position.y + projDirection.y * F_INFINITY,
			                  vertex->u->position.z + projDirection.z * F_INFINITY );
			PlgImmColour( colour->r, colour->g, colour->b, colour->a );
			break;
		}
	}
	apeDrawMesh( shadowMaterial, mesh, NULL, 0 );
}

static void DrawRoomStencilShadowVolumes( ApeWorldRoom *room, const ApeLight *light )
{
	unsigned int numFaces;
	ApeWorldFace **faces = apeGetWorldRoomFaces( room, &numFaces );
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		if ( faces[ i ]->material == NULL || !arl_material_shadows_enabled( faces[ i ]->material ) )
			continue;

		//if ( !PlIsSphereIntersectingAabb( &PlSetupCollisionSphere( light->position, light->radius ), &faces[ i ]->bounds ) )
		//	continue;

		PLVector3 lightDir = PlNormalizeVector3( PlSubtractVector3( faces[ i ]->origin, light->position ) );
		if ( PlVector3DotProduct( faces[ i ]->normal, lightDir ) >= 0 )
			continue;

		draw_room_stencil_shadow_volume( faces[ i ], light, &PL_COLOURU8( 255, 255, 255, 255 ) );
	}
}

static void DrawRoomStencilShadowPass( ApeWorldRoom *room, ApeCamera *camera, ApeLight *light )
{
	if ( light == NULL )
		return;

	if ( PlIsVectorArrayEmpty( room->faces ) )
		return;

	if ( !PlgIsBoxInsideView( camera->internal, &room->bounds ) )
		return;

	if ( !room->isDetail )
	{
		unsigned int numDetailRooms = PlGetNumVectorArrayElements( room->detailRooms );
		ApeWorldRoom **detailRooms = ( ApeWorldRoom ** ) PlGetVectorArrayData( room->detailRooms );
		for ( unsigned int j = 0; j < numDetailRooms; ++j )
			DrawRoomStencilShadowVolumes( detailRooms[ j ], light );
	}

	DrawRoomStencilShadowVolumes( room, light );
}

void apeDrawWorldStencilShadowPass_( ApeWorld *world, ApeCamera *camera, ApeLight *light )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

	PL_GET_CVAR( "world/showAllRooms", showAllRooms );
	if ( camera->room == NULL || ( showAllRooms != NULL && showAllRooms->b_value ) )
	{
		for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			assert( room != NULL );
			if ( room == NULL || room->isDetail )
				continue;

			DrawRoomStencilShadowPass( room, camera, light );
		}
	}
	else
		DrawRoomStencilShadowPass( camera->room, camera, light );

	PlPopMatrix();
}

void apeDrawWorld_( ApeWorld *world, ApeCamera *camera, ApeLight *light, bool ambienceOnly )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	PL_GET_CVAR( "world/showAllRooms", showAllRooms );
	if ( camera->room == NULL || ( showAllRooms != NULL && showAllRooms->b_value ) )
	{
		for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			assert( room != NULL );
			if ( room == NULL || room->isDetail )
				continue;

			DrawRoom( world, room, camera, true, light, ambienceOnly );
		}
	}
	else
	{
		PL_GET_CVAR( "world/skipPortals", skipPortals );
		DrawRoom( world, camera->room, camera, ( skipPortals != NULL ) ? skipPortals->b_value : false, light, ambienceOnly );
	}

	PlPopMatrix();
}
