/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "parser.h"

const char *EZP_SkipSpaces( const char *buffer ) {
	while( *buffer == ' ' ) {
		buffer++;
	}

	return buffer;
}

const char *EZP_SkipLine( const char *buffer ) {
	while( *buffer != '\0' && *buffer != '\n' ) {
		buffer++;
	}

	return ( *buffer == '\n' ) ? ++buffer : buffer;
}

/**
 * Returns NULL if string did not fit into destination.
 */
const char *EZP_ReadString( const char *buffer, char *destination, size_t length ) {
	bool isContained = false;
	if ( *buffer == '"' ) {
		isContained = true;
		buffer++;
	}

	unsigned int destPos = 0;
	while ( *buffer != '\0' ) {
		if ( ( *buffer == '\r' || *buffer == '\n' ) || ( isContained && *buffer == '"' ) || ( !isContained && *buffer == ' ' ) ) {
			buffer++;
			break;
		}

		destination[ destPos++ ] = *buffer;
		if ( destPos >= length ) {
			return NULL;
		}

		buffer++;
	}

	destination[ destPos ] = '\0';
	return EZP_SkipSpaces( buffer );
}
