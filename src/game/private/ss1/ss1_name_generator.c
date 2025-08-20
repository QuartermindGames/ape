// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "qmos/public/qm_os_random.h"

#include "ss1_game.h"

const char *ss1_name_generator_generate( char *buffer, size_t size )
{
	static const char *segments[] = {
	        "aa", "al", "el", "la",
	        "fa", "mo", "re", "ka",
	        "ca", "ma", "fe", "me",
	        "ra", "ke", "ce", "ee",
	        "he", "fo", "ru", "ku",
	        "cu", "eu", "hu", "fu" };

	unsigned int seed    = qm_os_random_seed_initialize();
	unsigned int maxSize = qm_os_random_int( &seed ) % size - 1;
	if ( maxSize < 4 )
	{
		maxSize = 4;
	}

	char *p = buffer;
	for ( size_t i = 0; i < maxSize; i += 2 )
	{
		unsigned int s = qm_os_random_int( &seed ) % PL_MAX_ARRAY_INDEX( segments );
		*p++           = segments[ s ][ 0 ];
		*p++           = segments[ s ][ 1 ];
	}

	// Ensure the first character is uppercase and null termination.
	buffer[ 0 ]       = ( char ) toupper( buffer[ 0 ] );
	buffer[ maxSize ] = '\0';
	return buffer;
}
