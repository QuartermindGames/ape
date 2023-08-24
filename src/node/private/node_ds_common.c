// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <assert.h>

#include <plcore/pl_physics.h>
#include <plgraphics/plg_mesh.h>

#include "node_private.h"

PLMatrix4 *NL_DS_DeserializeMatrix4( NdBranch *in, PLMatrix4 *out ) {
	if ( in == NULL ) {
		return NULL;
	}

	PlClearMatrix4( out );

	if ( in->childType != ND_PROPERTY_F32 )
		return NULL;

	NdBranch *c = ndGetFirstChild( in );
	for ( uint8_t i = 0; i < 16; ++i ) {
		if ( c == NULL || ( ndGetF32( c, &out->m[ i ] ) != ND_ERROR_SUCCESS ) )
			break;

		c = ndGetNextChild( c );
	}

	return out;
}

float *ndDS_DeserializeVector( NdBranch *in, float *out, uint8_t numElements ) {
	if ( in == NULL )
		return NULL;

	assert( numElements != 0 && numElements < 4 );
	const char *elements[] = { "x", "y", "z", "w" };
	for ( uint8_t i = 0; i < numElements; ++i )
		out[ i ] = ndGetF32ByName( in, elements[ i ], 0.0f );

	return out;
}

PLVector2 *ndDS_DeserializeVector2( NdBranch *in, PLVector2 *out ) { return ( PLVector2 * ) ndDS_DeserializeVector( in, ( float * ) out, 2 ); }
PLVector3 *ndDS_DeserializeVector3( NdBranch *in, PLVector3 *out ) { return ( PLVector3 * ) ndDS_DeserializeVector( in, ( float * ) out, 3 ); }

NdBranch *ndDS_SerializeColour( NdBranch *parent, const char *name, const PLColour *colour ) {
	NdBranch *object = ndPushBackObject( parent, name );
	ndPushBackI8( object, "r", colour->r );
	ndPushBackI8( object, "g", colour->g );
	ndPushBackI8( object, "b", colour->b );
	ndPushBackI8( object, "a", colour->a );
	return object;
}

PLColour *ndDS_DeserializeColour( NdBranch *in, PLColour *out ) {
	if ( in == NULL )
		return NULL;

	out->r = ND_GETUINT8( in, "r", 255 );
	out->g = ND_GETUINT8( in, "g", 255 );
	out->b = ND_GETUINT8( in, "b", 255 );
	out->a = ND_GETUINT8( in, "a", 255 );
	return out;
}

NdBranch *NL_DS_SerializeColourF32( NdBranch *parent, const char *name, const PLColourF32 *colour ) {
	NdBranch *object = ndPushBackObject( parent, name );
	ndPushBackF32( object, "r", colour->r );
	ndPushBackF32( object, "g", colour->g );
	ndPushBackF32( object, "b", colour->b );
	ndPushBackF32( object, "a", colour->a );
	return object;
}

PLColourF32 *ndDS_DeserializeColourF32( NdBranch *in, PLColourF32 *out ) {
	if ( in == NULL )
		return NULL;

	out->r = ndGetF32ByName( in, "r", 1.0f );
	out->g = ndGetF32ByName( in, "g", 1.0f );
	out->b = ndGetF32ByName( in, "b", 1.0f );
	out->a = ndGetF32ByName( in, "a", 1.0f );
	return out;
}

/****************************************
 ****************************************/

PLGVertex *NL_DS_DeserializeVertex( NdBranch *in, PLGVertex *out ) {
	if ( in == NULL )
		return NULL;

	ndDS_DeserializeVector3( ndGetChildByName( in, "position" ), &out->position );
	ndDS_DeserializeColour( ndGetChildByName( in, "colour" ), &out->colour );
	ndDS_DeserializeVector3( ndGetChildByName( in, "normal" ), &out->normal );
	ndDS_DeserializeVector3( ndGetChildByName( in, "tangent" ), &out->tangent );
	ndDS_DeserializeVector3( ndGetChildByName( in, "bitangent" ), &out->bitangent );
	ndDS_DeserializeVector2( ndGetChildByName( in, "uv" ), &out->st[ 0 ] );
	return out;
}

/****************************************
 ****************************************/

PLCollisionAABB *NL_DS_DeserializeCollisionAABB( NdBranch *in, PLCollisionAABB *out ) {
	if ( in == NULL )
		return NULL;

	ndDS_DeserializeVector3( ndGetChildByName( in, "mins" ), &out->mins );
	ndDS_DeserializeVector3( ndGetChildByName( in, "maxs" ), &out->maxs );
	ndDS_DeserializeVector3( ndGetChildByName( in, "origin" ), &out->origin );
	ndDS_DeserializeVector3( ndGetChildByName( in, "absOrigin" ), &out->absOrigin );
	return out;
}

/****************************************
 * SERIALISATION
 ****************************************/

NdBranch *ndDS_SerializeVector2( NdBranch *parent, const char *name, const PLVector2 *vector2 ) {
	NdBranch *object = ndPushBackObject( parent, name );
	ndPushBackF32( object, "x", vector2->x );
	ndPushBackF32( object, "y", vector2->y );
	return object;
}

NdBranch *ndDS_SerializeVector3( NdBranch *parent, const char *name, const PLVector3 *vector3 ) {
	NdBranch *object = ndPushBackObject( parent, name );
	ndPushBackF32( object, "x", vector3->x );
	ndPushBackF32( object, "y", vector3->y );
	ndPushBackF32( object, "z", vector3->z );
	return object;
}
