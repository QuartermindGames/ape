// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "dickens.h"

#define MAX_COMMAND_LENGTH 256
static char cmdLine[ MAX_COMMAND_LENGTH ];

int main( int argc, char **argv )
{
	printf( "Dickens DCMD Utility\n"
	        "For Dickens Scripting Language v%d.%d.%d\n"
	        "Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>\n"
	        "-----------------------------------------------------------------\n",
	        DICKENS_VERSION_MAJOR,
	        DICKENS_VERSION_MINOR,
	        DICKENS_VERSION_PATCH );

	while ( true )
	{
		printf( "> " );

		int i;
		char *p = cmdLine;
		while ( ( i = getchar() ) != '\n' )
		{
			*p++ = ( char ) i;
			unsigned int numChars = p - cmdLine;
			if ( numChars >= MAX_COMMAND_LENGTH - 1 )
			{
				printf( "Hit character limit!\n" );
				return EXIT_FAILURE;
			}
		}

		DkLexer *lexer = dk_generate_token_list( NULL, cmdLine, "command" );
		if ( lexer != NULL )
			dk_parse_program( lexer );

		memset( cmdLine, 0, sizeof( cmdLine ) );
	}

	return EXIT_SUCCESS;
}
