// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Cook utility

#include "cook.h"

CookState cook_state;

static void cook_project( NdBranch *root )
{
	const char *projectName = com_project_get_name();
	printf( "------------------------------------------------------\n"
	        "Cooking \"%s\" project...\n", projectName );

	NdBranch *cookBranch = nd_branch_get_child_by_name( root, "cook" );
	if ( cookBranch == NULL )
	{
		ERROR( "No cook configuration specified for project, aborting!\n" );
	}

	NdBranch *childBranch;

	childBranch = nd_branch_get_child_by_name( cookBranch, "worlds" );
	if ( childBranch != NULL )
	{
		printf( "Processing %u worlds...\n", nd_branch_get_num_of_children( childBranch ) );

		childBranch = nd_branch_get_first_child( childBranch );
		while( childBranch != NULL )
		{
			char worldName[ 64 ];
			nd_branch_get_string( childBranch, worldName, sizeof( worldName ) );

			cook_world_process( worldName );

			childBranch = nd_get_next_child( childBranch );
		}
	}

	childBranch = nd_branch_get_child_by_name( cookBranch, "models" );
	if ( childBranch != NULL )
	{
		printf( "Processing %u models...\n", nd_branch_get_num_of_children( childBranch ) );

		childBranch = nd_branch_get_first_child( childBranch );
		while ( childBranch != NULL )
		{
			char modelName[ 64 ];
			nd_branch_get_string( childBranch, modelName, sizeof( modelName ) );

			cook_model_process( modelName );

			childBranch = nd_get_next_child( childBranch );
		}
	}
}

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
	{
		ERROR( "Failed to initialize plcore: %s\n", PlGetError() );
	}

	PL_ZERO_( cook_state );

	com_initialize();

	PlMountLocalLocation( com_get_app_data_directory() );
	PlMountLocalLocation( com_get_local_data_directory() );

	NdBranch *config;
	const char *projectName = argv[ 1 ];
	if ( ( config = com_project_mount( projectName ) ) == NULL )
	{
		ERROR( "Failed to mount project (%s)!\n", projectName );
	}

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
		{
			break;
		}

		if ( numCommands >= MAX_COMMANDS )
		{
			printf( "Hit command limit for single execution!\n" );
			break;
		}

		if ( pl_strcasecmp( argv[ i ], "/world" ) == 0 || pl_strcasecmp( argv[ i ], "/model" ) == 0 )
		{
			commands[ numCommands ].command = argv[ i ];
			commands[ numCommands ].argument = argv[ ++i ];
			numCommands++;
		}
		else
		{
			printf( "Unknown command \"%s\", ignoring...\n", argv[ i ] );
		}
	}

	// Let's go ahead and operate on them now
	if ( numCommands > 0 )
	{
		printf( "Executing commands...\n" );
		for ( unsigned int i = 0; i < numCommands; ++i )
		{
			printf( " %s \"%s\" -> ", commands[ i ].command, commands[ i ].argument );
			if ( pl_strcasecmp( commands[ i ].command, "/world" ) == 0 )
			{
				cook_world_process( commands[ i ].argument );
			}
			else if ( pl_strcasecmp( commands[ i ].command, "/model" ) == 0 )
			{
				cook_model_process( commands[ i ].argument );
			}

			printf( "OK\n" );
		}
		return EXIT_SUCCESS;
	}

	// Otherwise we'll just carry on as normal
	cook_project( config );

	printf( "OK\n" );

	return EXIT_SUCCESS;
}
