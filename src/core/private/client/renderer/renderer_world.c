// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "renderer.h"
#include "world/world.h"
#include "legacy/actor.h"

#if 0
static void DrawSector( OgeWorld *world, OgeWorldRoom *sector, OgeCamera *camera );
static void DrawSectorBody( OgeWorldRoom *sector, OgeWorldMesh *worldMesh, OgeCamera *camera )
{
	if ( worldMesh == NULL )
	{
		return;
	}

	PLLinkedList *visibleFaces = VIS_GetVisibleFaces( camera, worldMesh->faces );
	if ( PlGetNumLinkedListNodes( visibleFaces ) == 0 )
	{
		return;
	}

	// Now check for portals - we'll draw these first
	PLLinkedList *visiblePortals = VIS_GetVisiblePortals( camera, visibleFaces );
	oge_RendererPerformance_.numVisiblePortals += PlGetNumLinkedListNodes( visiblePortals );

	unsigned int numLights;
	OgeLight *lights = YnCore_WorldSector_GetVisibleLights( sector, &numLights );

	// Draw transparent surfaces
	//DrawFaces( worldMesh, visiblePortals, lights, numLights, true );

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

		faceNode = PlGetNextLinkedListNode( faceNode );
	}

	// Draw solid surfaces
	DrawFaces( worldMesh, visibleFaces, lights, numLights, false );
	// Draw transparent surfaces
	DrawFaces( worldMesh, visiblePortals, lights, numLights, true );

	PlDestroyLinkedList( visiblePortals );
	visiblePortals = NULL;

	PlDestroyLinkedList( visibleFaces );
	visibleFaces = NULL;
}
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

static void DrawRoom( ApeWorld *world, ApeWorldRoom *room, ApeCamera *camera, bool skipPortals )
{
	if ( PlIsVectorArrayEmpty( room->faces ) )
		return;

	if ( !PlgIsBoxInsideView( camera->internal, &room->bounds ) )
		return;

	PL_GET_CVAR( "world/showRoomColours", showRoomColours );
	PL_GET_CVAR( "world/showRoomVolumes", showRoomVolumes );
	if ( showRoomVolumes != NULL && showRoomVolumes->b_value )
	{
		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
		PlgDrawBoundingVolume( &room->bounds, &room->colour );
	}

	// TODO: gross, we should handle this better rather than just overriding the world ambience
	if ( room->flags & APE_WORLD_ROOM_FLAG_AMBIENT )
	{
		world->ambience = PlColourU8ToF32( &room->ambientLight );
	}
	else
	{
		world->ambience = WORLD_DEFAULT_AMBIENCE;
	}

	//ape_RendererPerformance_.numLights += PlGetNumVectorArrayElements( visibleLights );

	if ( ape_config_.world.showPortals )
	{
		for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( room->faces ); ++i )
		{
			ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, i );
			assert( face != NULL );
			if ( face == NULL || face->portal == NULL )
			{
				continue;
			}

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

#if 1

	for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->materials ); ++i )
	{
		if ( room->batches[ i ].numSubMeshes == 0 )
			continue;

		ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, i );
		assert( material != NULL );
		if ( material == NULL )
			continue;

		assert( room->batches[ i ].material == material );

		unsigned int numLights = 0;
		ApeLightPointerArray lights;
		PL_ZERO_( lights );

		unsigned int numVisibleLights;
		ApeLight **visibleLights = apeGetVisibleLights_( &numVisibleLights );
		for ( uint32_t j = 0; j < numVisibleLights; ++j )
		{
			if ( numLights >= APE_MAX_LIGHTS_PER_PASS )
				break;

			ApeLight *light = visibleLights[ j ];
			if ( !PlIsPointIntersectingAabb( &room->bounds, light->position ) )
				continue;

			lights[ numLights ] = light;
			numLights++;
		}

		room->mesh->numSubMeshes   = room->batches[ i ].numSubMeshes;
		room->mesh->firstSubMeshes = room->batches[ i ].firstSubMeshes;
		room->mesh->subMeshes      = room->batches[ i ].subMeshes;

		if ( showRoomColours != NULL && showRoomColours->b_value )
			material = apeGetVertexMaterial();

		apeDrawMesh( material, room->mesh, lights, numLights );
	}

#endif

//	room->mesh->numSubMeshes   = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_ROOM ] ].numSubMeshes;
//	room->mesh->firstSubMeshes = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_ROOM ] ].firstSubMeshes;
//	room->mesh->subMeshes      = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_ROOM ] ].subMeshes;
//	apeDrawMesh( apeGetVertexMaterial(), room->mesh, NULL, 0 );

	room->mesh->numSubMeshes   = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_DETAIL ] ].numSubMeshes;
	room->mesh->firstSubMeshes = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_DETAIL ] ].firstSubMeshes;
	room->mesh->subMeshes      = room->batches[ room->builtInBatches[ APE_WORLD_ROOM_BATCH_DETAIL ] ].subMeshes;

	apeDrawMesh( apeGetShadowMaterial(), room->mesh, NULL, 0 );

	ape_rendererPerformance_.numRooms++;
}

static void DrawRoomStencilShadowPass( ApeWorld *world, ApeWorldRoom *room, ApeCamera *camera )
{

}

PLVector2 screenPosTest = { 0.0f, 0.0f };

void apeDrawWorldStencilShadowPass_( void )
{
	ApeCamera *camera = apeGetActiveCamera();
	if ( camera == NULL )
		return;

	ApeWorld *world = apeGetCurrentWorld();
	if ( world == NULL )
		return;

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

			DrawRoom( world, room, camera, true );
		}
	}
	else
	{
		PL_GET_CVAR( "world/skipPortals", skipPortals );
		DrawRoom( world, camera->room, camera, ( skipPortals != NULL ) ? skipPortals->b_value : false );
	}

	PlPopMatrix();
}

void apeDrawWorld_( ApeWorld *world )
{
	ape_rendererPerformance_.numLights = 0;

	PL_GET_CVAR( "world/draw", drawWorld );
	if ( world == NULL || ( drawWorld != NULL && !drawWorld->b_value ) )
		return;

	ApeCamera *camera = apeGetActiveCamera();
	if ( camera == NULL )
		return;

	COM_PROFILE_FUNCTION_START();

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

#if 0// test lights

	{
		static float tick = 0.0f;

		PLVector3 forward;
		PlAnglesAxes( camera->internal->angles, NULL, NULL, &forward );

		int w, h;
		apeGet2DViewportSize( &w, &h );

		ApeLight *light          = PlGetVectorArrayElementAt( world->lights, 0 );
		PLCollisionSphere sphere = PlSetupCollisionSphere( light->position, light->radius );
		if ( PlgIsSphereInsideView( camera->internal, &sphere ) )
		{
			PLMatrix4 viewProj  = PlMultiplyMatrix4( camera->internal->internal.proj, &camera->internal->internal.view );
			PLVector2 screenPos = PlConvertWorldToScreen( &light->position, &viewProj, w, h, 0, 0 );
			screenPosTest       = screenPos;
		}

		apeDrawAxesPivot( light->position, light->angles, 1.0f );

		light->position = PlAddVector3( apeGetCameraPosition( camera ), PlScaleVector3F( forward, 5.0f ) );
		light->position = PlAddVector3( light->position, ( PLVector3 ){
		                                                         sinf( apeGetNumTicks() / 5.0f ) / 10.0f,
		                                                         cosf( apeGetNumTicks() / 5.0f ) / 10.0f,
		                                                         sinf( apeGetNumTicks() / 5.0f ) / 10.0f } );
		light->colour.r = 1.0f;
		light->colour.g = 0.5f;
		light->colour.b = 0.5f;

		tick += 0.5f;
	}

#endif

	PL_GET_CVAR( "world/showAllRooms", showAllRooms );
	if ( camera->room == NULL || ( showAllRooms != NULL && showAllRooms->b_value ) )
	{
		for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			assert( room != NULL );
			if ( room == NULL || room->isDetail )
				continue;

			DrawRoom( world, room, camera, true );
		}
	}
	else
	{
		PL_GET_CVAR( "world/skipPortals", skipPortals );
		DrawRoom( world, camera->room, camera, ( skipPortals != NULL ) ? skipPortals->b_value : false );
	}

	PlPopMatrix();

	COM_PROFILE_FUNCTION_END();
}
