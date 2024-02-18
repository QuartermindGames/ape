// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#define PROTOCOL_VERSION     1
#define PROTOCOL_MESSAGESIZE 4096

typedef struct ApeProtocolMessageHeader
{
	uint32_t length;
	uint32_t type;
} ApeProtocolMessageHeader;
