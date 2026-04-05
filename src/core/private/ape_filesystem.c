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

const char *ape_fs_get_user_config_location( void )
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

void ape_fs_mount_base_locations( void )
{
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) == NULL )
	{
		snprintf( exePath, sizeof( exePath ), "./" );
		ape_console_warning_( "Failed to get executable directory, using fallback!\n" );
	}

	qm_fs_mount_local_location( exePath );
}

char *ape_fs_parse_string( QmFsFile *file, uint16_t *size )
{
	bool status;
	*size = PL_READUINT16( file, false, &status );
	assert( status );
	if ( *size == 0 || !status )
		return nullptr;

	char  *buf = QM_OS_MEMORY_NEW_( char, ( *size ) + 1 );
	size_t rb  = PlReadFile( file, buf, sizeof( char ), *size );
	assert( rb == *size );
	return buf;
}

char *ape_fs_parse_string_ex( QmFsFile *file, uint16_t *size, unsigned int version, unsigned int minVersion, unsigned int maxVersion )
{
	if ( version < minVersion || version > maxVersion )
		return nullptr;

	return ape_fs_parse_string( file, size );
}

uint8_t ape_fs_parse_byte( QmFsFile *file )
{
	bool    status;
	uint8_t i = PL_READUINT8( file, &status );
	assert( status );
	return i;
}

uint8_t ss_acl_fs_parse_byte_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, uint8_t fallback )
{
	if ( version < minVersion || version > maxVersion )
		return fallback;

	return ape_fs_parse_byte( file );
}

int ss_acl_fs_parse_int( QmFsFile *file )
{
	bool status;
	int  i = qm_fs_file_read_int32( file, false, &status );
	assert( status );
	return i;
}

int ss_acl_fs_parse_int_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, int fallback )
{
	if ( version < minVersion || version > maxVersion )
		return fallback;

	return ss_acl_fs_parse_int( file );
}

float ss_acl_fs_parse_float( QmFsFile *file )
{
	bool  status;
	float f = qm_fs_file_read_float( file, false, &status );
	assert( status && !isnan( f ) );
	return f;
}

float acl_fs_parse_float_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, float fallback )
{
	if ( version < minVersion || version > maxVersion )
		return fallback;

	return ss_acl_fs_parse_float( file );
}

QmMathVector3f ss_acl_fs_parse_vector( QmFsFile *file )
{
	return qm_math_vector3f( ss_acl_fs_parse_float( file ), ss_acl_fs_parse_float( file ), ss_acl_fs_parse_float( file ) );
}

QmMathVector3f acl_fs_parse_vector_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const QmMathVector3f *fallback )
{
	if ( version < minVersion || version > maxVersion )
		return *fallback;

	return ss_acl_fs_parse_vector( file );
}

QmMathVector4f ss_acl_fs_parse_vector4( QmFsFile *file )
{
	return qm_math_vector4f( ss_acl_fs_parse_float( file ), ss_acl_fs_parse_float( file ), ss_acl_fs_parse_float( file ), ss_acl_fs_parse_float( file ) );
}

QmMathVector4f ss_acl_fs_parse_vector4_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const QmMathVector4f *fallback )
{
	if ( version < minVersion || version > maxVersion )
		return *fallback;

	return ss_acl_fs_parse_vector4( file );
}

PLMatrix3 ss_acl_fs_parse_mat3( QmFsFile *file )
{
	return ( PLMatrix3 ) {
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

QmMathColour4ub ss_acl_fs_parse_colour( QmFsFile *file )
{
	bool            status;
	QmMathColour4ub c = qm_math_colour4ub( PL_READUINT8( file, &status ), PL_READUINT8( file, &status ), PL_READUINT8( file, &status ), PL_READUINT8( file, &status ) );
	assert( status );
	return c;
}
