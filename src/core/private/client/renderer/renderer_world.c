// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
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

static PLVectorArray *lights = NULL;

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

/**
 * Batches all of the detail room draws together for the given room,
 * which sadly isn't super efficient but it'll do for now.
 */
static void DrawDetailRooms( ApeWorld *world, ApeWorldRoom *room )
{
	PL_GET_CVAR( "world.drawDetailRooms", drawDetailRooms );
	if ( drawDetailRooms == NULL || !drawDetailRooms->b_value )
	{
		return;
	}

	ApeCamera *camera = ogeGetActiveCamera();
	if ( camera == NULL )
	{
		return;
	}

	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( room->detailRooms ); ++i )
	{
		ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( room->detailRooms, i );
		assert( detailRoom != NULL );
		if ( detailRoom == NULL )
		{
			continue;
		}

		for ( uint32_t j = 0; j < PlGetNumVectorArrayElements( world->materials ); ++j )
		{
			ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, j );
			assert( material != NULL );
			if ( material == NULL )
			{
				continue;
			}

			uint32_t numFaces = PlGetNumVectorArrayElements( room->faces );

			PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );

			if ( mesh->subMeshes == NULL )
			{
				mesh->maxSubMeshes   = ( numFaces * 2 );
				mesh->subMeshes      = PL_NEW_( int32_t, mesh->maxSubMeshes );
				mesh->firstSubMeshes = PL_NEW_( int32_t, mesh->maxSubMeshes );
			}
			else if ( numFaces > mesh->maxSubMeshes )
			{
				mesh->maxSubMeshes   = ( numFaces * 2 );
				mesh->subMeshes      = PL_REALLOCA( mesh->subMeshes, numFaces );
				mesh->firstSubMeshes = PL_REALLOCA( mesh->firstSubMeshes, numFaces );
			}
			mesh->numSubMeshes = 0;

#if 0
			for ( uint32_t k = 0; k < PlGetNumVectorArrayElements( detailRoom->faces ); ++k )
			{
				OgeWorldFace *face = PlGetVectorArrayElementAt( detailRoom->faces, k );
				assert( face != NULL );
				if ( face == NULL || face->material != material || !PlgIsBoxInsideView( camera->internal, &face->bounds ) )
				{
					continue;
				}

				PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
				while ( faceVertexNode != NULL )
				{
					OgeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
					assert( vertex->u != NULL );

					PlgImmPushVertex( vertex->u->position.x, vertex->u->position.y, vertex->u->position.z );
					PlgImmNormal( face->normal.x, face->normal.y, face->normal.z );
					PlgImmTextureCoord( vertex->textureU, vertex->textureV );
					PlgImmColour( 255, 255, 255, 255 );

					faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
				}

				unsigned int numVertices                   = PlGetNumLinkedListNodes( face->edgeLoop );
				mesh->firstSubMeshes[ mesh->numSubMeshes ] = ( mesh->numSubMeshes > 0 ) ? ( mesh->firstSubMeshes[ mesh->numSubMeshes - 1 ] + ( int ) numVertices ) : 0;
				mesh->subMeshes[ mesh->numSubMeshes ]      = ( int ) numVertices;

				mesh->numSubMeshes++;
			}
#endif

			PlClearVectorArray( lights );
			for ( uint32_t k = 0; k < PlGetNumVectorArrayElements( world->lights ); ++k )
			{
				ApeLight *light = PlGetVectorArrayElementAt( world->lights, k );
				if ( !PlIsPointIntersectingAabb( &room->bounds, light->position ) )
				{
					continue;
				}

				PlPushBackVectorArrayElement( lights, light );
				if ( PlGetNumVectorArrayElements( lights ) >= 8 )
				{
					break;
				}
			}

			apeDrawMesh( material, mesh, ( ApeLight ** ) PlGetVectorArrayData( lights ), PlGetNumVectorArrayElements( lights ) );

			mesh->numSubMeshes = 0;
		}
	}
}

static void DrawRoom( ApeWorld *world, ApeWorldRoom *room )
{
	if ( PlIsVectorArrayEmpty( room->faces ) )
	{
		return;
	}

	ApeCamera *camera = ogeGetActiveCamera();
	if ( camera == NULL )
	{
		return;
	}

	PL_GET_CVAR( "world.showRoomColours", showRoomColours );
	PLColour roomColour;
	if ( showRoomColours != NULL && showRoomColours->b_value )
	{
		roomColour = PlCreateColour4B( rand() % 255, rand() % 255, rand() % 255, 255 );
	}
	else
	{
		roomColour = PlCreateColour4B( 255, 255, 255, 255 );
	}

	PL_GET_CVAR( "world.showRoomVolumes", showRoomVolumes );
	if ( showRoomVolumes != NULL && showRoomVolumes->b_value )
	{
		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
		PlgDrawBoundingVolume( &room->bounds, &roomColour );
	}

	if ( !PlgIsBoxInsideView( camera->internal, &room->bounds ) )
	{
		return;
	}

	if ( lights == NULL )
	{
		lights = PlCreateVectorArray( 8 );
	}

	// TODO: gross, we should handle this better rather than just overriding the world ambience
	if ( room->ambientLightDefined )
	{
		world->ambience = PlColourU8ToF32( &room->ambientLight );
	}
	else
	{
		world->ambience = WORLD_DEFAULT_AMBIENCE;
	}

	//DrawDetailRooms( world, room );

	for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->materials ); ++i )
	{
		ApeMaterial *material = PlGetVectorArrayElementAt( world->materials, i );
		assert( material != NULL );
		if ( material == NULL )
		{
			continue;
		}

		uint32_t numFaces = PlGetNumVectorArrayElements( room->faces );

		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );

#if 0
		mesh->numSubMeshes = 0;
		if ( mesh->subMeshes == NULL )
		{
			mesh->maxSubMeshes   = numFaces;
			mesh->subMeshes      = PL_NEW_( int32_t, mesh->maxSubMeshes );
			mesh->firstSubMeshes = PL_NEW_( int32_t, mesh->maxSubMeshes );
		}
		else if ( numFaces > mesh->maxSubMeshes )
		{
			mesh->maxSubMeshes   = numFaces;
			mesh->subMeshes      = PL_REALLOCA( mesh->subMeshes, numFaces );
			mesh->firstSubMeshes = PL_REALLOCA( mesh->firstSubMeshes, numFaces );
		}
#endif

		for ( uint32_t j = 0; j < PlGetNumVectorArrayElements( room->faces ); ++j )
		{
			ApeWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
			assert( face != NULL );
			if ( face->material != material || !PlgIsBoxInsideView( camera->internal, &face->bounds ) )
			{
				continue;
			}

			uint32_t numVertices = PlGetNumLinkedListNodes( face->edgeLoop );

			PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
			while ( faceVertexNode != NULL )
			{
				ApeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
				assert( vertex->u != NULL );

				PlgImmPushVertex( vertex->u->position.x, vertex->u->position.y, vertex->u->position.z );
				PlgImmNormal( face->normal.x, face->normal.y, face->normal.z );
				PlgImmTextureCoord( vertex->textureU, vertex->textureV );
				PlgImmColour( roomColour.r, roomColour.g, roomColour.b, 255 );

				faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
			}

			//mesh->firstSubMeshes[ mesh->numSubMeshes ] = ( mesh->numSubMeshes > 0 ) ? ( mesh->firstSubMeshes[ mesh->numSubMeshes - 1 ] + ( int ) numVertices ) : 0;
			//mesh->subMeshes[ mesh->numSubMeshes ]      = ( int ) numVertices;
			//mesh->numSubMeshes++;
		}

		PlClearVectorArray( lights );
		for ( uint32_t k = 0; k < PlGetNumVectorArrayElements( world->lights ); ++k )
		{
			ApeLight *light = PlGetVectorArrayElementAt( world->lights, k );
			if ( !PlIsPointIntersectingAabb( &room->bounds, light->position ) )
			{
				continue;
			}

			PlPushBackVectorArrayElement( lights, light );
			if ( PlGetNumVectorArrayElements( lights ) >= 8 )
			{
				break;
			}
		}

		apeDrawMesh( material, mesh, ( ApeLight ** ) PlGetVectorArrayData( lights ), PlGetNumVectorArrayElements( lights ) );

		//mesh->numSubMeshes = 0;
	}
}

void apeDrawWorld_( ApeWorld *world )
{
	PL_GET_CVAR( "world.draw", drawWorld );
	if ( world == NULL || ( drawWorld != NULL && !drawWorld->b_value ) )
	{
		return;
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	PL_GET_CVAR( "world.showRoomColours", showRoomColours );
	if ( showRoomColours != NULL && showRoomColours->b_value )
	{
		srand( PlGetNumVectorArrayElements( world->rooms ) );
		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
		PlgSetTexture( NULL, 0 );
	}
	else
	{
		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );
		PlgSetTexture( apeGetFallbackTexture(), 0 );
	}

	// TODO: generate a list of visible rooms based on camera position

	PL_GET_CVAR( "world.drawRooms", drawRooms );
	if ( drawRooms != NULL && drawRooms->b_value && world->rooms != NULL )
	{
		for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			assert( room != NULL );
			if ( room == NULL || room->isDetail )
			{
				continue;
			}

			DrawRoom( world, room );
		}
	}

	PlPopMatrix();
}
