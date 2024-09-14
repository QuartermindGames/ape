// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_parse.h>
#include <float.h>

#include "../cook.h"
#include "model.h"

#include "model_obj.h"
#include "plgraphics/plg_mesh.h"

static void parse_material_template_library( ObjModel *obj, const char *path )
{
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		ERROR( "Failed to open OBJ material library: %s\n", PlGetError() );
	}

	// Copy it into a buffer we can parse
	size_t      fileBufSize = PlGetFileSize( file );
	const char *fileBuf     = PlGetFileData( file );
	char       *txtBuf      = PL_NEW_( char, fileBufSize + 1 );
	memcpy( txtBuf, fileBuf, fileBufSize );

	PlCloseFile( file );

	ObjMaterial *material = NULL;

	const char *c = txtBuf;
	while ( *c != '\0' )
	{
		if ( *c == '#' )
		{
			PlSkipLine( &c );
			continue;
		}

		char token[ 256 ];
		PlParseToken( &c, token, sizeof( token ) );
		if ( strcmp( token, "newmtl" ) == 0 )
		{
			assert( obj->numMaterials < OBJ_MAX_MATERIALS );
			if ( obj->numMaterials >= OBJ_MAX_MATERIALS )
			{
				ERROR( "Unexpected number of materials (%u >= %u)!\n", obj->numMaterials, OBJ_MAX_MATERIALS );
			}

			material = &obj->materials[ obj->numMaterials++ ];
			PlParseEnclosedString( &c, material->name, sizeof( material->name ) );
		}
		else if ( strcmp( token, "map_Kd" ) == 0 )
		{
			if ( material == NULL )
			{
				ERROR( "Invalid MTL file encountered!\n" );
			}
			PlParseEnclosedString( &c, material->diffuseMap, sizeof( material->diffuseMap ) );
		}

		PlSkipLine( &c );
	}
}

static void determine_sub_object_bounds( ObjModel *obj, ObjSubObject *subObject )
{
	subObject->mins = ( PLVector3 ){ FLT_MAX, FLT_MAX, FLT_MAX };
	subObject->maxs = ( PLVector3 ){ FLT_MIN, FLT_MIN, FLT_MIN };

	unsigned int numFaces;
	ObjFace    **faces = ( ObjFace    **) PlGetVectorArrayDataEx( subObject->faces, &numFaces );
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		for ( unsigned int j = 0; j < faces[ i ]->numEdges; ++j )
		{
			ObjVertex *vertex = PlGetVectorArrayElementAt( obj->vertices, faces[ i ]->indices[ j ][ OBJ_INDEX_VERTEX ] );
			if ( vertex == NULL )
			{
				ERROR( "Attempted to retrieve an invalid vertex (%u): %s\n", j, PlGetError() );
			}

			if ( vertex->position.x < subObject->mins.x ) subObject->mins.x = vertex->position.x;
			if ( vertex->position.y < subObject->mins.y ) subObject->mins.y = vertex->position.y;
			if ( vertex->position.z < subObject->mins.z ) subObject->mins.z = vertex->position.z;
			if ( vertex->position.x > subObject->maxs.x ) subObject->maxs.x = vertex->position.x;
			if ( vertex->position.y > subObject->maxs.y ) subObject->maxs.y = vertex->position.y;
			if ( vertex->position.z > subObject->maxs.z ) subObject->maxs.z = vertex->position.z;
		}
	}
}

ObjModel *model_obj_load( const char *path )
{
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
		ERROR( "Failed to open OBJ: %s\n", PlGetError() );

	// Copy it into a buffer we can parse
	size_t      fileBufSize = PlGetFileSize( file );
	const char *fileBuf     = PlGetFileData( file );
	char       *txtBuf      = PL_NEW_( char, fileBufSize + 1 );
	memcpy( txtBuf, fileBuf, fileBufSize );

	PlCloseFile( file );

	ObjModel     *obj            = PL_NEW( ObjModel );
	ObjSubObject *subObject      = NULL;
	unsigned int  materialIndex  = 0;
	unsigned int  smoothingIndex = 0;

	const char *c = txtBuf;
	while ( *c != '\0' )
	{
		// Object
		if ( *c == 'o' && *( c + 1 ) == ' ' )
		{
			c += 2;
			subObject = &obj->subObjects[ obj->numSubObjects++ ];
			PlParseToken( &c, subObject->name, sizeof( subObject->name ) );

			if ( subObject->faces == NULL )
			{
				subObject->faces = PlCreateVectorArray( 1 );
			}
		}
		// Vertex position
		else if ( *c == 'v' && *( c + 1 ) == ' ' )
		{
			c += 2;
			char      *end;
			ObjVertex *vertex  = PL_NEW( ObjVertex );
			vertex->position.x = strtof( c, &end );
			vertex->position.y = strtof( end, &end );
			vertex->position.z = strtof( end, &end );

			if ( !PlIsEndOfLine( end ) )
			{
				obj->storesColour = true;
				vertex->colour.x  = strtof( end, &end );
				vertex->colour.y  = strtof( end, &end );
				vertex->colour.z  = strtof( end, NULL );
			}
#if 0// unsure about this for now...
			else
			{
				vertex->colour.x = 1.0f;
				vertex->colour.y = 1.0f;
				vertex->colour.z = 1.0f;
			}
#endif

			if ( obj->vertices == NULL )
			{
				obj->vertices = PlCreateVectorArray( 1 );
			}

			PlPushBackVectorArrayElement( obj->vertices, vertex );
		}
		// Vertex normal
		else if ( *c == 'v' && *( c + 1 ) == 'n' && *( c + 2 ) == ' ' )
		{
			c += 3;
			char      *end;
			PLVector3 *normal = PL_NEW( PLVector3 );
			normal->x         = strtof( c, &end );
			normal->y         = strtof( end, &end );
			normal->z         = strtof( end, NULL );

			if ( obj->normals == NULL )
			{
				obj->normals = PlCreateVectorArray( 1 );
			}

			PlPushBackVectorArrayElement( obj->normals, normal );
		}
		// Vertex texture coordinate
		else if ( *c == 'v' && *( c + 1 ) == 't' && *( c + 2 ) == ' ' )
		{
			c += 3;
			char      *end;
			PLVector2 *uv = PL_NEW( PLVector2 );
			uv->x         = strtof( c, &end );
			uv->y         = strtof( end, NULL );

			if ( obj->textureCoords == NULL )
			{
				obj->textureCoords = PlCreateVectorArray( 1 );
			}

			PlPushBackVectorArrayElement( obj->textureCoords, uv );
		}
		// Face
		else if ( *c == 'f' && *( c + 1 ) == ' ' )
		{
			c += 2;

			// f <pos>/<uv>/<norm>

			assert( subObject->faces != NULL );

			ObjFace *face = PL_NEW( ObjFace );
			PlPushBackVectorArrayElement( subObject->faces, face );
			for ( ; face->numEdges < OBJ_MAX_EDGES; face->numEdges++ )
			{
				if ( PlIsEndOfLine( c ) )
				{
					break;
				}

				char *end;
				face->indices[ face->numEdges ][ OBJ_INDEX_VERTEX ] = ( strtoul( c, &end, 10 ) - 1 );
				end++;
				face->indices[ face->numEdges ][ OBJ_INDEX_TEXTURE ] = ( strtoul( end, &end, 10 ) - 1 );
				end++;
				face->indices[ face->numEdges ][ OBJ_INDEX_NORMAL ] = ( strtoul( end, &end, 10 ) - 1 );
				c                                                   = end;
			}

#if 0// Life wasn't this simple, sadly

			// Calculate the normal of the face
			unsigned int numNormals;
			const PLVector3 **vn = ( const PLVector3 ** ) PlGetVectorArrayDataEx( obj->normals, &numNormals );
			for ( unsigned int i = 0; i < face->numEdges; ++i )
			{
				const PLVector3 *n = vn[ face->indices[ i ][ OBJ_INDEX_NORMAL ] ];
				face->normal = PlAddVector3( face->normal, *n );
			}
			face->normal = PlNormalizeVector3( face->normal );

#else

			unsigned int numTriangles;
			if ( face->numEdges < 3 )
			{
				numTriangles = 0;
			}
			else
			{
				numTriangles = face->numEdges - 2;
			}
			if ( numTriangles > 0 )
			{
				unsigned int indices[ OBJ_MAX_EDGES * 3 ];
				PL_ZERO_( indices );
				unsigned int *index = indices;
				for ( unsigned int i = 1; i + 1 < face->numEdges; ++i )
				{
					index[ 0 ] = 0;
					index[ 1 ] = i;
					index[ 2 ] = i + 1;
					index += 3;
				}

				unsigned int      numVertices;
				const ObjVertex **v = ( const ObjVertex ** ) PlGetVectorArrayDataEx( obj->vertices, &numVertices );

				PLVector3 normals[ OBJ_MAX_EDGES ];
				PL_ZERO_( normals );
				for ( unsigned int i = 0, idx = 0; i < numTriangles; ++i, idx += 3 )
				{
					unsigned int x = indices[ idx ];
					unsigned int y = indices[ idx + 1 ];
					unsigned int z = indices[ idx + 2 ];

					PLVector3 n = PlgGenerateVertexNormal( v[ face->indices[ x ][ OBJ_INDEX_VERTEX ] ]->position,
					                                       v[ face->indices[ y ][ OBJ_INDEX_VERTEX ] ]->position,
					                                       v[ face->indices[ z ][ OBJ_INDEX_VERTEX ] ]->position );

					normals[ x ] = PlAddVector3( normals[ x ], n );
					normals[ y ] = PlAddVector3( normals[ y ], n );
					normals[ z ] = PlAddVector3( normals[ z ], n );
				}

				face->normal = normals[ 0 ];
			}

#endif

			face->material       = materialIndex;
			face->smoothingGroup = smoothingIndex;
		}
		else if ( *c == 's' && *( c + 1 ) == ' ' )
		{
			c += 2;
			smoothingIndex = strtoul( c, NULL, 10 );
		}
		// Material library
		else if ( strncmp( c, "mtllib ", 7 ) == 0 )
		{
			c += 7;

			char token[ 128 ];
			PlParseEnclosedString( &c, token, sizeof( token ) );

			PLPath libPath;
			PlSetupPath( libPath, true, "%s", path );
			char *s = strrchr( libPath, '/' ) + 1;
			*s      = '\0';
			PlAppendPath( libPath, token, true );

			parse_material_template_library( obj, libPath );
		}
		else if ( strncmp( c, "usemtl ", 7 ) == 0 )
		{
			c += 7;
			char token[ 128 ];
#if 1
			PlParseEnclosedString( &c, token, sizeof( token ) );
#else
			// well, the above would've been nice, but I hit a case where it's not enclosed despite having spaces...
			// maybe reading the whole line will be okay????
			PlParseLine( &c, token, sizeof( token ) );
#endif
			for ( materialIndex = 0; materialIndex < obj->numMaterials; ++materialIndex )
			{
				if ( strcmp( token, obj->materials[ materialIndex ].name ) == 0 )
				{
					break;
				}
			}

			assert( materialIndex < obj->numMaterials );
		}
		// Unhandled lines we just skip for now...

		PlSkipLine( &c );
	}

	PL_DELETE( txtBuf );

	for ( unsigned int i = 0; i < obj->numSubObjects; ++i )
	{
		determine_sub_object_bounds( obj, &obj->subObjects[ i ] );
	}

	return obj;
}

void model_obj_destroy( ObjModel *obj )
{
	if ( obj == NULL )
	{
		return;
	}

	PlDestroyVectorArrayEx( obj->vertices, PlFree );
	PlDestroyVectorArrayEx( obj->normals, PlFree );
	PlDestroyVectorArrayEx( obj->textureCoords, PlFree );

	for ( unsigned int i = 0; i < obj->numSubObjects; ++i )
	{
		PlDestroyVectorArrayEx( obj->subObjects[ i ].faces, PlFree );
	}

	PL_DELETE( obj );
}

CookModel *model_obj_to_ape( const ObjModel *obj, CookModel *out )
{
	out->numMeshes = obj->numSubObjects;
	if ( out->numMeshes >= APE_FORMAT_MODEL_MAX_MATERIALS )
	{
		WARN( "Hit maximum mesh limit (%u >= %u)!\n", out->numMeshes, APE_FORMAT_MODEL_MAX_MATERIALS );
		out->numMeshes = ( APE_FORMAT_MODEL_MAX_MATERIALS - 1 );
	}

	for ( unsigned int i = 0; i < out->numMeshes; ++i )
	{
		CookModelMesh *mesh = &out->meshes[ i ];

		unsigned int numFaces;
		ObjFace    **faces = ( ObjFace    **) PlGetVectorArrayDataEx( obj->subObjects[ i ].faces, &numFaces );
		for ( unsigned int j = 0; j < numFaces; ++j )
		{
			// We'll need to convert it into triangles here...
			unsigned int numTriangles = faces[ j ]->numEdges < 3 ? 0 : ( faces[ j ]->numEdges - 3 );
		}
	}
}

static CookModel *load_obj( const char *path ) { return ( CookModel * ) model_obj_load( path ); }
static CookModel *conv_obj( const CookModel *model, CookModel *out ) { return model_obj_to_ape( ( const ObjModel * ) model, out ); }
static void       destroy_obj( CookModel *model ) { model_obj_destroy( ( ObjModel       *) model ); }

const CookModelFormatInterface modelObjInterface = { "obj", load_obj, conv_obj, destroy_obj };
