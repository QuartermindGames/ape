// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"

#include "yin/core_fs.h"

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

void ape_fs_setup_config( NdBranch *root )
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

void *ss_acl_fs_load_file_buffer( const char *path, size_t *outSize )
{
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		PRINT_WARNING( "Failed to open file (%s): %s\n", path, PlGetError() );
		return NULL;
	}

	size_t fileSize = PlGetFileSize( file );
	*outSize = fileSize + 1;
	char *buf = PL_NEW_( char, *outSize );
	memcpy( buf, PlGetFileData( file ), fileSize );

	PlCloseFile( file );

	return buf;
}

void ape_fs_mount_base_locations( void )
{
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) == NULL )
	{
		snprintf( exePath, sizeof( exePath ), "./" );
		PRINT_WARNING( "Failed to get executable directory, using fallback!\n" );
	}

	PlMountLocalLocation( exePath );
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

char *ss_acl_fs_parse_string_ex( PLFile *file, uint16_t *size, unsigned int version, unsigned int minVersion, unsigned int maxVersion )
{
	if ( version < minVersion || version > maxVersion )
		return NULL;

	return ss_acl_fs_parse_string( file, size );
}

uint8_t ss_acl_fs_parse_byte( PLFile *file )
{
	bool status;
	uint8_t i = PL_READUINT8( file, &status );
	assert( status );
	return i;
}

uint8_t ss_acl_fs_parse_byte_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, uint8_t fallback )
{
	if ( version < minVersion || version > maxVersion )
		return fallback;

	return ss_acl_fs_parse_byte( file );
}

int ss_acl_fs_parse_int( PLFile *file )
{
	bool status;
	int i = PlReadInt32( file, false, &status );
	assert( status );
	return i;
}

int ss_acl_fs_parse_int_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, int fallback )
{
	if ( version < minVersion || version > maxVersion )
		return fallback;

	return ss_acl_fs_parse_int( file );
}

float ss_acl_fs_parse_float( PLFile *file )
{
	bool status;
	float f = PlReadFloat32( file, false, &status );
	assert( status && !isnan( f ) );
	return f;
}

float acl_fs_parse_float_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, float fallback )
{
	if ( version < minVersion || version > maxVersion )
		return fallback;

	return ss_acl_fs_parse_float( file );
}

PLVector3 ss_acl_fs_parse_vector( PLFile *file )
{
	return ( PLVector3 ){
	        ss_acl_fs_parse_float( file ),
	        ss_acl_fs_parse_float( file ),
	        ss_acl_fs_parse_float( file ),
	};
}

PLVector3 acl_fs_parse_vector_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const PLVector3 *fallback )
{
	if ( version < minVersion || version > maxVersion )
		return *fallback;

	return ss_acl_fs_parse_vector( file );
}

PLVector4 ss_acl_fs_parse_vector4( PLFile *file )
{
	return ( PLVector4 ){
	        ss_acl_fs_parse_float( file ),
	        ss_acl_fs_parse_float( file ),
	        ss_acl_fs_parse_float( file ),
	        ss_acl_fs_parse_float( file ),
	};
}

PLVector4 ss_acl_fs_parse_vector4_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const PLVector4 *fallback )
{
	if ( version < minVersion || version > maxVersion )
		return *fallback;

	return ss_acl_fs_parse_vector4( file );
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
