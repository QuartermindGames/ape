/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/pl_graphics.h>

#include "yin.h"
#include "map.h"
#include "gfx.h"
#include "act.h"
#include "game.h"
#include "image.h"

static struct {
	MapSector       *sectors;
	unsigned int    numSectors;
	MapVertex       *vertices;
	unsigned int    numVertices;
	MapFace         *faces;
	unsigned int    numFaces;

	/* textures */
	PLFileSystemMount   **texturePackages;
	unsigned int        numTexturePackages;
	PLTexture           **textures;
	unsigned int        numTextures;
} mapData = {
		.vertices       = NULL,
		.numVertices    = 0,
		.faces          = NULL,
		.numFaces       = 0,
};

static PLMesh *renderMesh = NULL;

static void Map_LoadTextures( PLPackage *mapPkg ) {
	/* all of the texture packages this map is using, eventually
	 * these will be loaded from the map itself */
	static const char *texturePackagePaths[]={
		"Textures/Blasphemer.pkg",
	};

	/* mount all of the packages we're going to use */
	mapData.numTexturePackages = plArrayElements( texturePackagePaths );
	PLFileSystemMount **texturePackages = Sys_AllocateMemory( mapData.numTexturePackages, sizeof( PLFileSystemMount* ) );
	for ( unsigned int i = 0; i < mapData.numTexturePackages; ++i ) {
		texturePackages[ i ] = plMountLocation( texturePackagePaths[ i ] );
	}

	/* all the textures the map is using, eventually these will be loaded
	 * from the map itself */
	static const char *textureNames[]={
		"Walls:woodsl04.gif",
		"Engine:DefaultTile.gfx",
		"Walls:wall515.gif",
		"Walls:woodsl07.gif",
		"Walls:woodsl10.gif",
	};

	mapData.numTextures = plArrayElements( textureNames );
	mapData.textures = Sys_AllocateMemory( mapData.numTextures, sizeof( PLTexture* ) );

	for ( unsigned int i = 0; i < mapData.numTextures; ++i ) {
		PLImage *image = Image_LoadPackedImage( textureNames[ i ] );
		if ( image == NULL ) {
			mapData.textures[ i ] = Gfx_GetWallTexture( 10 );
			continue;
		}

		mapData.textures[ i ] = plCreateTexture();
		if ( mapData.textures[ i ] == NULL ) {
			plDestroyImage( image );
			mapData.textures[ i ] = Gfx_GetWallTexture( 10 );
			continue;
		}

		plUploadTextureImage( mapData.textures[ i ], image );

		plDestroyImage( image );
	}
}

static void Map_LoadVertices( PLPackage *mapPkg ) {
	PLFile *filePtr = plLoadPackageFile( mapPkg, "Data:Vertices" );
	if ( filePtr == NULL ) {
		PrintError( "Failed to open vertex data in \"%s\"!\nPL: %s\n", plGetPackagePath( mapPkg ), plGetError() );
	}

	bool status;
	mapData.numVertices = plReadInt32( filePtr, false, &status );
	if ( !status ) {
		PrintError( "Failed to fetch number of vertices in \"%s\"!\nPL: %s\n", plGetPackagePath( mapPkg ), plGetError() );
	}

	mapData.vertices = Sys_AllocateMemory( mapData.numVertices, sizeof( MapVertex ) );
	for ( unsigned int i = 0; i < mapData.numVertices; ++i ) {
		mapData.vertices[ i ].x = plReadInt32( filePtr, false, &status );
		mapData.vertices[ i ].y = plReadInt32( filePtr, false, &status );
		mapData.vertices[ i ].z = plReadInt32( filePtr, false, &status );
	}

	if ( !status ) {
		PrintError( "Failed to read in all vertices in \"%s\"!\nPL: %s\n", plGetPackagePath( mapPkg ), plGetError() );
	}
}

static void Map_LoadFaces( PLPackage *mapPkg ) {
	PLFile *filePtr = plLoadPackageFile( mapPkg, "Data:Polygons" );
	if ( filePtr == NULL ) {
		PrintError( "Failed to open primitive data!\nPL: %s\n", plGetError() );
	}

	bool status;
	mapData.numFaces = plReadInt32( filePtr, false, &status );
	if ( !status ) {
		PrintError( "Failed to fetch number of primitives!\nPL: %s\n", plGetError() );
	}

	mapData.faces = Sys_AllocateMemory( mapData.numFaces, sizeof( MapFace ) );
	for ( unsigned int i = 0; i < mapData.numFaces; ++i ) {
		MapFace *curFace = &mapData.faces[ i ];

		curFace->flags = plReadInt8( filePtr, &status );

		uint32_t textureId = plReadInt32( filePtr, false, &status );
		if ( textureId >= mapData.numTextures ) {
			PrintWarn( "Invalid texture id %d for polygon %d!\n", textureId, i );
			curFace->texture = Gfx_GetFallbackTexture();
		} else {
			curFace->texture = mapData.textures[ textureId ];
		}

		curFace->textureOffset.x = ( float ) plReadInt32( filePtr, false, &status );
		curFace->textureOffset.y = ( float ) plReadInt32( filePtr, false, &status );
		curFace->textureScale.x = ( float ) plReadInt32( filePtr, false, &status );
		curFace->textureScale.y = ( float ) plReadInt32( filePtr, false, &status );

		uint8_t numVertices = plReadInt8( filePtr, &status );

		curFace->polygon = plCreatePolygon();
		if ( curFace->polygon == NULL ) {
			PrintError( "Failed to create polygon!\n" );
		}

		for ( unsigned int j = 0; j < numVertices; ++j ) {
			uint32_t vertIndex = plReadInt32( filePtr, false, &status );
			if ( !status ) {
				PrintError( "Failed to read vertex %d!\n", j );
			}

			if ( vertIndex >= mapData.numVertices ) {
				PrintError( "Invalid vertex index!\n" );
			}

			PLVertex vertex;
			memset( &vertex, 0, sizeof( PLVertex ) );

			vertex.position.x = mapData.vertices[ vertIndex ].x * 100.0f;
			vertex.position.y = mapData.vertices[ vertIndex ].y * 100.0f;
			vertex.position.z = mapData.vertices[ vertIndex ].z * 100.0f;
			vertex.colour = PLColour( 128, 128, 0, 255 );

			plAddPolygonVertex( curFace->polygon, &vertex );
		}

		if ( !status ) {
			PrintError( "Failed to read in polygon %d in \"%s\"!\nPL: %s\n", i, plGetPackagePath( mapPkg ), plGetError() );
		}
	}
}

void Map_LoadSectors( PLPackage *wad ) {
#if 0
	/* all this for now is just dummy code. everything is treated as one sector */

	mapData.numSectors = 1; //plReadInt32( filePtr, false, &status );
	mapData.sectors = Sys_AllocateMemory( mapData.numSectors, sizeof( MapSector ) );
	for ( unsigned int i = 0; i < mapData.numSectors; ++i ) {
		MapSector *area = &mapData.sectors[ i ];

		/* generate a list of all our line indices */
		mapData.sectors[ i ].numLines = mapData.numPolygons;
		area->lineIndices = Sys_AllocateMemory( mapData.sectors[ i ].numLines, sizeof( unsigned int ) );

		area->max[ 0 ] = area->max[ 1 ] = INT32_MIN;
		area->min[ 0 ] = area->min[ 1 ] = INT32_MAX;

		for ( unsigned int j = 0; j < mapData.sectors[ i ].numLines; ++j ) {
			mapData.sectors[ i ].lineIndices[ j ] = j; //plReadInt16( filePtr, false, &status );
			if ( mapData.sectors[ i ].lineIndices[ j ] >= mapData.numPolygons ) {
				PrintError( "Invalid line index %d in area %d!\n", mapData.sectors[ i ].lineIndices[ j ], i );
			}

			MapPolygon *curLine = &mapData.polygons[ mapData.sectors[ i ].lineIndices[ j ] ];

			/* calculate the area bounds */
			const MapVertex *points[ 2 ];
			points[ 0 ] = &mapData.vertices[ curLine->startVertex ];
			points[ 1 ] = &mapData.vertices[ curLine->endVertex ];

			for ( unsigned int k = 0; k < 2; ++k ) {
				if ( points[ k ]->x > area->max[ 0 ] ) {
					area->max[ 0 ] = points[ k ]->x;
				}

				if ( points[ k ]->y > area->max[ 1 ] ) {
					area->max[ 1 ] = points[ k ]->y;
				}

				if ( points[ k ]->x < area->min[ 0 ] ) {
					area->min[ 0 ] = points[ k ]->x;
				}

				if ( points[ k ]->y < area->min[ 1 ] ) {
					area->min[ 1 ] = points[ k ]->y;
				}
			}

			/* generate the normal for this particular face */
			curLine->normal = plComputeLineNormal( &PLVector2( points[ 0 ]->x, points[ 0 ]->y ), &PLVector2( points[ 1 ]->x, points[ 1 ]->y ) );
		}
	}
#endif
}

void Map_Load( const char *path ) {
	PLPackage *mapPkg = plLoadPackage( path );
	if ( mapPkg == NULL ) {
		PrintWarn( "Failed to open map, \"%s\"!\nPL: %s\n", path, plGetError() );
		return;
	}

	Map_LoadTextures( mapPkg );
	Map_LoadVertices( mapPkg );
	Map_LoadFaces( mapPkg );
	//Map_LoadSectors( mapPkg );

	/* create our mesh container for rendering */

	unsigned int maxTriangles = 0;
	for ( unsigned int i = 0; i < mapData.numFaces; ++i ) {
		maxTriangles += plGetNumOfPolygonTriangles( mapData.faces[ i ].polygon );
	}

	renderMesh = plCreateMesh( PL_MESH_TRIANGLES, PL_DRAW_DYNAMIC, maxTriangles, maxTriangles * 3 );
	if ( renderMesh == NULL ) {
		PrintError( "Failed to create render mesh!\nPL: %s\n", plGetError() );
	}
}

bool Map_CheckCollisions( const PLCollisionAABB *bounds, unsigned int curArea ) {
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

void Map_Draw( void ) {
	/* fetch the local player so we can perform vis testing */
	Actor *player = Gam_GetPlayer();
	if ( player == NULL ) {
		return;
	}

	Gfx_EnableShaderProgram( SHADER_GENERIC );

	/* super duper slow innefficient rendering, wheeee */
	for ( unsigned int i = 0; i < mapData.numFaces; ++i ) {
		MapFace *curFace = &mapData.faces[ i ];

		plClearMesh( renderMesh );

		plSetTexture( curFace->texture, 0 );

		unsigned int numVertices;
		PLVertex *vertices = plGetPolygonVertices( curFace->polygon, &numVertices );
		for( unsigned int j = 0; j < numVertices; ++j ) {
			plAddMeshVertex( renderMesh, vertices[ j ].position, vertices[ j ].normal, vertices[ j ].colour, vertices[ j ].st[ 0 ] );
		}

		unsigned int numTriangles;
		unsigned int *indices = plConvertPolygonToTriangles( curFace->polygon, &numTriangles );
		unsigned int *curIndex = indices;
		for ( unsigned int j = 0; j < numTriangles; ++j ) {
			plAddMeshTriangle( renderMesh, curIndex[ 0 ], curIndex[ 1 ], curIndex[ 2] );
			curIndex += 3;
		}

		plGenerateMeshNormals( renderMesh, true );

		plUploadMesh( renderMesh );
		plDrawMesh( renderMesh );
	}

	Gfx_EnableShaderProgram( SHADER_GENERIC );

	plMatrixMode( PL_MODELVIEW_MATRIX );
	plLoadIdentityMatrix();

	const PLMatrix4 *transform = plGetMatrix( PL_MODELVIEW_MATRIX );
	plDrawMeshNormals( transform, renderMesh );

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
}
