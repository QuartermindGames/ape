/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 *
 * Purpose: Deserialisation/Serialisation for common types.
 */

#include <plgraphics/plg.h>

#include "node_private.h"

PLMatrix4 *NL_DS_DeserializeMatrix4( NLNode *in, PLMatrix4 *out )
{
	PlClearMatrix4( out );

	if ( in->childType != NL_PROP_F32 )
		return NULL;

	NLNode *c = NL_GetFirstChild( in );
	for ( uint8_t i = 0; i < 16; ++i )
	{
		if ( c == NULL || ( NL_GetF32( c, &out->m[ i ] ) != NL_ERROR_SUCCESS ) )
			break;

		c = NL_GetNextChild( c );
	}

	return out;
}

float *NL_DS_DeserializeVector( NLNode *in, float *out, uint8_t numElements )
{
	u_assert( numElements != 0 && numElements < 4 );
	const char *elements[] = { "x", "y", "z", "w" };
	for ( uint8_t i = 0; i < numElements; ++i )
		out[ i ] = NL_GetF32ByName( in, elements[ i ], 0.0f );

	return out;
}

PLVector2 *   NL_DS_DeserializeVector2( NLNode *in, PLVector2 *out ) { return ( PLVector2 * ) NL_DS_DeserializeVector( in, ( float * ) out, 2 ); }
PLVector3 *   NL_DS_DeserializeVector3( NLNode *in, PLVector3 *out ) { return ( PLVector3 * ) NL_DS_DeserializeVector( in, ( float * ) out, 3 ); }
PLVector4 *   NL_DS_DeserializeVector4( NLNode *in, PLVector4 *out ) { return ( PLVector4 * ) NL_DS_DeserializeVector( in, ( float * ) out, 4 ); }
PLQuaternion *NL_DS_DeserializeQuaternion( NLNode *in, PLQuaternion *out ) { return ( PLQuaternion * ) NL_DS_DeserializeVector( in, ( float * ) out, 4 ); }

PLColour *NL_DS_DeserializeColour( NLNode *in, PLColour *out )
{
	const char *elements[] = { "r", "g", "b", "a" };
	for ( uint8_t i = 0; i < 4; ++i )
		PlColourIndex( out, i ) = NL_GetI32ByName( in, elements[ i ], 255 );

	return out;
}

PLGVertex *NL_DS_DeserializeVertex( NLNode *in, PLGVertex *out )
{
	NLNode *n;
	if ( ( n = NL_GetChildByName( in, "position" ) ) != NULL )
		NL_DS_DeserializeVector3( n, &out->position );
	if ( ( n = NL_GetChildByName( in, "colour" ) ) != NULL )
		NL_DS_DeserializeColour( n, &out->colour );
	if ( ( n = NL_GetChildByName( in, "normal" ) ) != NULL )
		NL_DS_DeserializeVector3( n, &out->normal );
	if ( ( n = NL_GetChildByName( in, "tangent" ) ) != NULL )
		NL_DS_DeserializeVector3( n, &out->tangent );
	if ( ( n = NL_GetChildByName( in, "bitangent" ) ) != NULL )
		NL_DS_DeserializeVector3( n, &out->bitangent );
	if ( ( n = NL_GetChildByName( in, "uv" ) ) != NULL )
		NL_DS_DeserializeVector2( n, &out->st[ 0 ] );

	return out;
}
