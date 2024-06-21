// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#define APE_PROTOCOL_MAGIC   PL_MAGIC_TO_NUM( 'A', 'P', 'E', '0' )
#define APE_PROTOCOL_VERSION 1

#define APE_PROTOCOL_MESSAGE_SIZE 4096

PL_EXTERN_C

typedef enum ApeProtocolMessageType
{
	APE_PROTOCOL_MESSAGE_TYPE_VALIDATION,// validation request
	APE_PROTOCOL_MESSAGE_TYPE_VALIDATED, // successful validation message
	APE_PROTOCOL_MESSAGE_TYPE_GAME,      // game-specific message
} ApeProtocolMessageType;

typedef struct __attribute__( ( packed ) ) ApeProtocolMessageHeader
{
	uint16_t length;
	uint16_t type;
} ApeProtocolMessageHeader;

typedef struct ApeProtocolMessage
{
	char receiveBuffer[ APE_PROTOCOL_MESSAGE_SIZE ];
	size_t receivedBytes;
} ApeProtocolMessage;

static inline const void *ape_protocol_validate_message( const ApeProtocolMessage *src, const char *name, size_t dstSize )
{
	if ( src->receivedBytes != dstSize )
	{
		ape_warning_( "Invalid message size for \"%s\" (%u != %u)!\n", src->receivedBytes, dstSize );
		return nullptr;
	}

	return &src->receiveBuffer[ 0 ];
}

#define APE_PROTOCOL_VALIDATE_MESSAGE( SRC, DST ) \
	ape_protocol_validate_message( SRC, #DST, sizeof( DST ) )

typedef struct __attribute__( ( packed ) ) ApeProtocolValidationMessage
{
	ApeProtocolMessageHeader header;
	uint32_t magic;
	uint16_t version;
	char identifier[ 8 ];
} ApeProtocolValidationMessage;

#define APE_PROTOCOL_IMPLEMENT_WRITE_FUNCTION( NAME, TYPE )          \
	static inline TYPE NAME( const void **buf )                      \
	{                                                                \
		TYPE value;                                                  \
		memcpy( &value, *buf, sizeof( typeof( value ) ) );           \
		*buf = ( void * ) ( ( char * ) ( *buf ) + sizeof( value ) ); \
		return value;                                                \
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

PL_EXTERN_C_END
