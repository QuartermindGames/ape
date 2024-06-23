// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

bool ape_server_send( ApeServerClientHandle *clientHandle, const void **buf, size_t *bufSizes, unsigned int numBuffers );

PL_EXTERN_C_END
