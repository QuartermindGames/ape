/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "map.h"
#include "client/renderer/renderer.h"

bool Map_CheckCollisions( const PLCollisionAABB *bounds, unsigned int curArea )
{
#if 0
	const MapSector *area = &mapData.sectors[ curArea ];
	for( unsigned int j = 0; j < area->numLines; ++j ) {
		MapPolygon  *line = &mapData.polygons[ area->lineIndices[ j ] ];
		MapVertex *startPoint = &mapData.vertices[ line->startVertex ];
		MapVertex *endPoint = &mapData.vertices[ line->endVertex ];

		//bool hit = plIsAABBIntersectingLine( bounds, &PLVector2( startPoint->x, startPoint->y ), &PLVector2( endPoint->x, endPoint->y ), &line->normal );
		float hitValue;
		bool hit = plIsPointIntersectingLine( &PLVector2( bounds->origin.x, bounds->origin.z ), &PLVector2( startPoint->x, startPoint->y ), &PLVector2( endPoint->x, endPoint->y ), &line->normal, &hitValue );
		if( hit ) {
			//PrintMsg( "HIT: %f\n", hitValue );
			//return true;
		}

		//PrintMsg( "NO HIT: %f\n", hitValue );
	}
#endif
	return false;
}

static PLVector3 GetOriginPointFromVertices( const PLGVertex *vertices, unsigned int numVertices )
{
	/* we can cheat this a little bit by depending on the bounding box we generate */
	return PlgGenerateAabbFromVertices( vertices, numVertices, true ).absOrigin;
}

#if 0
void Map_DrawSector( PLGCamera *camera, const MapSector *sector, bool smPass )
{
	for ( unsigned int i = 0; i < mapData.numMaterials; ++i )
	{
		PlgClearMesh( renderMesh );

		for ( unsigned int j = 0; j < mapData.numFaces; ++j )
		{
			MapFace *curFace = &mapData.faces[ j ];

			curFace->bounds.origin = PLVector3( 0, 0, 0 );

			if ( curFace->material != mapData.materials[ i ] )
			{
				continue;
			}

			/* check the face is actually visible */
			if ( !PlgIsBoxInsideView( camera, &curFace->bounds ) )
			{
				continue;
			}

			unsigned int numVertices;
			PLGVertex *	 vertices = PlgGetPolygonVertices( curFace->polygon, &numVertices );
			for ( unsigned int k = 0; k < numVertices; ++k )
			{
				unsigned int v = PlgAddMeshVertex( renderMesh, vertices[ k ].position, vertices[ k ].normal, vertices[ k ].colour, vertices[ k ].st[ 0 ] );
				/* this shit is generated earlier in the process, and right now I'm not sure if it's appropriate to add to AddMeshVertex */
				renderMesh->vertices[ v ].tangent	= vertices[ k ].tangent;
				renderMesh->vertices[ v ].bitangent = vertices[ k ].bitangent;
			}

			unsigned int  numTriangles;
			unsigned int *indices  = PlgConvertPolygonToTriangles( curFace->polygon, &numTriangles );
			unsigned int *curIndex = indices;
			for ( unsigned int k = 0; k < numTriangles; ++k )
			{
				PlgAddMeshTriangle( renderMesh,
									curIndex[ 0 ] + renderMesh->num_verts - numVertices,
									curIndex[ 1 ] + renderMesh->num_verts - numVertices,
									curIndex[ 2 ] + renderMesh->num_verts - numVertices );
				curIndex += 3;
			}
			globalSystem.Free( indices );
		}

		if ( renderMesh->num_triangles == 0 )
		{
			continue;
		}

		Material *material = mapData.materials[ i ];
		if ( smPass )
		{
			material = RM_CacheMaterial( "materials/engine/simple.mat", 0, true );
		}

		RM_DrawMesh( material, renderMesh );
	}
}
#endif

static void Map_SetupScene( PLGCamera *camera )
{
	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_LIGHTING_PASS ] );

	PLGShaderProgram *program = PlgGetCurrentShaderProgram();
	if ( program == NULL )
	{
		return;
	}

	int numLights = 0;
	PlgSetShaderUniformValue( program, "numLights", &numLights, false );

#if 0
	srand( numLights );
	for ( unsigned int i = 0; i < numLights; ++i ) {
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), "lights[%d].colour", i );
		plSetShaderUniformValue( program, buf, &PLVector4(
		                                               plByteToFloat( rand() % 255 ),
		                                               plByteToFloat( rand() % 255 ),
		                                               plByteToFloat( rand() % 255 ), 1.5f ), false );
	}

	PLVector3 lightPosition;
	lightPosition = PLVector3(
			-128 + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			32 + sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			-128 - -sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f
	);
	plSetShaderUniformValue( program, "lights[0].position", &lightPosition, false );

	lightPosition = PLVector3(
			-32 - sinf( Engine_GetNumTicks() / 32.0f ) * 100.0f,
			64,
			32 - -sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f
	);
	plSetShaderUniformValue( program, "lights[1].position", &lightPosition, false );

	lightPosition = PLVector3(
			64 + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			64,
			-64 - -sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f
	);
	plSetShaderUniformValue( program, "lights[2].position", &lightPosition, false );
#endif
}

#if 0
PLMatrix4 transform = plMatrix4Identity();

	plSetTexture( mapData.staticMeshes[ 0 ]->texture, 0);
	plSetNamedShaderUniformMatrix4(NULL, "pl_model", transform, true);
	plUploadMesh( mapData.staticMeshes[ 0 ] );

	plDrawMesh( mapData.staticMeshes[ 0 ] );

	plSetTexture( NULL, 0 );

	for ( unsigned int i = 0; i < mapData.staticMeshes[ 0 ]->num_verts; ++i ) {
		Gfx_EnableShaderProgram( SHADER_GENERIC );

		PLVector3 linePos = mapData.staticMeshes[ 0 ]->vertices[ i ].position;
		PLVector3 lineEndPos = plAddVector3( linePos, plScaleVector3f( mapData.staticMeshes->vertices[ i ].normal, 64.0f ) );

		plDrawSimpleLine( &transform, &linePos, &lineEndPos, &PLColour( 255, 0, 0, 255 ) );

		Gfx_EnableShaderProgram( SHADER_LIT );
	}

	/* prototype only supports a single wall texture at a time */
	srand( mapData.numTextures );
	PLTexture *floorTexture = mapData.textures[ rand() % mapData.numTextures ];
	PLTexture *ceilingTexture = mapData.textures[ rand() % mapData.numTextures ];

	for ( unsigned int i = 0; i < mapData.numSectors; ++i ) {
		const MapSector *area = &mapData.sectors[ i ];
		for ( unsigned int j = 0; j < area->numLines; ++j ) {
			MapVertex *startPoint = &mapData.vertices[ mapData.polygons[ area->lineIndices[ j ] ].startVertex ];
			MapVertex *endPoint = &mapData.vertices[ mapData.polygons[ area->lineIndices[ j ] ].endVertex ];

			/* ensure the wall is visible before we draw it */
			bool aVisible = Player_IsPointVisible( player, &PLVector2( startPoint->x, startPoint->y ) );
			bool bVisible = Player_IsPointVisible( player, &PLVector2( endPoint->x, endPoint->y ) );
			if ( !aVisible && !bVisible ) {
				continue;
			}

			PLTexture *wallTexture = mapData.textures[ mapData.polygons[ area->lineIndices[ j ] ].textureId ];
			if ( wallTexture == NULL ) {
				wallTexture = Gfx_GetWallTexture( 10 );
			}

			/* in the long term this should obviously all just get batched... */
			unsigned int wallHeight = wallTexture->h * 2;
			plDrawTexturedQuad(
					&PLVector3( startPoint->x, wallHeight, startPoint->y ),
					&PLVector3( endPoint->x, wallHeight, endPoint->y ),
					&PLVector3( startPoint->x, 0, startPoint->y ),
					&PLVector3( endPoint->x, 0, endPoint->y ),
					2, 2,
					wallTexture
			);

#ifdef DEBUG_WALL_NORMALS
			Gfx_EnableShaderProgram( SHADER_GENERIC );

			PLVector2 linePos;
			linePos = plAddVector2( PLVector2( startPoint->x, startPoint->y ), PLVector2( endPoint->x, endPoint->y ) );
			linePos = plDivideVector2f( &linePos, 2.0f );

			PLVector2 lineEndPos;
			lineEndPos = plAddVector2( linePos, plScaleVector2f( &mapData.lines[ area->lineIndices[ j ] ].normal, 64.0f ) );

			PLMatrix4 transform = plMatrix4Identity();
			plDrawSimpleLine( &transform, &PLVector3( linePos.x, 16.0f, linePos.y ), &PLVector3( lineEndPos.x, 16.0f, lineEndPos.y ), &PLColour( 255, 0, 0, 255 ) );

			Gfx_EnableShaderProgram( SHADER_LIT );
#endif
		}

		/* draw the ceiling and floor */
		plDrawTexturedQuad(
				&PLVector3( area->max[ 0 ], 0.0f, area->max[ 1 ] ),
				&PLVector3( area->min[ 0 ], 0.0f, area->max[ 1 ] ),
				&PLVector3( area->max[ 0 ], 0.0f, area->min[ 1 ] ),
				&PLVector3( area->min[ 0 ], 0.0f, area->min[ 1 ] ),
				2, 2,
				floorTexture
		);
#ifndef DEBUG_CAM
		plDrawTexturedQuad(
				&PLVector3( area->max[ 0 ], 128.0f, area->max[ 1 ] ),
				&PLVector3( area->min[ 0 ], 128.0f, area->max[ 1 ] ),
				&PLVector3( area->max[ 0 ], 128.0f, area->min[ 1 ] ),
				&PLVector3( area->min[ 0 ], 128.0f, area->min[ 1 ] ),
				2, 2,
				ceilingTexture
		);
#endif
	}
#endif
