/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "map.h"
#include "renderer/renderer.h"

#define MAP_GEOMETRY_IDENTIFIER "geometry"
#define MAP_GEOMETRY_VERSION    "version 2"

static struct
{
	PLGVertex *  vertices;
	unsigned int numVertices;
	MapFace *    faces;
	unsigned int numFaces;
	MapSector *  sectors;
	unsigned int numSectors;

	/* materials */
	Material **  materials;
	unsigned int numMaterials;
} mapData;

static PLGMesh *renderMesh = NULL;

/**
 * Returns a list of faces for the specified sector.
 */
MapFace *Map_GetFacesForSector( unsigned int sectorNum, unsigned int *numFaces )
{
	/* for now, just return the entire list -_-; */
	*numFaces = mapData.numFaces;
	return mapData.faces;
}

/**
 * Clears out the current map data.
 */
void Map_ClearData( void )
{
	memset( &mapData, 0, sizeof( mapData ) );
}

/**********************************************************/
/** Map File Reader **/

static bool Map_ValidateGeometryFile( PLFile *file )
{
	char buffer[ 256 ];

	/* check the identifier */
	PlReadString( file, buffer, sizeof( buffer ) );
	if ( strcmp( MAP_GEOMETRY_IDENTIFIER "\n", buffer ) != 0 )
	{
		PrintWarn( "Invalid identifier found, expected \"%s\" but found \"%s\"\n", MAP_GEOMETRY_IDENTIFIER, buffer );
		return false;
	}

	/* and now check the version */
	PlReadString( file, buffer, sizeof( buffer ) );
	if ( strcmp( MAP_GEOMETRY_VERSION "\n", buffer ) != 0 )
	{
		PrintWarn( "Invalid version found, expected \"%s\" but found \"%s\"\n", MAP_GEOMETRY_VERSION, buffer );
		return false;
	}

	return true;
}

static void Map_ParseTextures( PLFile *file )
{
	char buffer[ 256 ];

	/* fetch all the materials the map will be using */
	PlReadString( file, buffer, sizeof( buffer ) );
	if ( sscanf( buffer, "materials %d\n", &mapData.numMaterials ) != 1 )
	{
		PrintError( "Failed to fetch number of textures in \"%s\"!\n", PlGetFilePath( file ) );
	}

	/* allocate storage for them */
	mapData.materials = globalSystem.CAlloc( mapData.numMaterials, sizeof( Material * ), true );

	for ( unsigned int i = 0; i < mapData.numMaterials; ++i )
	{
		if ( PlReadString( file, buffer, sizeof( buffer ) ) == NULL )
		{
			PrintError( "Failed to read in texture \"%d\" in \"%s\"!\n", i, PlGetFilePath( file ) );
		}

		/* copy the material name; excluding the line terminator */
		char materialName[ 64 ];
		for ( unsigned int j = 0; j < sizeof( materialName ); ++j )
		{
			if ( buffer[ j ] == '\n' )
			{
				materialName[ j ] = '\0';
				break;
			}

			materialName[ j ] = buffer[ j ];
		}

		mapData.materials[ i ] = RM_CacheMaterial( materialName, CACHE_GROUP_WORLD, true );
	}
}

static void Map_ParseVertices( PLFile *file )
{
	char buffer[ 256 ];

	PlReadString( file, buffer, sizeof( buffer ) );
	if ( sscanf( buffer, "vertices %d\n", &mapData.numVertices ) != 1 )
	{
		PrintError( "Failed to fetch number of vertices in \"%s\"!\n", PlGetFilePath( file ) );
	}

	/* allocate buffer for vertices and then read them all in */
	mapData.vertices = globalSystem.CAlloc( mapData.numVertices, sizeof( PLGVertex ), true );
	for ( unsigned int i = 0; i < mapData.numVertices; ++i )
	{
		if ( PlReadString( file, buffer, sizeof( buffer ) ) == NULL )
		{
			PrintError( "Failed to read in vertex \"%d\" in \"%s\"!\n", i, PlGetFilePath( file ) );
		}

		if ( sscanf( buffer, "%f %f %f %f %f %f %f %f %d %d %d %d",
		             &mapData.vertices[ i ].position.x,
		             &mapData.vertices[ i ].position.y,
		             &mapData.vertices[ i ].position.z,
		             &mapData.vertices[ i ].normal.x,
		             &mapData.vertices[ i ].normal.y,
		             &mapData.vertices[ i ].normal.z,
		             &mapData.vertices[ i ].st[ 0 ].x,
		             &mapData.vertices[ i ].st[ 0 ].y,
		             &mapData.vertices[ i ].colour.r,
		             &mapData.vertices[ i ].colour.g,
		             &mapData.vertices[ i ].colour.b,
		             &mapData.vertices[ i ].colour.a ) < 3 )
		{
			PrintError( "Failed to read in vertex \"%d\" element in \"%s\"!\n", i, PlGetFilePath( file ) );
		}
	}
}

static void Map_ParseFaces( PLFile *file )
{
	char buffer[ 256 ];

	PlReadString( file, buffer, sizeof( buffer ) );
	if ( sscanf( buffer, "faces %d\n", &mapData.numFaces ) != 1 )
	{
		PrintError( "Failed to fetch number of faces in \"%s\"!\n", PlGetFilePath( file ) );
	}

	/* allocate buffer for faces and then read them all in */
	mapData.faces = globalSystem.CAlloc( mapData.numFaces, sizeof( MapFace ), true );
	for ( unsigned int i = 0; i < mapData.numFaces; ++i )
	{
		if ( PlReadString( file, buffer, sizeof( buffer ) ) == NULL )
		{
			PrintError( "Failed to read in face \"%d\" in \"%s\"!\n", i, PlGetFilePath( file ) );
		}

#define MAX_FACE_INDICES 64
		char *       token      = strtok( buffer, " " );
		unsigned int numIndices = strtol( token, NULL, 10 );
		if ( numIndices == 0 || numIndices >= MAX_FACE_INDICES )
		{
			PrintError( "Invalid number of indices for face \"%d\" in \"%s\"!\n", i, PlGetFilePath( file ) );
		}

		unsigned int faceIndices[ MAX_FACE_INDICES ];
		for ( unsigned int j = 0; j < numIndices; ++j )
		{
			token = strtok( NULL, " " );
			if ( token == NULL )
			{
				PrintError( "Failed to read in face \"%d\" elements in \"%s\"!\n", i, PlGetFilePath( file ) );
			}

			faceIndices[ j ] = strtol( token, NULL, 10 );
		}

		/* now try to get the texture index and flags (these are optional, for now) */

		unsigned int materialIndex = 0;

		token = strtok( NULL, " " );
		if ( token != NULL )
		{
			materialIndex = strtol( token, NULL, 10 );

			token = strtok( NULL, " " );
			if ( token != NULL )
			{
				mapData.faces[ i ].flags = strtol( token, NULL, 10 );
			}
		}

		if ( materialIndex >= mapData.numMaterials )
		{
			PrintError( "Invalid texture id %d for polygon %d!\n", materialIndex, i );
		}
		else
		{
			mapData.faces[ i ].material = mapData.materials[ materialIndex ];
		}

		mapData.faces[ i ].polygon = PlgCreatePolygon( NULL, PLVector2( 0.0f, 0.0f ), PLVector2( 2.0f, 2.0f ), 0.0f );
		if ( mapData.faces[ i ].polygon == NULL )
		{
			PrintError( "Failed to create polygon for face \"%d\" in \"%s\"!\n", i, PlGetFilePath( file ) );
		}

		for ( unsigned int j = 0; j < numIndices; ++j )
		{
			if ( faceIndices[ j ] >= mapData.numVertices )
			{
				PrintError( "Invalid vertex index!\n" );
			}

			PlgAddPolygonVertex( mapData.faces[ i ].polygon, &mapData.vertices[ faceIndices[ j ] ] );
		}

		unsigned int numPolyVertices;
		PLGVertex *  vertices = PlgGetPolygonVertices( mapData.faces[ i ].polygon, &numPolyVertices );
		if ( numPolyVertices != numIndices )
		{
			PrintError( "Number of vertices from polygon did not match vertices loaded!\n" );
		}

		/* generate tangets */
		unsigned int  numTriangles;
		unsigned int *indices = PlgConvertPolygonToTriangles( mapData.faces[ i ].polygon, &numTriangles );
		PlgGenerateTangentBasis( vertices, numPolyVertices, indices, numTriangles );
		free( indices );

		/* generate the bounds for cheap culling */
		mapData.faces[ i ].bounds = PlgGenerateAabbFromVertices( vertices, numPolyVertices, true );
	}
}

void Map_ParseSectors( PLFile *file )
{
	char buffer[ 256 ];

	//PlReadString( file, buffer, sizeof( buffer ) );
	//if ( sscanf( buffer, "sectors %d\n", &mapData.numSectors ) != 1 ) {
	//	PrintError( "Failed to fetch number of sectors in \"%s\"!\n", PlGetFilePath( file ) );
	//}

	mapData.numSectors = 1;
	mapData.sectors    = globalSystem.CAlloc( mapData.numSectors, sizeof( MapSector ), true );

#if 0
	/* all this for now is just dummy code. everything is treated as one sector */

	mapData.numSectors = 1; //PlReadInt32( filePtr, false, &status );
	mapData.sectors = Sys_AllocateMemory( mapData.numSectors, sizeof( MapSector ) );
	for ( unsigned int i = 0; i < mapData.numSectors; ++i ) {
		MapSector *area = &mapData.sectors[ i ];

		/* generate a list of all our line indices */
		mapData.sectors[ i ].numLines = mapData.numPolygons;
		area->lineIndices = Sys_AllocateMemory( mapData.sectors[ i ].numLines, sizeof( unsigned int ) );

		area->max[ 0 ] = area->max[ 1 ] = INT32_MIN;
		area->min[ 0 ] = area->min[ 1 ] = INT32_MAX;

		for ( unsigned int j = 0; j < mapData.sectors[ i ].numLines; ++j ) {
			mapData.sectors[ i ].lineIndices[ j ] = j; //PlReadInt16( filePtr, false, &status );
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

void Map_LoadGeometryData( const char *mapName )
{
	char path[ PL_SYSTEM_MAX_PATH ];
	snprintf( path, sizeof( path ), "worlds/%s.geometry", mapName );

	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		PrintWarn( "Failed to open \"geometry\" file, \"%s\"!\nPL: %s\n", path, PlGetError() );
		return;
	}

	if ( !Map_ValidateGeometryFile( file ) )
	{
		PrintWarn( "\"%s\" is not a valid \"geometry\" file!\n", PlGetFilePath( file ) );
	}
	else
	{
		Map_ParseTextures( file );
		Map_ParseVertices( file );
		Map_ParseFaces( file );
		Map_ParseSectors( file );
	}

	PlCloseFile( file );
}

void Map_Load( const char *mapName )
{
	Map_LoadGeometryData( mapName );

	/* create our mesh container for rendering */

	unsigned int maxTriangles = 0;
	for ( unsigned int i = 0; i < mapData.numFaces; ++i )
	{
		maxTriangles += PlgGetNumOfPolygonTriangles( mapData.faces[ i ].polygon );
	}

	/* destroy it if it already exists */
	PlgDestroyMesh( renderMesh );

	renderMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, maxTriangles, maxTriangles * 3 );
	if ( renderMesh == NULL )
	{
		PrintError( "Failed to create render mesh!\nPL: %s\n", PlGetError() );
	}
}

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

PLMatrix4 Map_GetPortalView( GfxCamera *camera, MapFace *source, MapFace *destination )
{
	/* so, this is a little awkward, but we need to figure out where each face is in the world
	 * by averaging the vertex coords for each and then positioning the portal at each. */

	typedef struct Portal
	{
		PLVector3 origin;
		PLVector3 rotation;
	} Portal;
	Portal portals[ 2 ];

	/* first, figure it out for the source */
	unsigned int numVertices;
	PLGVertex *  vertices;

	vertices            = PlgGetPolygonVertices( source->polygon, &numVertices );
	portals[ 0 ].origin = GetOriginPointFromVertices( vertices, numVertices );

	vertices            = PlgGetPolygonVertices( destination->polygon, &numVertices );
	portals[ 1 ].origin = GetOriginPointFromVertices( vertices, numVertices );

	Gfx_DrawAxesPivot( portals[ 0 ].origin, portals[ 1 ].rotation );
	Gfx_DrawAxesPivot( portals[ 1 ].origin, portals[ 1 ].rotation );

#if 0
	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();

	plRotateMatrix( 180.0f, 0.0f, 1.0f, 0.0f );

	plPopMatrix();

	return *PlGetMatrix( PL_MODELVIEW_MATRIX );
#endif
}

/**
 * Draw scrolling clouds.
 */
void Map_DrawSky( PLGCamera *camera )
{
	static Material *skyMaterial = NULL;
	if ( skyMaterial == NULL )
	{
		skyMaterial = RM_CacheMaterial( "materials/sky/cloudlayer00.mat", CACHE_GROUP_WORLD, true );
		if ( skyMaterial == NULL )
		{
			PrintError( "Failed to load cloud layer!\n" );
		}
	}

	static PLGVertex vertices[] = {
	        { .position = PLVector3( 10.0f, 100.f, 100.0f ),
	          .colour   = PL_COLOUR_WHITE }, /* top right */
	        { .position = PLVector3( 10.0f, 200.0f, 200.0f ),
	          .colour   = PLColourA( 0 ) }, /* top right far */
	        { .position = PLVector3( 10.0f, 100.0f, -100.0f ),
	          .colour   = PL_COLOUR_WHITE }, /* lower right */
	        { .position = PLVector3( 10.0f, 200.0f, -200.0f ),
	          .colour   = PLColourA( 0 ) }, /* lower right far */
	        { .position = PLVector3( 10.0f, -100.0f, -100.0f ),
	          .colour   = PL_COLOUR_WHITE }, /* lower left */
	        { .position = PLVector3( 10.0f, -200.0f, -200.0f ),
	          .colour   = PLColourA( 0 ) }, /* lower left far */
	        { .position = PLVector3( 10.0f, -100.0f, 100.0f ),
	          .colour   = PL_COLOUR_WHITE }, /* top left */
	        { .position = PLVector3( 10.0f, -200.0f, 200.0f ),
	          .colour   = PLColourA( 0 ) } }; /* top left far */
	static unsigned int indices[][ 3 ] = {
	        /* corners */
	        { 2, 1, 0 },
	        { 3, 1, 2 },
	        { 4, 3, 2 },
	        { 5, 3, 4 },
	        { 6, 5, 4 },
	        { 7, 5, 6 },
	        { 0, 7, 6 },
	        { 1, 7, 0 },
	        /* middle */
	        { 4, 2, 0 },
	        { 6, 4, 0 },
	};

	static PLGMesh *skyMesh = NULL;
	if ( skyMesh == NULL )
	{
		skyMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, plArrayElements( indices ), plArrayElements( vertices ) );
		if ( skyMesh == NULL )
		{
			PrintError( "Failed to create sky mesh!\nPL: %s\n", PlGetError() );
		}

		for ( unsigned int i = 0, curIndex = 0; i < plArrayElements( indices ); ++i )
		{
			PlgSetMeshTrianglePosition( skyMesh, &curIndex, indices[ i ][ 0 ], indices[ i ][ 1 ], indices[ i ][ 2 ] );
		}

		for ( unsigned int i = 0; i < plArrayElements( vertices ); ++i )
		{
			PlgSetMeshVertexPosition( skyMesh, i, PLVector3( vertices[ i ].position.y, vertices[ i ].position.x, vertices[ i ].position.z ) );
			PlgSetMeshVertexColour( skyMesh, i, vertices[ i ].colour );
		}
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );
	PlgSetDepthMask( false );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	PlTranslateMatrix( PLVector3( camera->position.x, camera->position.y + 10.0f, camera->position.z ) );

	/* todo: do this in shader... */
	PLVector2 skyOffset;
	skyOffset.x = Engine_GetNumTicks() / 1000.0f;
	skyOffset.y = Engine_GetNumTicks() / 1000.0f;
	PlgGenerateTextureCoordinates( skyMesh->vertices, skyMesh->num_verts, skyOffset, PLVector2( 0.75f, 0.75f ) );

	RM_DrawMesh( skyMaterial, skyMesh );

	/* todo: do this in shader... */
	skyOffset.x = ( Engine_GetNumTicks() / 100.0f ) * -1;
	skyOffset.y = Engine_GetNumTicks() / 100.0f;
	PlgGenerateTextureCoordinates( skyMesh->vertices, skyMesh->num_verts, skyOffset, PLVector2( 0.45f, 0.45f ) );

	RM_DrawMesh( skyMaterial, skyMesh );

	PlPopMatrix();

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
	PlgSetDepthMask( true );
}

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
			PLGVertex *  vertices = PlgGetPolygonVertices( curFace->polygon, &numVertices );
			for ( unsigned int k = 0; k < numVertices; ++k )
			{
				unsigned int v = PlgAddMeshVertex( renderMesh, vertices[ k ].position, vertices[ k ].normal, vertices[ k ].colour, vertices[ k ].st[ 0 ] );
				/* this shit is generated earlier in the process, and right now I'm not sure if it's appropriate to add to AddMeshVertex */
				renderMesh->vertices[ v ].tangent   = vertices[ k ].tangent;
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
			material = RM_CacheMaterial( "materials/engine/simple.mat", CACHE_GROUP_STATIC, true );
		}

		RM_DrawMesh( material, renderMesh );
	}
}

static void Map_SetupScene( PLGCamera *camera )
{
	PlgSetShaderProgram( defaultShaderPrograms[ GFX_SHADER_LIGHTING_PASS ] );

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

void Map_Draw( PLGCamera *camera, bool smPass )
{
	if ( renderMesh == NULL )
	{
		return;
	}

	CPUTimer_StartMeasure( PROFILE_DRAW_MAP );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	Map_SetupScene( camera );
	Map_DrawSky( camera );

	/* start drawing from the first sector that the camera is in */
	Map_DrawSector( camera, &mapData.sectors[ 0 ], smPass );

	PlPopMatrix();

	CPUTimer_EndMeasure( PROFILE_DRAW_MAP );
}

PLVector4 World_GetAmbience( void )
{
	return PLVector4( 0.4f, 0.4f, 0.4f, 1.0f );
}

PLVector4 World_GetSunColour( void )
{
	return PLVector4( 1.0f, 1.0f, 1.0f, 1.25f );
}

PLVector3 World_GetSunPosition( void )
{
	return PLVector3( 0.5f, -1.0f, 0.5f );
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
