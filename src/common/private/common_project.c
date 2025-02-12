// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_filesystem.h>

#include <acm/acm.h>

#include "common_private.h"
#include "common_project.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

#define COM_MAX_PROJECT_BASENAME 64
#define COM_MAX_PROJECT_NAME     64
#define COM_MAX_DEPENDENCIES     8

typedef struct ComProject
{
	bool isActive;

	char baseName[ COM_MAX_PROJECT_BASENAME ];
	char name[ COM_MAX_PROJECT_NAME ];
	char developer[ 64 ];
	int  version[ 3 ];

	PLFileSystemMount *mountLocation;// x/projects/blah

#define MAX_FILESYSTEM_MOUNTS 255
	PLFileSystemMount *subMountLocations[ MAX_FILESYSTEM_MOUNTS ];
	unsigned int       numSubMountLocations;

	struct ComProject *parent;
	struct ComProject *dependencies[ COM_MAX_DEPENDENCIES ];
	unsigned int       numDependencies;

	AcmBranch *config;

	PLPath localPath;
} ComProject;

static ComProject project;

static void parse_mount_config( AcmBranch *root, ComProject *out )
{
	unsigned int numChildren = acm_get_num_of_children( root );
	if ( numChildren == 0 )
	{
		// nothing to mount, okay then
		return;
	}

	AcmBranch *child = acm_get_first_child( root );
	if ( acm_branch_get_type( child ) != ACM_PROPERTY_TYPE_STRING )
	{
		com_warning_( "Invalid child type found in config!\n" );
		return;
	}

	for ( unsigned int i = 0; i < numChildren; ++i )
	{
		PLPath path;
		acm_branch_get_string( child, path, sizeof( PLPath ) );
		child = acm_get_next_child( child );

		if ( ( out->subMountLocations[ out->numSubMountLocations ] = PlMountLocation( path ) ) == NULL )
		{
			com_warning_( "Failed to mount \"%s\": %s\n", path, PlGetError() );
			continue;
		}

		out->numSubMountLocations++;
	}
}

static ComProject *deserialize_project( AcmBranch *root, const char *name, ComProject *out )
{
	PLPath path;
	PlSetupPath( path, true, "%s/projects/%s", com_get_local_data_directory(), name );
	out->mountLocation = PlMountLocalLocation( path );
	if ( out->mountLocation == NULL )
	{
		com_warning_( "Failed to mount project location: %s\n", PlGetError() );
		return NULL;
	}

	PlSetupPath( out->localPath, true, "%s", path );

	snprintf( out->baseName, sizeof( out->baseName ), "%s", name );
	snprintf( out->name, sizeof( out->name ), "%s", acm_get_string( root, "name", "none" ) );
	snprintf( out->developer, sizeof( out->developer ), "%s", acm_get_string( root, "developer", "none" ) );

	AcmBranch *child;
	if ( ( child = acm_get_child_by_name( root, "version" ) ) != NULL )
	{
		acm_branch_get_int32_array( child, out->version, 3 );
	}
	if ( ( child = acm_get_child_by_name( root, "mountLocations" ) ) != NULL )
	{
		parse_mount_config( child, out );
	}
	if ( ( child = acm_get_child_by_name( root, "dependencies" ) ) != NULL )
	{
		child = acm_get_first_child( child );
		while ( child != NULL )
		{
			char baseName[ COM_MAX_PROJECT_BASENAME ];
			if ( acm_branch_get_string( child, baseName, sizeof( baseName ) ) != ND_ERROR_SUCCESS )
			{
				com_warning_( "Failed to load dependency due to invalid dependency listing!\n" );
				return NULL;
			}

			PlSetupPath( path, true, "%s/projects/%s/%s.prj.n", com_get_local_data_directory(), baseName, baseName );

			AcmBranch *croot = com_acm_load_file( path, "project" );
			if ( croot == NULL )
			{
				com_warning_( "Failed to load depedency (%s) project file: %s\n", baseName, acm_get_error_message() );
				return NULL;
			}

			// A lot of this is going to fall to bits if you have a project that depends on another
			// project that then depends back on the first project or vice-versa. We'll deal with this
			// if we ever make any sort of public SDK, but for now I'm quickly throwing this together
			// to meet a deadline... aaaahhhh

			ComProject *head = ( out->parent == NULL ) ? out : out->parent;
			if ( strcmp( head->baseName, baseName ) == 0 )
			{
				com_warning_( "Project is including self as dependency, bailing!\n" );
				return NULL;
			}

			unsigned int index                  = head->numDependencies;
			head->dependencies[ index ]         = PL_NEW( ComProject );
			head->dependencies[ index ]->parent = out;
			head->numDependencies++;

			if ( deserialize_project( croot, baseName, head->dependencies[ index ] ) == NULL )
			{
				com_warning_( "Failed to load dependency (%s)!\n", baseName );
				return NULL;
			}

			acm_branch_destroy( croot );

			if ( head->numDependencies >= COM_MAX_DEPENDENCIES )
			{
				com_warning_( "Hit dependency limit (%u), bailing on others (this might mean content will be missing!)\n", COM_MAX_DEPENDENCIES );
				break;
			}

			child = acm_get_next_child( child );
		}
	}

	return out;
}

static void free_project( ComProject *out )
{
	if ( out->config != NULL )
	{
		acm_branch_destroy( out->config );
		out->config = NULL;
	}

	if ( out->mountLocation != NULL )
	{
		PlClearMountedLocation( out->mountLocation );
		out->mountLocation = NULL;
	}

	for ( unsigned int i = 0; i < out->numSubMountLocations; ++i )
	{
		if ( out->subMountLocations[ i ] == NULL )
		{
			break;
		}

		PlClearMountedLocation( out->subMountLocations[ i ] );
		out->subMountLocations[ i ] = NULL;
	}
	out->numSubMountLocations = 0;

	for ( unsigned int i = 0; i < out->numDependencies; ++i )
	{
		if ( out->dependencies[ i ] == NULL )
			break;

		free_project( out->dependencies[ i ] );
		PL_DELETE( out->dependencies[ i ] );
		out->dependencies[ i ] = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

AcmBranch *com_project_mount( const char *name )
{
	assert( !project.isActive );
	if ( project.isActive )
	{
		com_warning_( "A project is already active! Unmount current project first.\n" );
		return nullptr;
	}

	PLPath path;
	PlSetupPath( path, true, "%s/projects/%s/%s.prj.n", com_get_local_data_directory(), name, name );

	AcmBranch *root = com_acm_load_file( path, "project" );
	if ( root == NULL )
	{
		com_warning_( "Failed to load project file: %s\n", acm_get_error_message() );
		return nullptr;
	}

	if ( deserialize_project( root, name, &project ) == NULL )
	{
		com_project_unmount();// call unmount to cleanup
	}

	project.config = root;
	return project.config;
}

void com_project_unmount( void )
{
	free_project( &project );

	PL_ZERO_( project );
}

const char *com_project_get_local_path( void ) { return project.localPath; }
const char *com_project_get_base_name( void ) { return project.baseName; }
const char *com_project_get_name( void ) { return project.name; }

AcmBranch *com_project_get_config()
{
	return project.config;
}
