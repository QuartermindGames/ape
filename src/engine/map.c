/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/pl_graphics.h>

#include "yin.h"
#include "map.h"
#include "image.h"
#include "renderer/renderer.h"

#define MAP_IDENTIFIER  "map"
#define MAP_VERSION     "version 1"

static struct {
	MapSector       *sectors;
	unsigned int	maxSectors;
	unsigned int    numSectors;
	MapVertex       *vertices;
	unsigned int	maxVertices;
	unsigned int    numVertices;
	MapFace         *faces;
	unsigned int	maxFaces;
	unsigned int    numFaces;

	/* textures */
	PLFileSystemMount   **texturePackages;
	unsigned int        numTexturePackages;
	PLTexture           **textures;
	unsigned int        numTextures;
} mapData;

static PLMesh *renderMesh = NULL;

/**
 * Clears out the current map data.
 */
void Map_ClearData( void ) {
	memset( &mapData, 0, sizeof( mapData ) );
}

/**********************************************************/
/** Map File Reader **/

static bool Map_ValidateFile( PLFile *file ) {
	char buffer[ 256 ];

	/* check the identifier */
	plReadString( file, buffer, sizeof( buffer ) );
	if ( strcmp( MAP_IDENTIFIER "\n", buffer ) != 0 ) {
		PrintWarn( "Invalid identifier found, expected \"%s\" but found \"%s\"\n", MAP_IDENTIFIER, buffer );
		return false;
	}

	/* and now check the version */
	plReadString( file, buffer, sizeof( buffer ) );
	if ( strcmp( MAP_VERSION "\n", buffer ) != 0 ) {
		PrintWarn( "Invalid version found, expected \"%s\" but found \"%s\"\n", MAP_VERSION, buffer );
		return false;
	}

	return true;
}

static void Map_ParseTextures( PLFile *file ) {
	char buffer[ 256 ];

	/* figure out how many packages we have on our hands */
	plReadString( file, buffer, sizeof( buffer ) );
	if ( sscanf( buffer, "texturepacks %d\n", &mapData.numTexturePackages ) != 1 ) {
		PrintError( "Failed to fetch number of texture packages in \"%s\"!\n", plGetFilePath( file ) );
	}

	/* now mount all of the packages we're going to use */
	PLFileSystemMount **texturePackages = g_system.calloc( mapData.numTexturePackages, sizeof( PLFileSystemMount* ) );
	for ( unsigned int i = 0; i < mapData.numTexturePackages; ++i ) {
		if ( plReadString( file, buffer, sizeof( buffer ) ) == NULL ) {
			PrintError( "Failed to read in texture package \"%d\" in \"%s\"!\n", i, plGetFilePath( file ) );
		}

		/* copy the texture name; excluding the line terminator */
		char packagePath[ PL_SYSTEM_MAX_PATH ];
		for ( unsigned int j = 0; j < sizeof( packagePath ); ++j ) {
			if ( buffer[ j ] == '\n' ) {
				packagePath[ j ] = '\0';
				break;
			}

			packagePath[ j ] = buffer[ j ];
		}

		texturePackages[ i ] = plMountLocation( packagePath );
		if ( texturePackages[ i ] == NULL ) {
			PrintWarn( "Failed to mount the specified location, \"%s\"!\n", packagePath );
		}
	}

	/* now fetch all the textures the map will be using */
	plReadString( file, buffer, sizeof( buffer ) );
	if ( sscanf( buffer, "textures %d\n", &mapData.numTextures ) != 1 ) {
		PrintError( "Failed to fetch number of textures in \"%s\"!\n", plGetFilePath( file ) );
	}

	/* allocate storage for them */
	mapData.textures = g_system.calloc( mapData.numTextures, sizeof( PLTexture* ) );

	for ( unsigned int i = 0; i < mapData.numTextures; ++i ) {
		if ( plReadString( file, buffer, sizeof( buffer ) ) == NULL ) {
			PrintError( "Failed to read in texture \"%d\" in \"%s\"!\n", i, plGetFilePath( file ) );
		}

		/* copy the texture name; excluding the line terminator */
		char textureName[ 64 ];
		for ( unsigned int j = 0; j < sizeof( textureName ); ++j ) {
			if ( buffer[ j ] == '\n' ) {
				textureName[ j ] = '\0';
				break;
			}

			textureName[ j ] = buffer[ j ];
		}

		mapData.textures[ i ] = Gfx_LoadTexture( textureName );
	}
}

static void Map_ParseVertices( PLFile *file ) {
	char buffer[ 256 ];

	plReadString( file, buffer, sizeof( buffer ) );
	if ( sscanf( buffer, "vertices %d\n", &mapData.numVertices ) != 1 ) {
		PrintError( "Failed to fetch number of vertices in \"%s\"!\n", plGetFilePath( file ) );
	}

	/* allocate buffer for vertices and then read them all in */
	mapData.vertices = g_system.calloc( mapData.numVertices, sizeof( MapVertex ) );
	for ( unsigned int i = 0; i < mapData.numVertices; ++i ) {
		if ( plReadString( file, buffer, sizeof( buffer ) ) == NULL ) {
			PrintError( "Failed to read in vertex \"%d\" in \"%s\"!\n", i, plGetFilePath( file ) );
		}

		if ( sscanf( buffer, "%f %f %f", &mapData.vertices[ i ].x, &mapData.vertices[ i ].y, &mapData.vertices[ i ].z ) != 3 ) {
			PrintError( "Failed to read in vertex \"%d\" elements in \"%s\"!\n", i, plGetFilePath( file ) );
		}
	}
}

static void Map_ParseFaces( PLFile *file ) {
	char buffer[ 256 ];

	plReadString( file, buffer, sizeof( buffer ) );
	if ( sscanf( buffer, "faces %d\n", &mapData.numFaces ) != 1 ) {
		PrintError( "Failed to fetch number of faces in \"%s\"!\n", plGetFilePath( file ) );
	}

	/* allocate buffer for faces and then read them all in */
	mapData.faces = g_system.calloc( mapData.numFaces, sizeof( MapFace ) );
	for ( unsigned int i = 0; i < mapData.numFaces; ++i ) {
		if ( plReadString( file, buffer, sizeof( buffer ) ) == NULL ) {
			PrintError( "Failed to read in face \"%d\" in \"%s\"!\n", i, plGetFilePath( file ) );
		}

#define MAX_FACE_INDICES 16

		char *token = strtok( buffer, " " );
		unsigned int numIndices = strtol( token, NULL, 10 );
		if ( numIndices == 0 || numIndices >= MAX_FACE_INDICES ) {
			PrintError( "Invalid number of indices for face \"%d\" in \"%s\"!\n", i, plGetFilePath( file ) );
		}

		unsigned int faceIndices[ MAX_FACE_INDICES ];
		for ( unsigned int j = 0; j < numIndices; ++j ) {
			token = strtok( NULL, " " );
			if ( token == NULL ) {
				PrintError( "Failed to read in face \"%d\" elements in \"%s\"!\n", i, plGetFilePath( file ) );
			}

			faceIndices[ j ] = strtol( token, NULL, 10 );
		}

		/* now try to get the texture index and flags (these are optional, for now) */

		unsigned int textureIndex = 0;

		token = strtok( NULL, " " );
		if ( token != NULL ) {
			textureIndex = strtol( token, NULL, 10 );

			token = strtok( NULL, " " );
			if ( token != NULL ) {
				mapData.faces[ i ].flags = strtol( token, NULL, 10 );
			}
		}

		PLTexture *texturePtr;
		if ( textureIndex >= mapData.numTextures ) {
			PrintWarn( "Invalid texture id %d for polygon %d!\n", textureIndex, i );
			texturePtr = Gfx_GetFallbackTexture();
		} else {
			texturePtr = mapData.textures[ textureIndex ];
		}

		mapData.faces[ i ].polygon = plCreatePolygon( texturePtr, PLVector2( 0.0f, 0.0f ), PLVector2( 4.0f, 4.0f ), 0.0f );
		if ( mapData.faces[ i ].polygon == NULL ) {
			PrintError( "Failed to create polygon for face \"%d\" in \"%s\"!\n", i, plGetFilePath( file ) );
		}

		for ( unsigned int j = 0; j < numIndices; ++j ) {
			if ( faceIndices[ j ] >= mapData.numVertices ) {
				PrintError( "Invalid vertex index!\n" );
			}

			PLVertex vertex;
			memset( &vertex, 0, sizeof( PLVertex ) );

			vertex.position.x = mapData.vertices[ faceIndices[ j ] ].x * 30.0f;
			vertex.position.y = mapData.vertices[ faceIndices[ j ] ].y * 30.0f;
			vertex.position.z = mapData.vertices[ faceIndices[ j ] ].z * 30.0f;
			vertex.colour = PLColour( 255, 255, 255, 255 );

			plAddPolygonVertex( mapData.faces[ i ].polygon, &vertex );
		}

		unsigned int numPolyVertices;
		PLVertex *vertices = plGetPolygonVertices( mapData.faces[ i ].polygon, &numPolyVertices );
		if ( numPolyVertices != numIndices ) {
			PrintError( "Number of vertices from polygon did not match vertices loaded!\n" );
		}

		/* generate the bounds for cheap culling */
		mapData.faces[ i ].bounds = plGenerateAABB( vertices, numPolyVertices, true );
	}
}

void Map_ParseSectors( PLFile *file ) {
	char buffer[ 256 ];

	plReadString( file, buffer, sizeof( buffer ) );
	if ( sscanf( buffer, "sectors %d\n", &mapData.numSectors ) != 1 ) {
		PrintError( "Failed to fetch number of sectors in \"%s\"!\n", plGetFilePath( file ) );
	}

	mapData.sectors = g_system.calloc( mapData.numSectors, sizeof( MapSector ) );

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

void Map_ParseFile( PLFile *file ) {
	if ( !Map_ValidateFile( file ) ) {
		PrintWarn( "\"%s\" is not a valid map!\n", plGetFilePath( file ) );
		return;
	}

	Map_ParseTextures( file );
	Map_ParseVertices( file );
	Map_ParseFaces( file );
	Map_ParseSectors( file );
	//Map_ParseActors( file );
}

void Map_Load( const char *path ) {
	PLFile *file = plOpenFile( path, false );
	if ( file == NULL ) {
		PrintWarn( "Failed to open map, \"%s\"!\nPL: %s\n", path, plGetError() );
		return;
	}

	/* now, let's make sure it's valid! */

	Map_ParseFile( file );

	plCloseFile( file );

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

static PLVector3 GetOriginPointFromVertices( const PLVertex *vertices, unsigned int numVertices ) {
	/* we can cheat this a little bit by depending on the bounding box we generate */
	PLCollisionAABB bounds = plGenerateAABB( vertices, numVertices, true );
	return bounds.origin;
}

PLMatrix4 Map_GetPortalView( GfxCamera *camera, MapFace *source, MapFace *destination ) {
	/* so, this is a little awkward, but we need to figure out where each face is in the world
	 * by averaging the vertex coords for each and then positioning the portal at each. */

	typedef struct Portal {
		PLVector3 origin;
		PLVector3 rotation;
	} Portal;
	Portal portals[ 2 ];

	/* first, figure it out for the source */
	unsigned int numVertices;
	PLVertex *vertices;

	vertices = plGetPolygonVertices( source->polygon, &numVertices );
	portals[ 0 ].origin = GetOriginPointFromVertices( vertices, numVertices );

	vertices = plGetPolygonVertices( destination->polygon, &numVertices );
	portals[ 1 ].origin = GetOriginPointFromVertices( vertices, numVertices );

	Gfx_DrawAxesPivot( portals[ 0 ].origin, portals[ 1 ].rotation );
	Gfx_DrawAxesPivot( portals[ 1 ].origin, portals[ 1 ].rotation );

#if 0
	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();

	plRotateMatrix( 180.0f, 0.0f, 1.0f, 0.0f );

	plPopMatrix();

	return *plGetMatrix( PL_MODELVIEW_MATRIX );
#endif
}

void Map_DrawSector( GfxCamera *camera, const MapSector *sector ) {
	/* super duper slow inefficient rendering, wheeee */
	for ( unsigned int i = 0; i < mapData.numFaces; ++i ) {
		MapFace *curFace = &mapData.faces[ i ];

		if ( !plIsBoxInsideView( camera->cameraPtr, &curFace->bounds ) ) {
			continue;
		}

		if ( i == 0 ) {
			Map_GetPortalView( NULL, curFace, curFace );
		}

		curFace->bounds.origin = PLVector3( 0,0 ,0 );

		//plDrawBoundingVolume( &curFace->bounds, PL_COLOUR_BLUE );

		PLMatrix4 transform = *plGetMatrix( PL_MODELVIEW_MATRIX );
		plSetNamedShaderUniformMatrix4( NULL, "pl_model", transform, true );

		plClearMesh( renderMesh );

		plSetTexture( plGetPolygonTexture( curFace->polygon ), 0 );

		unsigned int numVertices;
		PLVertex *vertices = plGetPolygonVertices( curFace->polygon, &numVertices );
		for( unsigned int j = 0; j < numVertices; ++j ) {
			plAddMeshVertex( renderMesh, vertices[ j ].position, vertices[ j ].normal, vertices[ j ].colour, vertices[ j ].st[ 0 ] );
		}

		unsigned int numTriangles;
		unsigned int *indices = plConvertPolygonToTriangles( curFace->polygon, &numTriangles );
		unsigned int *curIndex = indices;
		for ( unsigned int j = 0; j < numTriangles; ++j ) {
			plAddMeshTriangle( renderMesh, curIndex[ 0 ], curIndex[ 1 ], curIndex[ 2 ] );
			curIndex += 3;
		}

		plUploadMesh( renderMesh );
		plDrawMesh( renderMesh );

		g_gfxPerfStats.numFacesDrawn++;
	}
}

void Map_Draw( GfxCamera *camera ) {
	if ( renderMesh == NULL ) {
		return;
	}

	Gfx_EnableShaderProgram( SHADER_LIT );

	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();
	plLoadIdentityMatrix();

	PLMatrix4 transform = *plGetMatrix( PL_MODELVIEW_MATRIX );
	plSetNamedShaderUniformMatrix4( NULL, "pl_model", transform, true );

#if 0
	static PLVector3 sunPosition = PLVector3( 32.0f, -25.0f, 25.0f );
	sunPosition.x = sinf( Engine_GetNumTicks() / 32.0f ) * 100.0f;
	plSetNamedShaderUniformVector3( NULL, "sun_position", sunPosition );
#endif

	plSetNamedShaderUniformInt( NULL, "numLights", 1 );

	float brightness = 2.0f; //( sinf( Engine_GetNumTicks() / 100.0f ) * 4.0f ) / 1.0f;
	if ( brightness < 0.0f ) {
		brightness = 0.0f;
	}

	float radius = ( sinf( Engine_GetNumTicks() / 100.0f ) * 4.0f ) / 1.0f;
	if ( radius < 0.0f ) {
		radius = 0.0f;
	}

	plSetNamedShaderUniformVector4( NULL, "lights[0].colour", PLVector4( 1.0f, 1.0f, 1.0f, brightness ) );
	PLVector3 lightPosition = {
			-440, //+ sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			64, //+ cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f, //+ sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			-440, //- sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f
	};
	plSetNamedShaderUniformVector3( NULL, "lights[0].position", lightPosition );
	plSetNamedShaderUniformFloat( NULL, "lights[0].radius", radius );

	plSetNamedShaderUniformVector4( NULL, "lights[1].colour", PLVector4( 1.0f, 0.0f, 0.0f, 1.0f ) );
	lightPosition = PLVector3(
			-440 + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			64,//128 + sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f, //+ sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			-440 - -sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f
	);
	plSetNamedShaderUniformVector3( NULL, "lights[1].position", lightPosition );

	plSetNamedShaderUniformVector4( NULL, "lights[2].colour", PLVector4( 0.0f, 1.0f, 0.0f, 1.0f ) );
	lightPosition = PLVector3(
			-440 - sinf( Engine_GetNumTicks() / 32.0f ) * 100.0f,
			64, //+ cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f, //+ sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			-440 - -sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f
	);
	plSetNamedShaderUniformVector3( NULL, "lights[2].position", lightPosition );

	plSetNamedShaderUniformVector4( NULL, "lights[3].colour", PLVector4( 0.0f, 0.0f, 1.0f, 1.0f ) );
	lightPosition = PLVector3(
			-440 + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			64,//128 + sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f, //+ sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f,
			-440 - -sinf( Engine_GetNumTicks() / 64.0f ) * 100.0f + cosf( Engine_GetNumTicks() / 64.0f ) * 100.0f
	);
	plSetNamedShaderUniformVector3( NULL, "lights[3].position", lightPosition );

	Map_DrawSector( camera, &mapData.sectors[ 0 ] );

	plPopMatrix();

	Gfx_EnableShaderProgram( SHADER_GENERIC );

#if 0
	plMatrixMode( PL_MODELVIEW_MATRIX );
	plLoadIdentityMatrix();

	plDrawMeshNormals( transform, renderMesh );
#endif

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
