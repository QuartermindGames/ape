// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#define APE_PROTOCOL_MAGIC   PL_MAGIC_TO_NUM( 'A', 'P', 'E', '0' )
#define APE_PROTOCOL_VERSION 1

#define APE_PROTOCOL_MESSAGE_SIZE 4096

static constexpr unsigned int APE_PROTOCOL_MAX_IDENTIFIER  = 8;
static constexpr unsigned int APE_PROTOCOL_MAX_CLIENT_NAME = 16;

static constexpr size_t MAX_PAYLOAD_ITEMS = 16;

typedef struct __attribute__( ( packed ) ) ApeProtocolPayloadBuffer
{
	const void  *items[ MAX_PAYLOAD_ITEMS ];
	unsigned int numItems;
	size_t       sizes[ MAX_PAYLOAD_ITEMS ];
} ApeProtocolPayloadBuffer;

static inline void ape_protocol_payload_push( ApeProtocolPayloadBuffer *buffer, const void *item, size_t size )
{
	assert( buffer->numItems < MAX_PAYLOAD_ITEMS );
	buffer->items[ buffer->numItems ] = item;
	buffer->sizes[ buffer->numItems ] = size;
	buffer->numItems++;
}

#define APE_PROTOCOL_IMPLEMENT_PARSE_FUNCTION( NAME, TYPE )          \
	static inline TYPE NAME( const void **buf )                      \
	{                                                                \
		TYPE value;                                                  \
		memcpy( &value, *buf, sizeof( typeof( value ) ) );           \
		*buf = ( void * ) ( ( char * ) ( *buf ) + sizeof( value ) ); \
		return value;                                                \
	}

APE_PROTOCOL_IMPLEMENT_PARSE_FUNCTION( ape_protocol_parse_int8, int8_t );
APE_PROTOCOL_IMPLEMENT_PARSE_FUNCTION( ape_protocol_parse_int16, int16_t );
APE_PROTOCOL_IMPLEMENT_PARSE_FUNCTION( ape_protocol_parse_int32, int32_t );
APE_PROTOCOL_IMPLEMENT_PARSE_FUNCTION( ape_protocol_parse_float, float );
APE_PROTOCOL_IMPLEMENT_PARSE_FUNCTION( ape_protocol_parse_double, double );
