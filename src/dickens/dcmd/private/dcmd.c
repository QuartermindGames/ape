// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "dickens.h"

#include <plcore/pl_linkedlist.h>

#define MAX_COMMAND_LENGTH 256
static char cmdLine[ MAX_COMMAND_LENGTH ];

int main( int argc, char **argv ) {
	printf( "Dickens DCMD Utility\n"
	        "For Dickens Scripting Language v%d.%d.%d\n"
	        "Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>\n"
	        "-----------------------------------------------------------------\n",
	        DICKENS_VERSION_MAJOR,
	        DICKENS_VERSION_MINOR,
	        DICKENS_VERSION_PATCH );

	while ( true ) {
		printf( "> " );

		int i;
		char *p = cmdLine;
		while ( ( i = getchar() ) != '\n' ) {
			*p++ = ( char ) i;
			unsigned int numChars = p - cmdLine;
			if ( numChars >= MAX_COMMAND_LENGTH - 1 ) {
				printf( "Hit character limit!\n" );
				return EXIT_FAILURE;
			}
		}

		DKLexer *lexer = DKLexer_GenerateTokenList( NULL, cmdLine, "command" );
		if ( lexer != NULL )
			DKParser_ParseProgram( lexer );

		memset( cmdLine, 0, sizeof( cmdLine ) );
	}

	return EXIT_SUCCESS;
}
