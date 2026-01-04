// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape/ape_public_protocol.h"

PL_EXTERN_C

static constexpr unsigned int APE_PROTOCOL_MAX_CONNECTION_RETRIES = 8;

static constexpr double APE_PROTOCOL_HEARTBEAT_TIME = 3.0; // seconds
static constexpr double APE_PROTOCOL_TIMEOUT        = 30.0;//seconds

typedef enum ApeProtocolMessageType
{
	APE_PROTOCOL_MESSAGE_TYPE_VALIDATION,// validation request
	APE_PROTOCOL_MESSAGE_TYPE_VALIDATED, // successful validation message
	APE_PROTOCOL_MESSAGE_TYPE_HEARTBEAT_REQUEST,
	APE_PROTOCOL_MESSAGE_TYPE_HEARTBEAT_RESPONSE,
	APE_PROTOCOL_MESSAGE_TYPE_GAME,// game-specific message
} ApeProtocolMessageType;

typedef struct __attribute__( ( packed ) ) ApeProtocolMessageHeader
{
	uint16_t length;
	uint16_t type;
} ApeProtocolMessageHeader;

typedef struct ApeProtocolMessage
{
	char   receiveBuffer[ APE_PROTOCOL_MESSAGE_SIZE ];
	size_t receivedBytes;
} ApeProtocolMessage;

static inline const void *ape_protocol_validate_message( const ApeProtocolMessage *src, const char *name, size_t dstSize )
{
	if ( src->receivedBytes != dstSize )
	{
		ape_console_warning_( "Invalid message size for \"%s\" (%u != %u)!\n", src->receivedBytes, dstSize );
		return nullptr;
	}

	return &src->receiveBuffer[ 0 ];
}

#define APE_PROTOCOL_VALIDATE_MESSAGE( SRC, DST ) \
	ape_protocol_validate_message( SRC, #DST, sizeof( DST ) )

typedef struct __attribute__( ( packed ) ) ApeProtocolValidationMessage
{
	ApeProtocolMessageHeader header;
	uint32_t                 magic;
	uint16_t                 version;
	char                     identifier[ APE_PROTOCOL_MAX_IDENTIFIER ];
	char                     clientName[ APE_PROTOCOL_MAX_CLIENT_NAME ];
} ApeProtocolValidationMessage;

PL_EXTERN_C_END
