/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <stdlib.h>
#include <stdbool.h>

#include "parser.h"

const char *P_SkipSpaces( const char *buffer ) {
	while ( *buffer == ' ' ) buffer++;

	return buffer;
}

const char *P_SkipLine( const char *buffer ) {
	while ( *buffer != '\0' && *buffer != '\n' ) {
		buffer++;
	}

	return ( *buffer == '\n' ) ? ++buffer : buffer;
}

/**
 * Returns NULL if string did not fit into destination.
 */
const char *P_ReadString( const char *buffer, char *destination, size_t length ) {
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
	return P_SkipSpaces( buffer );
}
