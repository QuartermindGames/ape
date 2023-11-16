// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"

#include "yin/core_fs.h"
#include <yin/node.h>

/////////////////////////////////////////////////////////////////////////////////////
// Private

static NdBranch *fileSystemConfig;

static void parse_aliases( NdBranch *root )
{
	unsigned int numAliases = ndGetNumOfChildren( root ) / 2;
	if ( numAliases == 0 )
	{
		return;
	}

	NdBranch *child = ndGetFirstChild( root );
	if ( ndGetType( child ) != ND_PROPERTY_STRING )
	{
		PRINT_WARNING( "Invalid child type found in config!\n" );
		return;
	}

	for ( unsigned int i = 0; i < numAliases; i++ )
	{
		PLPath aliasPath;
		ndGetStr( child, aliasPath, sizeof( PLPath ) );
		child = ndGetNextChild( child );
		if ( child == NULL )
		{
			PRINT_WARNING( "Encountered alias with no path: %u\n", i );
			break;
		}

		PLPath targetPath;
		ndGetStr( child, targetPath, sizeof( PLPath ) );

		PlAddFileAlias( aliasPath, targetPath );
		PRINT( "Registered alias: \"%s\" > \"%s\"\n", aliasPath, targetPath );

		child = ndGetNextChild( child );
	}
}

#define USER_CONFIG "user.cfg" ND_DEFAULT_EXTENSION
static PLPath configPath = { '\0' };

/////////////////////////////////////////////////////////////////////////////////////
// Public

const char *ss_acl_fs_get_user_config_location( void )
{
	if ( *configPath == '\0' )
	{
		// Figure out where to load/store the config
		const char *p = PlGetApplicationDataDirectory( ENGINE_APP_NAME, configPath, sizeof( configPath ) - ( strlen( USER_CONFIG ) + 1 ) );
		if ( p == NULL )
		{
			PRINT_WARNING( "Failed to fetch application data directory, config may not be saved upon closing!\n" );
			snprintf( configPath, sizeof( configPath ), "./%s", USER_CONFIG );
		}
		else
		{
			if ( !PlCreateDirectory( p ) )
				PRINT_WARNING( "Failed to create application data directory: %s\n", p );

			p = &p[ strlen( p ) - 1 ];
			if ( *p == '\\' || *p == '/' )
				strcat( configPath, USER_CONFIG );
			else
				strcat( configPath, "/" USER_CONFIG );
		}

		PRINT( "Config: %s\n", configPath );
	}

	return configPath;
}

void ss_acl_fs_setup_config( NdBranch *root )
{
	PlClearFileAliases();

	fileSystemConfig = ndGetChildByName( root, "fileSystem" );
	if ( fileSystemConfig != NULL )
	{
		NdBranch *child;
		if ( ( child = ndGetChildByName( fileSystemConfig, "aliases" ) ) != NULL )
			parse_aliases( child );
	}

	// TODO: move this into the project handler
}

void ss_acl_fs_mount_base_locations( void )
{
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) == NULL )
	{
		snprintf( exePath, sizeof( exePath ), "./" );
		PRINT_WARNING( "Failed to get executable directory, using fallback!\n" );
	}

	PlMountLocalLocation( exePath );
	PlMountLocalLocation( comGetDataDirectory() );
}

char *ss_acl_fs_parse_string( PLFile *file, uint16_t *size )
{
	bool status;
	*size = PL_READUINT16( file, false, &status );
	assert( status );
	if ( *size == 0 || !status )
		return NULL;

	char *buf = PL_NEW_( char, ( *size ) + 1 );
	size_t rb = PlReadFile( file, buf, sizeof( char ), *size );
	assert( rb == *size );
	return buf;
}

float ss_acl_fs_parse_float( PLFile *file )
{
	bool status;
	float f = PlReadFloat32( file, false, &status );
	assert( status && !isnan( f ) );
	return f;
}

PLVector3 ss_acl_fs_parse_vector( PLFile *file )
{
	return ( PLVector3 ){
	        ss_acl_fs_parse_float( file ),
	        ss_acl_fs_parse_float( file ),
	        ss_acl_fs_parse_float( file ),
	};
}

PLMatrix3 ss_acl_fs_parse_mat3( PLFile *file )
{
	return ( PLMatrix3 ){
	        // forward
	        .m[ 0 ] = ss_acl_fs_parse_float( file ),
	        .m[ 1 ] = ss_acl_fs_parse_float( file ),
	        .m[ 2 ] = ss_acl_fs_parse_float( file ),
	        // right
	        .m[ 3 ] = ss_acl_fs_parse_float( file ),
	        .m[ 4 ] = ss_acl_fs_parse_float( file ),
	        .m[ 5 ] = ss_acl_fs_parse_float( file ),
	        // up
	        .m[ 6 ] = ss_acl_fs_parse_float( file ),
	        .m[ 7 ] = ss_acl_fs_parse_float( file ),
	        .m[ 8 ] = ss_acl_fs_parse_float( file ),
	};
}

PLColour ss_acl_fs_parse_colour( PLFile *file )
{
	bool status;
	PLColour c = ( PLColour ){
	        PL_READUINT8( file, &status ),
	        PL_READUINT8( file, &status ),
	        PL_READUINT8( file, &status ),
	        PL_READUINT8( file, &status ) };
	assert( status );
	return c;
}
