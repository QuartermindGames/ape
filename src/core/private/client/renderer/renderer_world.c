// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "renderer.h"
#include "world/world.h"
#include "legacy/actor.h"

/****************************************
 ****************************************/

static void DrawFace( OgeWorldMesh *mesh, OgeWorldFace *face )
{
	/* Gee, this sure isn't efficient, is it?
	 * In the long-term I want to store triangulated geom
	 * on the GPU as chunks, but for now this seems to be
	 * fast enough for what I need
	 * */
	unsigned int numTriangles;
	unsigned int *indices  = ogeWorld_ConvertFaceToTriangles( face, &numTriangles );
	unsigned int *curIndex = indices;
	for ( unsigned int k = 0; k < numTriangles; ++k, curIndex += 3 )
	{
		PlgAddMeshTriangle( mesh->drawMesh, curIndex[ 0 ], curIndex[ 1 ], curIndex[ 2 ] );
	}
	PL_DELETE( indices );

	oge_RendererPerformance_.numFacesDrawn++;

#if 0// draw face normal
	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_VERTEX ] );
	PlgDrawLine( *PlGetMatrix( PL_MODELVIEW_MATRIX ), face->origin, PL_COLOUR_BLUE, PlAddVector3( face->origin, PlScaleVector3F( face->normal, 4.0f ) ), PL_COLOUR_BLUE );
#endif
}

static void DrawFaces( OgeWorldMesh *sectorBody, PLLinkedList *visibleFaces, OgeLight *lights, unsigned int numLights, bool drawTransparent )
{
	unsigned int numBatches = sectorBody->numMaterials + 1;// Extra batch for fallback pass
	for ( unsigned int i = 0; i < numBatches; ++i )
	{
		PlgClearMeshTriangles( sectorBody->drawMesh );

		OgeMaterial *material = ( i < sectorBody->numMaterials ) ? sectorBody->materials[ i ] : ogeGetFallbackMaterial();
		if ( i < sectorBody->numMaterials && ( material == ogeGetFallbackMaterial() ) )
		{
			continue;
		}

		PLLinkedListNode *faceNode = PlGetFirstNode( visibleFaces );
		while ( faceNode != NULL )
		{
			OgeWorldFace *face = PlGetLinkedListNodeUserData( faceNode );
			if ( material != face->material || ( YnCore_World_IsFacePortal( face ) && !drawTransparent ) )// for now, skip portals...
			{
				faceNode = PlGetNextLinkedListNode( faceNode );
				continue;
			}

			DrawFace( sectorBody, face );

			faceNode = PlGetNextLinkedListNode( faceNode );
		}

		if ( sectorBody->drawMesh->num_triangles == 0 )
			continue;

		ogeMaterial_DrawMesh( material, sectorBody->drawMesh, lights, 0 );
	}

#if 0
	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_VERTEX ] );
	PlgDrawMeshNormals( sectorBody->drawMesh );
#endif
}

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
void ogeDrawWorldWireframe( OgeWorld *world, OgeCamera *camera )
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

static void DrawRoom( OgeWorld *world, OgeWorldRoom *room )
{
	OgeCamera *camera = ogeGetActiveCamera();
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
		PlgDrawBoundingVolume( &room->bounds, &roomColour );
	}

	if ( !PlgIsBoxInsideView( camera->internal, &room->bounds ) )
	{
		return;
	}

	for ( uint32_t j = 0; j < PlGetNumVectorArrayElements( room->faces ); ++j )
	{
		OgeWorldFace *face = PlGetVectorArrayElementAt( room->faces, j );
		assert( face != NULL );

		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );

		PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
		while ( faceVertexNode != NULL )
		{
			OgeWorldFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
			assert( vertex->u != NULL );

			PlgImmPushVertex( vertex->u->position.x,
			                  vertex->u->position.y,
			                  vertex->u->position.z );
			PlgImmTextureCoord( vertex->textureU, vertex->textureV );
			PlgImmColour( roomColour.r, roomColour.g, roomColour.b, 255 );

			faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
		}


		if ( face->material != NULL )
		{
			ogeMaterial_DrawMesh( face->material, mesh, NULL, 0 );
		}
		else
		{
			oge_RendererPerformance_.numTriangles = mesh->num_verts / 2;
			oge_RendererPerformance_.numBatches++;

			PlgSetShaderProgram( oge_defaultShaderPrograms_[ OGE_SHADER_DEFAULT ] );
			PlgSetTexture( ogeGetFallbackTexture(), 0 );

			PlgImmDraw();
		}
	}
}

void ogeDrawWorld( OgeWorld *world, OgeWorldRoom *originSector, OgeCamera *camera )
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
		PlgSetShaderProgram( oge_defaultShaderPrograms_[ OGE_SHADER_DEFAULT_VERTEX ] );
		PlgSetTexture( NULL, 0 );
	}
	else
	{
		PlgSetShaderProgram( oge_defaultShaderPrograms_[ OGE_SHADER_DEFAULT ] );
		PlgSetTexture( ogeGetFallbackTexture(), 0 );
	}

	PL_GET_CVAR( "world.drawRooms", drawRooms );
	if ( drawRooms != NULL && drawRooms->b_value && world->rooms != NULL )
	{
		for ( uint32_t i = 0; i < PlGetNumVectorArrayElements( world->rooms ); ++i )
		{
			OgeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, i );
			assert( room != NULL );
			if ( room == NULL )
			{
				continue;
			}

			DrawRoom( world, room );
		}
	}

	PlPopMatrix();
}
