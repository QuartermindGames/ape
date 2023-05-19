// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "renderer.h"
#include "world.h"
#include "renderer_visibility.h"
#include "legacy/actor.h"

/****************************************
 * SKY
 ****************************************/

static void DrawSkyLayer( PLGMesh *mesh, YNCoreMaterial *material, const PLVector3 *location, float x, float y, float scale )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlTranslateMatrix( *location );

	/* todo: do this in shader... */
	PlgGenerateTextureCoordinates( mesh->vertices, mesh->num_verts, PLVector2( x, y ), PLVector2( scale, scale ) );
	mesh->isDirty = true;
	YnCore_Material_DrawMesh( material, mesh, NULL, 0 );

	PlPopMatrix();
}

/**
 * Draw scrolling clouds.
 */
static void DrawSky( YNCoreWorld *world, YNCoreCamera *camera )
{
	if ( world->numSkyMaterials == 0 )
	{
		return;
	}

	static PLGMesh *skyMesh = NULL;
	if ( skyMesh == NULL )
	{
		static unsigned int indices[][ 3 ] = {
  /* corners */
		        {2,  1, 0},
		        { 3, 1, 2},
		        { 4, 3, 2},
		        { 5, 3, 4},
		        { 6, 5, 4},
		        { 7, 5, 6},
		        { 0, 7, 6},
		        { 1, 7, 0},
 /* middle */
		        { 4, 2, 0},
		        { 6, 4, 0},
		};
		unsigned int numTriangles = PL_ARRAY_ELEMENTS( indices );

		skyMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, numTriangles, 8 );
		if ( skyMesh == NULL )
		{
			PRINT_ERROR( "Failed to create sky mesh!\nPL: %s\n", PlGetError() );
		}

		PlgAddMeshVertex( skyMesh, &PLVector3( 100.0f, 10.0f, 100.0f ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &pl_vecOrigin2 );   /* top right */
		PlgAddMeshVertex( skyMesh, &PLVector3( 200.0f, 10.0f, 200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );    /* top right far */
		PlgAddMeshVertex( skyMesh, &PLVector3( 100.0f, 10.0f, -100.0f ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &pl_vecOrigin2 );  /* lower right */
		PlgAddMeshVertex( skyMesh, &PLVector3( 200.0f, 10.0f, -200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );   /* lower right far */
		PlgAddMeshVertex( skyMesh, &PLVector3( -100.0f, 10.0f, -100.0f ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &pl_vecOrigin2 ); /* lower left */
		PlgAddMeshVertex( skyMesh, &PLVector3( -200.0f, 10.0f, -200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );  /* lower left far */
		PlgAddMeshVertex( skyMesh, &PLVector3( -100.0f, 10.0f, 100.0f ), &pl_vecOrigin3, &PL_COLOUR_WHITE, &pl_vecOrigin2 );  /* top left */
		PlgAddMeshVertex( skyMesh, &PLVector3( -200.0f, 10.0f, 200.0f ), &pl_vecOrigin3, &PLColourA( 0 ), &pl_vecOrigin2 );   /* top left far */

		for ( unsigned int i = 0; i < numTriangles; ++i )
		{
			PlgAddMeshTriangle( skyMesh, indices[ i ][ 0 ], indices[ i ][ 1 ], indices[ i ][ 2 ] );
		}

		PlgUploadMesh( skyMesh );
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );
	PlgSetDepthMask( false );

	PL_GET_CVAR( "r.skyheightoffset", skyHeightOffset );

	PLVector3 location;
	location = camera->internal->position;
	location.y += skyHeightOffset->f_value;

	float ticks = ( float ) YnCore_GetNumTicks();

	DrawSkyLayer( skyMesh, world->skyMaterials[ 0 ], &location, ticks / 700.0f, ticks / 400.0f, 0.15f );

	if ( world->numSkyMaterials > 1 )
	{
#if 1
		location.y += 2.0f;
		DrawSkyLayer( skyMesh, world->skyMaterials[ 1 ], &location, ( ticks / 100.0f ) * -1, ticks / 100.0f, 0.45f );
#else
		W_DrawSkyLayer( skyMesh, world->skyMaterials[ 1 ], &PLVector3( 0.0f, camera->internal->position.y + skyHeightOffset->f_value - 30.0f, 0.0f ), ( ticks / 500.0f ) * -1, ticks / 500.0f, 0.45f );
#endif
	}

	if ( world->numSkyMaterials > 2 )
	{
		location.y += 4.0f;
		DrawSkyLayer( skyMesh, world->skyMaterials[ 2 ], &location, camera->internal->position.x / 100.0f, camera->internal->position.z / 100.0f, 0.01f );
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgSetDepthMask( true );
}

/****************************************
 ****************************************/

static void DrawFace( YNCoreWorldMesh *mesh, YNCoreWorldFace *face )
{
	/* Gee, this sure isn't efficient, is it?
	 * In the long-term I want to store triangulated geom
	 * on the GPU as chunks, but for now this seems to be
	 * fast enough for what I need
	 * */
	unsigned int numTriangles;
	unsigned int *indices  = YnCore_World_ConvertFaceToTriangles( face, &numTriangles );
	unsigned int *curIndex = indices;
	for ( unsigned int k = 0; k < numTriangles; ++k, curIndex += 3 )
	{
		PlgAddMeshTriangle( mesh->drawMesh, curIndex[ 0 ], curIndex[ 1 ], curIndex[ 2 ] );
	}
	PL_DELETE( indices );

	g_gfxPerfStats.numFacesDrawn++;

#if 0// draw face normal
	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_VERTEX ] );
	PlgDrawLine( *PlGetMatrix( PL_MODELVIEW_MATRIX ), face->origin, PL_COLOUR_BLUE, PlAddVector3( face->origin, PlScaleVector3F( face->normal, 4.0f ) ), PL_COLOUR_BLUE );
#endif
}

static void DrawFaces( YNCoreWorldMesh *sectorBody, PLLinkedList *visibleFaces, YNCoreLight *lights, unsigned int numLights, bool drawTransparent )
{
	unsigned int numBatches = sectorBody->numMaterials + 1;// Extra batch for fallback pass
	for ( unsigned int i = 0; i < numBatches; ++i )
	{
		PlgClearMeshTriangles( sectorBody->drawMesh );

		YNCoreMaterial *material = ( i < sectorBody->numMaterials ) ? sectorBody->materials[ i ] : YnCore_GetFallbackMaterial();
		if ( i < sectorBody->numMaterials && ( material == YnCore_GetFallbackMaterial() ) )
		{
			continue;
		}

		PLLinkedListNode *faceNode = PlGetFirstNode( visibleFaces );
		while ( faceNode != NULL )
		{
			YNCoreWorldFace *face = PlGetLinkedListNodeUserData( faceNode );
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

		YnCore_Material_DrawMesh( material, sectorBody->drawMesh, lights, 0 );
	}

#if 0
	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_VERTEX ] );
	PlgDrawMeshNormals( sectorBody->drawMesh );
#endif
}

static void DrawSector( YNCoreWorld *world, YNCoreWorldSector *sector, YNCoreCamera *camera );
static void DrawSectorBody( YNCoreWorldSector *sector, YNCoreWorldMesh *worldMesh, YNCoreCamera *camera )
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
	g_gfxPerfStats.numVisiblePortals += PlGetNumLinkedListNodes( visiblePortals );

	unsigned int numLights;
	YNCoreLight *lights = YnCore_WorldSector_GetVisibleLights( sector, &numLights );

	// Draw transparent surfaces
	//DrawFaces( worldMesh, visiblePortals, lights, numLights, true );

	PLLinkedListNode *faceNode = PlGetFirstNode( visiblePortals );
	while ( faceNode != NULL )
	{
		YNCoreWorldFace *face = PlGetLinkedListNodeUserData( faceNode );
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

#if 0
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
#else
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
#endif

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

static void DrawSector( YNCoreWorld *world, YNCoreWorldSector *sector, YNCoreCamera *camera )
{
	if ( sector == NULL )
	{
		return;
	}

	DrawSectorBody( sector, sector->mesh, camera );

	Act_DrawActors( camera, sector );
	YnCore_EntityManager_Draw( camera, sector );
}

/**
 * World is drawn using polygons, rather than straight up triangles,
 * so to more accuratly display it in wireframe, we'll need to render
 * it in such a mode ourselves. This is mostly for the sake of the
 * editor.
 */
void YnCore_World_DrawWireframe( YNCoreWorld *world, YNCoreCamera *camera )
{
	if ( world == NULL )
		return;

	PlgSetShaderProgram( oge_defaultShaderPrograms[ OGE_SHADER_DEFAULT_VERTEX ] );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	PlgSetTexture( NULL, 0 );

	PlgImmBegin( PLG_MESH_LINES );
	for ( unsigned int i = 0; i < world->numSectors; ++i )
	{
		if ( world->sectors[ i ].mesh == NULL )
		{
			continue;
		}

		YNCoreWorldMesh *mesh  = world->sectors[ i ].mesh;
		PLLinkedListNode *node = PlGetFirstNode( mesh->faces );
		while ( node != NULL )
		{
			YNCoreWorldFace *face = PlGetLinkedListNodeUserData( node );
			for ( unsigned int j = 0; j < face->numVertices; ++j )
			{
				YNCoreWorldVertex *a = &mesh->vertices[ face->vertices[ j ] ];
				YNCoreWorldVertex *b = ( ( j + 1 ) < face->numVertices ) ? &mesh->vertices[ face->vertices[ j + 1 ] ] : &mesh->vertices[ face->vertices[ 0 ] ];

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
	for ( unsigned int i = 0; i < world->numSectors; ++i )
	{
		if ( world->sectors[ i ].mesh == NULL )
		{
			continue;
		}

		YNCoreWorldMesh *mesh  = world->sectors[ i ].mesh;
		PLLinkedListNode *node = PlGetFirstNode( mesh->faces );
		while ( node != NULL )
		{
			YNCoreWorldFace *face = PlGetLinkedListNodeUserData( node );
			for ( unsigned int j = 0; j < face->numVertices; ++j )
			{
				YNCoreWorldVertex *a = &mesh->vertices[ face->vertices[ j ] ];
				PlgImmPushVertex( a->position.x, a->position.y, a->position.z );
				PlgImmColour( 0, 255, 0, 255 );
			}

			node = PlGetNextLinkedListNode( node );
		}
	}
	PlgImmDraw();

	PlPopMatrix();
}

void YnCore_World_Draw( YNCoreWorld *world, YNCoreWorldSector *originSector, YNCoreCamera *camera )
{
	if ( world == NULL )
	{
		return;
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	DrawSky( world, camera );

	PL_GET_CVAR( "world.drawSectorVolumes", drawSectorVolumes );
	DrawSector( world, originSector, camera );

	PlPopMatrix();
}
