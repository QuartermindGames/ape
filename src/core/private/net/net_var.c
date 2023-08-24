// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "net.h"

typedef enum NetVariableType {
	NET_VARIABLE_TYPE_INT8,
	NET_VARIABLE_TYPE_INT16,
	NET_VARIABLE_TYPE_INT32,
	NET_VARIABLE_TYPE_INT64,

	NET_VARIABLE_TYPE_UINT8,
	NET_VARIABLE_TYPE_UINT16,
	NET_VARIABLE_TYPE_UINT32,
	NET_VARIABLE_TYPE_UINT64,

	NET_VARIABLE_TYPE_FLOAT32,
	NET_VARIABLE_TYPE_FLOAT64,

	NET_MAX_VARIABLE_TYPES
} NetVariableType;

typedef struct NetVariable {
	const void *originPointer;
	NetVariableType type;
	union {
		int8_t varInt8;
		int16_t varInt16;
		int32_t varInt32;
		int64_t varInt64;

		float varFloat32;
		double varFloat64;
	} baseline;
} NetVariable;

bool Net_Variable_IsDirty( const NetVariable *variable ) {
	switch ( variable->type ) {
		default:
			PRINT_ERROR( "Unhandled networked variable type!\n" );
		case NET_VARIABLE_TYPE_INT8:
		case NET_VARIABLE_TYPE_UINT8:
			return ( ( *( int8_t * ) variable->originPointer ) != variable->baseline.varInt8 );
		case NET_VARIABLE_TYPE_INT16:
		case NET_VARIABLE_TYPE_UINT16:
			return ( ( *( int16_t * ) variable->originPointer ) != variable->baseline.varInt16 );
		case NET_VARIABLE_TYPE_INT32:
		case NET_VARIABLE_TYPE_UINT32:
			return ( ( *( int32_t * ) variable->originPointer ) != variable->baseline.varInt32 );
		case NET_VARIABLE_TYPE_FLOAT32:
			return ( ( *( float * ) variable->originPointer ) != variable->baseline.varFloat32 );
	}
}
