// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Cook utility

#include "cook.h"

CookState cook_state;

static void process_collection( AcmBranch *root, const char *tag, void ( *callback )( const char * ) )
{
	assert( root != nullptr );
	assert( tag != nullptr );
	assert( callback != nullptr );

	AcmBranch *child = acm_branch_get_child_by_name( root, tag );
	if ( child == nullptr )
	{
		printf( "No \"%s\" collection, skipping\n", tag );
		return;
	}

	unsigned int numTotal = acm_branch_get_num_of_children( child );
	printf( "Processing %u %s...\n", numTotal, tag );

	unsigned int num = 1;
	child            = acm_branch_get_first_child( child );
	while ( child != nullptr )
	{
		char name[ 64 ];
		acm_branch_get_string( child, name, sizeof( name ) );

		printf( "(%u/%u) %s\n", num, numTotal, name );
		callback( name );

		child = acm_get_next_child( child );
		num++;
	}
}

static void cook_project( AcmBranch *root )
{
	const char *projectName = com_project_get_name();
	printf( "------------------------------------------------------\n"
	        "Cooking \"%s\" project...\n",
	        projectName );

	AcmBranch *cookBranch = acm_branch_get_child_by_name( root, "cook" );
	if ( cookBranch == NULL )
	{
		ERROR( "No cook configuration specified for project, aborting!\n" );
	}

	process_collection( cookBranch, "worlds", cook_world_process );
	process_collection( cookBranch, "models", cook_model_process );
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

	AcmBranch  *config;
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
	unsigned int              numCommands  = 0;
	LaunchCommand             commands[ MAX_COMMANDS ];
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
			commands[ numCommands ].command  = argv[ i ];
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
