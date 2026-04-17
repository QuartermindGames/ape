// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Cook utility

#include <plcore/pl_filesystem.h>

#include "cook.h"

CookState cook_state;

static void process_collection( AcmBranch *root, const char *tag, void ( *callback )( const char * ) )
{
	assert( root != nullptr );
	assert( tag != nullptr );
	assert( callback != nullptr );

	AcmBranch *child = acm_get_child_by_name( root, tag );
	if ( child == nullptr )
	{
		printf( "No \"%s\" collection, skipping\n", tag );
		return;
	}

	unsigned int numTotal = acm_get_num_of_children( child );
	printf( "Processing %u %s...\n", numTotal, tag );

	unsigned int num = 1;
	child            = acm_get_first_child( child );
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

	AcmBranch *cookBranch = acm_get_child_by_name( root, "cook" );
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

	QM_OS_ZERO_( cook_state );

	aux_initialize( argc, argv );

	qm_fs_mount_local_location( com_get_app_data_directory() );
	qm_fs_mount_local_location( com_get_local_data_directory() );

	AcmBranch  *config;
	const char *projectName = argv[ 1 ];
	if ( ( config = com_project_mount( projectName ) ) == NULL )
	{
		ERROR( "Failed to mount project (%s)!\n", projectName );
	}

	unsigned int numCommands = 0;
	for ( unsigned int i = 2; i < argc; ++i )
	{
		if ( argv[ i ] == NULL )
		{
			break;
		}

		if ( pl_strcasecmp( argv[ i ], "/world" ) == 0 )
		{
			cook_world_process( argv[ ++i ] );
			numCommands++;
		}
		else if ( pl_strcasecmp( argv[ i ], "/model" ) == 0 )
		{
			cook_model_process( argv[ ++i ] );
			numCommands++;
		}
		else
		{
			printf( "Unknown command \"%s\", ignoring...\n", argv[ i ] );
		}
	}

	if ( numCommands > 0 )
	{
		return EXIT_SUCCESS;
	}

	// Otherwise we'll just carry on as normal
	cook_project( config );

	printf( "OK\n" );

	return EXIT_SUCCESS;
}
