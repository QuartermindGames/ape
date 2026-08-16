// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "ape_private.h"

#include "yin/core_fs.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static AcmBranch *fileSystemConfig;

static void parse_aliases( AcmBranch *root )
{
	unsigned int numAliases = acm_get_num_of_children( root ) / 2;
	if ( numAliases == 0 )
	{
		return;
	}

	AcmBranch *child = acm_get_first_child( root );
	if ( acm_branch_get_type( child ) != ACM_PROPERTY_TYPE_STRING )
	{
		ape_console_warning_( "Invalid child type found in config!\n" );
		return;
	}

	for ( unsigned int i = 0; i < numAliases; i++ )
	{
		PLPath aliasPath;
		acm_branch_get_string( child, aliasPath, sizeof( PLPath ) );
		child = acm_get_next_child( child );
		if ( child == NULL )
		{
			ape_console_warning_( "Encountered alias with no path: %u\n", i );
			break;
		}

		PLPath targetPath;
		acm_branch_get_string( child, targetPath, sizeof( PLPath ) );

		PlAddFileAlias( aliasPath, targetPath );
		ape_console_print_( "Registered alias: \"%s\" > \"%s\"\n", aliasPath, targetPath );

		child = acm_get_next_child( child );
	}
}

#define USER_CONFIG "user.cfg" ACM_DEFAULT_EXTENSION_OLD
static PLPath configPath;

/////////////////////////////////////////////////////////////////////////////////////
// Public

const char *ape_fs_get_user_config_location()
{
	if ( *configPath == '\0' )
	{
		// Figure out where to load/store the config
		const char *p = PlGetApplicationDataDirectory( ENGINE_APP_NAME, configPath, sizeof( configPath ) - ( strlen( USER_CONFIG ) + 1 ) );
		if ( p == NULL )
		{
			ape_console_warning_( "Failed to fetch application data directory, config may not be saved upon closing!\n" );
			snprintf( configPath, sizeof( configPath ), "./%s", USER_CONFIG );
		}
		else
		{
			if ( !PlCreateDirectory( p ) )
			{
				ape_console_warning_( "Failed to create application data directory: %s\n", p );
			}

			p = &p[ strlen( p ) - 1 ];
			if ( *p == '\\' || *p == '/' )
			{
				S_STRCAT( configPath, USER_CONFIG );
			}
			else
			{
				S_STRCAT( configPath, "/" USER_CONFIG );
			}
		}

		ape_console_print_( "Config: %s\n", configPath );
	}

	return configPath;
}

void ape_fs_setup_config( AcmBranch *root )
{
	PlClearFileAliases();

	fileSystemConfig = acm_get_child_by_name( root, "fileSystem" );
	if ( fileSystemConfig != NULL )
	{
		AcmBranch *child;
		if ( ( child = acm_get_child_by_name( fileSystemConfig, "aliases" ) ) != NULL )
			parse_aliases( child );
	}

	// TODO: move this into the project handler
}

void *ape_fs_load_file_buffer( const char *path, size_t *outSize )
{
	QmFsFile *file = qm_fs_file_open( path, true );
	if ( file == NULL )
	{
		ape_console_warning_( "Failed to open file (%s): %s\n", path, PlGetError() );
		return NULL;
	}

	size_t fileSize = qm_fs_file_get_size( file );
	*outSize        = fileSize + 1;
	char *buf       = QM_OS_MEMORY_NEW_( char, *outSize );
	memcpy( buf, qm_fs_file_get_data( file ), fileSize );

	PlCloseFile( file );

	return buf;
}

void ape_fs_mount_base_locations()
{
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) == NULL )
	{
		snprintf( exePath, sizeof( exePath ), "./" );
		ape_console_warning_( "Failed to get executable directory, using fallback!\n" );
	}

	qm_fs_mount_local_location( exePath );
}

time_t ape_fs_get_timestamp( const char *path )
{
	time_t r = 0;

	QmFsFile *file = qm_fs_file_open( path, false );
	if ( file != nullptr )
	{
		r = qm_fs_file_get_timestamp( file );
		PlCloseFile( file );
	}

	return r;
}
