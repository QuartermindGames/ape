// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Cook utility

#include "cook.h"

CookState cook_state;

int main( int argc, char **argv )
{
	printf( "COOK - APE's project cooking utility!\n"
	        "------------------------------------------------------\n" );

	if ( argc <= 1 )
	{
		printf( "Usage: cook <project-name>\n" );
		return EXIT_SUCCESS;
	}

	if ( PlInitialize( argc, argv ) != PL_RESULT_SUCCESS )
		ERROR( "Failed to initialize plcore: %s\n", PlGetError() );

	PL_ZERO_( cook_state );

	com_initialize();

	PlMountLocalLocation( comGetAppDataDirectory() );
	PlMountLocalLocation( comGetDataDirectory() );

	const char *projectName = argv[ 1 ];
	if ( !com_project_mount( projectName ) )
		ERROR( "Failed to mount project (%s)!\n", projectName );

	PLPath configPath;
	PlSetupPath( configPath, true, "projects/%s/%s.prj.n", argv[ 1 ] );

	typedef struct LaunchCommand
	{
		const char *command;
		const char *argument;
	} LaunchCommand;

	// Collect up all commands we've been issued, if any,
	// as we might not want to operate on the entire project
	static const unsigned int MAX_COMMANDS = 16;
	unsigned int numCommands = 0;
	LaunchCommand commands[ MAX_COMMANDS ];
	for ( unsigned int i = 2; i < argc; ++i )
	{
		if ( argv[ i ] == NULL )
			break;

		if ( numCommands >= MAX_COMMANDS )
		{
			printf( "Hit command limit for single execution!\n" );
			break;
		}

		if ( pl_strcasecmp( argv[ i ], "/world" ) == 0 )
		{
			commands[ numCommands ].command = argv[ i ];
			commands[ numCommands ].argument = argv[ ++i ];
			numCommands++;
		}
		else
			printf( "Unknown command \"%s\", ignoring...\n", argv[ i ] );
	}

	// Let's go ahead and operate on them now
	if ( numCommands > 0 )
	{
		printf( "Executing commands...\n" );
		for ( unsigned int i = 0; i < numCommands; ++i )
		{
			printf( " %s \"%s\" -> ", commands[ i ].command, commands[ i ].argument );
			if ( pl_strcasecmp( commands[ i ].command, "/world" ) == 0 )
				cook_world_process( commands[ i ].argument );

			printf( "OK\n" );
		}
		return EXIT_SUCCESS;
	}

	// Otherwise we'll just carry on as normal
	printf( "Processing \"%s\" project...\n", argv[ 1 ] );
	//TODO: operate on entire project

	return EXIT_SUCCESS;
}
