// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>

#include "common_private.h"
#include "common_project.h"

#include "yin/node.h"

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
	char website[ 256 ];
	int version[ 3 ];

	PLFileSystemMount *mountLocation;// x/projects/blah

#define MAX_FILESYSTEM_MOUNTS 255
	PLFileSystemMount *subMountLocations[ MAX_FILESYSTEM_MOUNTS ];
	unsigned int numSubMountLocations;

	struct ComProject *parent;
	struct ComProject *dependencies[ COM_MAX_DEPENDENCIES ];
	unsigned int numDependencies;

	PLPath localPath;
} ComProject;

static ComProject project;

static void parse_mount_config( NdBranch *root, ComProject *out )
{
	unsigned int numChildren = ndGetNumOfChildren( root );
	if ( numChildren == 0 )
		/* nothing to mount, okay then */
		return;

	NdBranch *child = ndGetFirstChild( root );
	if ( ndGetType( child ) != ND_PROPERTY_STRING )
	{
		Warning( "Invalid child type found in config!\n" );
		return;
	}

	for ( unsigned int i = 0; i < numChildren; ++i )
	{
		PLPath path;
		ndGetStr( child, path, sizeof( PLPath ) );
		child = ndGetNextChild( child );

		if ( ( out->subMountLocations[ out->numSubMountLocations ] = PlMountLocation( path ) ) == NULL )
		{
			Warning( "Failed to mount \"%s\": %s\n", path, PlGetError() );
			continue;
		}

		out->numSubMountLocations++;
	}
}

static ComProject *deserialize_project( NdBranch *root, const char *name, ComProject *out )
{
	PLPath path;
	PlSetupPath( path, true, "%s/projects/%s", comGetDataDirectory(), name );
	out->mountLocation = PlMountLocalLocation( path );
	if ( out->mountLocation == NULL )
	{
		Warning( "Failed to mount project location: %s\n", PlGetError() );
		return NULL;
	}

	PlSetupPath( out->localPath, true, "%s", path );

	snprintf( out->baseName, sizeof( out->baseName ), "%s", name );
	snprintf( out->name, sizeof( out->name ), "%s", ndGetStringByName( root, "name", "none" ) );
	snprintf( out->developer, sizeof( out->developer ), "%s", ndGetStringByName( root, "developer", "none" ) );
	snprintf( out->website, sizeof( out->website ), "%s", ndGetStringByName( root, "website", "none" ) );

	NdBranch *child;
	if ( ( child = ndGetChildByName( root, "version" ) ) != NULL )
		ndGetI32Array( child, out->version, 3 );
	if ( ( child = ndGetChildByName( root, "mountLocations" ) ) != NULL )
		parse_mount_config( child, out );
	if ( ( child = ndGetChildByName( root, "dependencies" ) ) != NULL )
	{
		child = ndGetFirstChild( child );
		while ( child != NULL )
		{
			char baseName[ COM_MAX_PROJECT_BASENAME ];
			if ( ndGetStr( child, baseName, sizeof( baseName ) ) != ND_ERROR_SUCCESS )
			{
				Warning( "Failed to load dependency due to invalid dependency listing!\n" );
				return NULL;
			}

			PlSetupPath( path, true, "%s/projects/%s/%s.prj.n", comGetDataDirectory(), baseName, baseName );

			NdBranch *croot = ndLoadFile( path, "project" );
			if ( croot == NULL )
			{
				Warning( "Failed to load depedency (%s) project file: %s\n", baseName, ndGetErrorMessage() );
				return NULL;
			}

			// A lot of this is going to fall to bits if you have a project that depends on another
			// project that then depends back on the first project or vice-versa. We'll deal with this
			// if we ever make any sort of public SDK, but for now I'm quickly throwing this together
			// to meet a deadline... aaaahhhh

			ComProject *head = ( out->parent == NULL ) ? out : out->parent;
			if ( strcmp( head->baseName, baseName ) == 0 )
			{
				Warning( "Project is including self as dependency, bailing!\n" );
				return NULL;
			}

			unsigned int index = head->numDependencies;
			head->dependencies[ index ] = PL_NEW( ComProject );
			head->dependencies[ index ]->parent = out;
			head->numDependencies++;

			if ( deserialize_project( croot, baseName, head->dependencies[ index ] ) == NULL )
			{
				Warning( "Failed to load dependency (%s)!\n", baseName );
				return NULL;
			}

			ndDestroyBranch( croot );

			if ( head->numDependencies >= COM_MAX_DEPENDENCIES )
			{
				Warning( "Hit dependency limit (%u), bailing on others (this might mean content will be missing!)\n", COM_MAX_DEPENDENCIES );
				break;
			}

			child = ndGetNextChild( child );
		}
	}

	return out;
}

static void free_project( ComProject *out )
{
	if ( out->mountLocation != NULL )
	{
		PlClearMountedLocation( out->mountLocation );
		out->mountLocation = NULL;
	}

	for ( unsigned int i = 0; i < out->numSubMountLocations; ++i )
	{
		if ( out->subMountLocations[ i ] == NULL )
			break;

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

bool com_project_mount( const char *name )
{
	assert( !project.isActive );
	if ( project.isActive )
	{
		Warning( "A project is already active! Unmount current project first.\n" );
		return false;
	}

	PLPath path;
	PlSetupPath( path, true, "%s/projects/%s/%s.prj.n", comGetDataDirectory(), name, name );

	NdBranch *root = ndLoadFile( path, "project" );
	if ( root == NULL )
	{
		Warning( "Failed to load project file: %s\n", ndGetErrorMessage() );
		return false;
	}

	if ( deserialize_project( root, name, &project ) == NULL )
		com_project_unmount();// call unmount to cleanup

	ndDestroyBranch( root );

	return true;
}

void com_project_unmount( void )
{
	free_project( &project );

	PL_ZERO_( project );
}

const char *com_project_get_local_path( void )
{
	return project.localPath;
}
