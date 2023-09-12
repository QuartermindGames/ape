// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_parse.h>

#include "../cook.h"

#include "format_obj.h"

ObjModel *ObjModel_LoadFromFile( const char *path )
{
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
		ERROR( "Failed to open OBJ: %s\n", PlGetError() );

	// Copy it into a buffer we can parse
	size_t fileBufSize = PlGetFileSize( file );
	const char *fileBuf = PlGetFileData( file );
	char *txtBuf = PL_NEW_( char, fileBufSize + 1 );
	memcpy( txtBuf, fileBuf, fileBufSize );

	PlCloseFile( file );

	ObjModel *obj = PL_NEW( ObjModel );
	ObjSubObject *subObject = NULL;

	const char *c = txtBuf;
	while ( *c != '\0' )
	{
		// Comment
		if ( *c == '#' )
		{
			PlSkipLine( &c );
			continue;
		}
		// Object
		else if ( *c == 'o' && *( c + 1 ) == ' ' )
		{
			c += 2;
			subObject = &obj->subObjects[ obj->numSubObjects++ ];
			PlParseToken( &c, subObject->name, sizeof( subObject->name ) );
			PlSkipLine( &c );
			continue;
		}
		// Vertex position
		else if ( *c == 'v' && *( c + 1 ) == ' ' )
		{
			c += 2;
			char *end;
			PLVector3 *vertex = PL_NEW( PLVector3 );
			vertex->x = strtof( c, &end );
			vertex->y = strtof( end, &end );
			vertex->z = strtof( end, NULL );

			if ( subObject->vertices == NULL )
				subObject->vertices = PlCreateVectorArray( 1 );

			PlPushBackVectorArrayElement( subObject->vertices, vertex );
			PlSkipLine( &c );
		}
		// Vertex normal
		else if ( *c == 'v' && *( c + 1 ) == 'n' && *( c + 2 ) == ' ' )
		{
			c += 3;
			char *end;
			PLVector3 *normal = PL_NEW( PLVector3 );
			normal->x = strtof( c, &end );
			normal->y = strtof( end, &end );
			normal->z = strtof( end, NULL );

			if ( subObject->normals == NULL )
				subObject->normals = PlCreateVectorArray( 1 );

			PlPushBackVectorArrayElement( subObject->normals, normal );
			PlSkipLine( &c );
		}
		// Vertex texture coordinate
		else if ( *c == 'v' && *( c + 1 ) == 't' && *( c + 2 ) == ' ' )
		{
			c += 3;
			char *end;
			PLVector2 *uv = PL_NEW( PLVector2 );
			uv->x = strtof( c, &end );
			uv->y = strtof( end, NULL );

			if ( subObject->textureCoords == NULL )
				subObject->textureCoords = PlCreateVectorArray( 1 );

			PlPushBackVectorArrayElement( subObject->textureCoords, uv );
			PlSkipLine( &c );
		}
		// Face
		else if ( *c == 'f' && *( c + 1 ) == ' ' )
		{
			c += 2;

			// f <pos>/<uv>/<norm>

			if ( subObject->faces == NULL )
				subObject->faces = PlCreateVectorArray( 1 );

			ObjFace *face = PL_NEW( ObjFace );
			PlPushBackVectorArrayElement( subObject->faces, face );
			for ( ; face->numEdges < OBJ_MAX_EDGES; face->numEdges++ )
			{
				if ( PlIsEndOfLine( c ) )
					break;

				char *end;
				face->vertices[ face->numEdges ] = ( strtoul( c, &end, 10 ) - 1 );
				end++;
				face->textureCoords[ face->numEdges ] = ( strtoul( end, &end, 10 ) - 1 );
				end++;
				face->normals[ face->numEdges ] = ( strtoul( end, &end, 10 ) - 1 );
				c = end;
			}
		}
		// Material library
		else if ( strncmp( c, "mtllib ", 7 ) == 0 )
		{
			PlSkipLine( &c );
		}
		else
		{
			//printf( "Unhandled OBJ opcode (%c), ignoring!\n", c[ 0 ] );
			PlSkipLine( &c );
		}
	}

	PL_DELETE( txtBuf );

	return obj;
}

void ObjModel_Destroy( ObjModel *obj )
{
	for ( unsigned int i = 0; i < obj->numSubObjects; ++i )
	{
		if ( obj->subObjects[ i ].vertices != NULL )
		{
			PlDestroyVectorArrayElements( obj->subObjects[ i ].vertices, PlFree );
			PlDestroyVectorArray( obj->subObjects[ i ].vertices );
		}
		if ( obj->subObjects[ i ].normals != NULL )
		{
			PlDestroyVectorArrayElements( obj->subObjects[ i ].normals, PlFree );
			PlDestroyVectorArray( obj->subObjects[ i ].normals );
		}
		if ( obj->subObjects[ i ].textureCoords != NULL )
		{
			PlDestroyVectorArrayElements( obj->subObjects[ i ].textureCoords, PlFree );
			PlDestroyVectorArray( obj->subObjects[ i ].textureCoords );
		}
		if ( obj->subObjects[ i ].faces != NULL )
		{
			PlDestroyVectorArrayElements( obj->subObjects[ i ].faces, PlFree );
			PlDestroyVectorArray( obj->subObjects[ i ].faces );
		}
	}

	PL_DELETE( obj );
}
