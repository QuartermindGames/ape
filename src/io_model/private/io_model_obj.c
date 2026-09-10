// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <float.h>

#include "plcore/pl_filesystem.h"

#include "qmos/public/qm_os_memory.h"
#include "qmparse/public/qm_parse.h"

#include "io_model/public/io_model.h"
#include "io_model/public/io_model_obj.h"

static void parse_material_template_library( IOModelObj *obj, const char *path, IOModelResult *result )
{
	QmFsFile *file = qm_fs_file_open( path, true );
	if ( file == NULL )
	{
		IO_MODEL_RESULT( result, "failed to open OBJ material library", IO_MODEL_RESULT_CODE_IO_ERROR );
		return;
	}

	// Copy it into a buffer we can parse
	const size_t fileBufSize = qm_fs_file_get_size( file );
	const char  *fileBuf     = qm_fs_file_get_data( file );
	char        *txtBuf      = QM_OS_MEMORY_NEW_( char, fileBufSize + 1 );
	memcpy( txtBuf, fileBuf, fileBufSize );

	PlCloseFile( file );

	IOModelObjMaterial *material = NULL;

	const char *c = txtBuf;
	while ( *c != '\0' )
	{
		if ( *c == '#' )
		{
			qm_parse_skip_line( &c );
			continue;
		}

		char token[ 256 ];
		qm_parse_token( &c, token, sizeof( token ) );
		if ( strcmp( token, "newmtl" ) == 0 )
		{
			assert( obj->numMaterials < IO_MODEL_OBJ_MAX_MATERIALS );
			if ( obj->numMaterials >= IO_MODEL_OBJ_MAX_MATERIALS )
			{
				IO_MODEL_RESULT( result, "unexpected number of materials", IO_MODEL_RESULT_CODE_IO_ERROR );
				break;
			}

			material = &obj->materials[ obj->numMaterials++ ];
			qm_parse_enclosed( &c, material->name, sizeof( material->name ) );
		}
		else if ( strcmp( token, "map_Kd" ) == 0 )
		{
			if ( material == NULL )
			{
				IO_MODEL_RESULT( result, "invalid MTL file encountered", IO_MODEL_RESULT_CODE_IO_ERROR );
				break;
			}
			qm_parse_enclosed( &c, material->diffuseMap, sizeof( material->diffuseMap ) );
		}

		qm_parse_skip_line( &c );
	}

	qm_os_memory_free( txtBuf );
}

static void determine_sub_object_bounds( const IOModelObj *obj, IOModelObjSubObject *subObject, IOModelResult *result )
{
	static constexpr QmMathVector3f MAX_VECTOR = QM_MATH_VECTOR3F( FLT_MAX, FLT_MAX, FLT_MAX );
	static constexpr QmMathVector3f MIN_VECTOR = QM_MATH_VECTOR3F( FLT_MIN, FLT_MIN, FLT_MIN );

	subObject->mins = MAX_VECTOR;
	subObject->maxs = MIN_VECTOR;

	unsigned int     numFaces;
	IOModelObjFace **faces = ( IOModelObjFace ** ) PlGetVectorArrayDataEx( subObject->faces, &numFaces );
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		for ( unsigned int j = 0; j < faces[ i ]->numEdges; ++j )
		{
			IOModelObjVertex *vertex = PlGetVectorArrayElementAt( obj->vertices, faces[ i ]->indices[ j ][ IO_MODEL_OBJ_INDEX_VERTEX ] );
			if ( vertex == NULL )
			{
				IO_MODEL_RESULT( result, "attempted to retrieve an invalid vertex", IO_MODEL_RESULT_CODE_IO_ERROR );
				return;
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

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void obj_destroy( void *ptr )
{
	const IOModelObj *self = ptr;

	PlDestroyVectorArrayEx( self->vertices, qm_os_memory_free );
	PlDestroyVectorArrayEx( self->normals, qm_os_memory_free );
	PlDestroyVectorArrayEx( self->textureCoords, qm_os_memory_free );

	for ( unsigned int i = 0; i < self->numSubObjects; ++i )
	{
		PlDestroyVectorArrayEx( self->subObjects[ i ].faces, qm_os_memory_free );
	}
}

IOModelObj *io_model_obj_load( const char *path, IOModelResult *result )
{
	QmFsFile *file = qm_fs_file_open( path, true );
	if ( file == NULL )
	{
		IO_MODEL_RESULT( result, "failed to open file", IO_MODEL_RESULT_CODE_IO_ERROR );
		return nullptr;
	}

	// Copy it into a buffer we can parse
	const size_t fileBufSize = qm_fs_file_get_size( file );
	const char  *fileBuf     = qm_fs_file_get_data( file );
	char        *txtBuf      = QM_OS_MEMORY_NEW_( char, fileBufSize + 1 );
	memcpy( txtBuf, fileBuf, fileBufSize );

	PlCloseFile( file );

	IOModelObj          *obj            = QM_OS_MEMORY_NEW_D( IOModelObj, obj_destroy );
	IOModelObjSubObject *subObject      = nullptr;
	unsigned int         materialIndex  = 0;
	unsigned int         smoothingIndex = 0;

	const char *c = txtBuf;
	while ( *c != '\0' )
	{
		// Object
		if ( *c == 'o' && *( c + 1 ) == ' ' )
		{
			c += 2;
			subObject = &obj->subObjects[ obj->numSubObjects++ ];
			qm_parse_token( &c, subObject->name, sizeof( subObject->name ) );

			if ( subObject->faces == NULL )
			{
				subObject->faces = PlCreateVectorArray( 1 );
			}
		}
		// Vertex position
		else if ( *c == 'v' && *( c + 1 ) == ' ' )
		{
			c += 2;
			char             *end;
			IOModelObjVertex *vertex = QM_OS_MEMORY_NEW( IOModelObjVertex );
			vertex->position.x       = strtof( c, &end );
			vertex->position.y       = strtof( end, &end );
			vertex->position.z       = strtof( end, &end );

			if ( !qm_parse_is_end_of_line( end ) )
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
			char           *end;
			QmMathVector3f *normal = QM_OS_MEMORY_NEW( QmMathVector3f );
			normal->x              = strtof( c, &end );
			normal->y              = strtof( end, &end );
			normal->z              = strtof( end, NULL );

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
			char           *end;
			QmMathVector2f *uv = QM_OS_MEMORY_NEW( QmMathVector2f );
			uv->x              = strtof( c, &end );
			uv->y              = strtof( end, NULL );

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

			IOModelObjFace *face = QM_OS_MEMORY_NEW( IOModelObjFace );
			PlPushBackVectorArrayElement( subObject->faces, face );
			for ( ; face->numEdges < IO_MODEL_OBJ_MAX_EDGES; face->numEdges++ )
			{
				if ( qm_parse_is_end_of_line( c ) )
				{
					break;
				}

				char *end;
				face->indices[ face->numEdges ][ IO_MODEL_OBJ_INDEX_VERTEX ] = ( strtoul( c, &end, 10 ) - 1 );
				end++;
				face->indices[ face->numEdges ][ IO_MODEL_OBJ_INDEX_TEXTURE ] = ( strtoul( end, &end, 10 ) - 1 );
				end++;
				face->indices[ face->numEdges ][ IO_MODEL_OBJ_INDEX_NORMAL ] = ( strtoul( end, &end, 10 ) - 1 );
				c                                                            = end;
			}

#if 0// Life wasn't this simple, sadly

			// Calculate the normal of the face
			unsigned int numNormals;
			const QmMathVector3f **vn = ( const QmMathVector3f ** ) PlGetVectorArrayDataEx( obj->normals, &numNormals );
			for ( unsigned int i = 0; i < face->numEdges; ++i )
			{
				const QmMathVector3f *n = vn[ face->indices[ i ][ IO_MODEL_OBJ_INDEX_NORMAL ] ];
				face->normal = qm_math_vector3f_add( face->normal, *n );
			}
			face->normal = qm_math_vector3f_normalize( face->normal );

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
				unsigned int indices[ IO_MODEL_OBJ_MAX_EDGES * 3 ];
				QM_OS_ZERO_( indices );
				unsigned int *index = indices;
				for ( unsigned int i = 1; i + 1 < face->numEdges; ++i )
				{
					index[ 0 ] = 0;
					index[ 1 ] = i;
					index[ 2 ] = i + 1;
					index += 3;
				}

				unsigned int             numVertices;
				const IOModelObjVertex **v = ( const IOModelObjVertex ** ) PlGetVectorArrayDataEx( obj->vertices, &numVertices );

				QmMathVector3f normals[ IO_MODEL_OBJ_MAX_EDGES ];
				QM_OS_ZERO_( normals );
				for ( unsigned int i = 0, idx = 0; i < numTriangles; ++i, idx += 3 )
				{
					const unsigned int x = indices[ idx ];
					const unsigned int y = indices[ idx + 1 ];
					const unsigned int z = indices[ idx + 2 ];

					const QmMathVector3f n = qm_math_compute_triangle_normal( v[ face->indices[ x ][ IO_MODEL_OBJ_INDEX_VERTEX ] ]->position,
					                                                          v[ face->indices[ y ][ IO_MODEL_OBJ_INDEX_VERTEX ] ]->position,
					                                                          v[ face->indices[ z ][ IO_MODEL_OBJ_INDEX_VERTEX ] ]->position );

					normals[ x ] = qm_math_vector3f_add( normals[ x ], n );
					normals[ y ] = qm_math_vector3f_add( normals[ y ], n );
					normals[ z ] = qm_math_vector3f_add( normals[ z ], n );
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
			qm_parse_enclosed( &c, token, sizeof( token ) );

			PLPath libPath;
			PlSetupPath( libPath, true, "%s", path );
			char *s = strrchr( libPath, '/' ) + 1;
			*s      = '\0';
			PlAppendPath( libPath, token, true );

			parse_material_template_library( obj, libPath, result );
		}
		else if ( strncmp( c, "usemtl ", 7 ) == 0 )
		{
			c += 7;
			char token[ 128 ];
#if 1
			qm_parse_enclosed( &c, token, sizeof( token ) );
#else
			// well, the above would've been nice, but I hit a case where it's not enclosed despite having spaces...
			// maybe reading the whole line will be okay????
			qm_parse_line( &c, token, sizeof( token ) );
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

		qm_parse_skip_line( &c );
	}

	qm_os_memory_free( txtBuf );

	for ( unsigned int i = 0; i < obj->numSubObjects; ++i )
	{
		determine_sub_object_bounds( obj, &obj->subObjects[ i ], result );
	}

	return obj;
}
