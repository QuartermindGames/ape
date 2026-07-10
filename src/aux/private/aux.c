// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <plcore/pl_filesystem.h>
#include <plcore/pl_math.h>

#include <acm/acm.h>

#include "aux/public/aux_log.h"

#include "aux_private.h"

static int logPrint;
static int logWarn;
static int logError;

void aux_initialize( int argc, char **argv )
{
	PlInitialize( argc, argv );

	com_pack_pkg_register_();

	// Initialize directory lookups
	com_get_local_data_directory();
	com_get_app_data_directory();

	aux_log_initialize_();

	logPrint = aux_log_register_source( "aux", PL_COLOUR_WHITE, true );
	logWarn  = aux_log_register_source( "aux/warn", PL_COLOUR_YELLOW, true );
	logError = aux_log_register_source( "aux/error", PL_COLOUR_RED, true );
}

void aux_shutdown()
{
	aux_log_shutdown_();
}

const char *com_get_local_data_directory( void )
{
	// cache it
	static PLPath dataPath;
	if ( *dataPath != '\0' )
	{
		return dataPath;
	}

	PLPath exeDir;
	if ( PlGetExecutableDirectory( exeDir, sizeof( exeDir ) ) != NULL )
	{
		PlSetupPath( dataPath, true, "%s/../../runtime", exeDir );
		if ( PlPathExists( dataPath ) )
		{
			PlSetupPath( dataPath, true, "%s/../..", exeDir );
		}
		else
		{
			PlSetupPath( dataPath, true, "%s", exeDir );
		}
	}
	else
	{
		// oh dear oh dear...
		const char *cwd = PlGetWorkingDirectory();
		PlSetupPath( dataPath, true, "%s/../../runtime", cwd );
		if ( PlPathExists( dataPath ) )
		{
			PlSetupPath( dataPath, true, "%s/../..", cwd );
		}
		else
		{
			PlSetupPath( dataPath, true, "%s", cwd );
		}
	}

#if defined( __unix__ )

	char *rpath;
	if ( ( rpath = realpath( dataPath, nullptr ) ) != nullptr )
	{
		PlSetupPath( dataPath, true, "%s", rpath );
		free( rpath );
	}

#endif//todo: windows...

	return dataPath;
}

const char *com_get_app_data_directory( void )
{
	static PLPath appDataPath;
	if ( *appDataPath != '\0' )
	{
		return appDataPath;
	}

	if ( PlGetApplicationDataDirectory( "ape", appDataPath, sizeof( appDataPath ) ) != NULL )
	{
		return appDataPath;
	}

	PlSetupPath( appDataPath, true, "." );
	return appDataPath;
}

AcmBranch *com_get_config( const char *name )
{
	PLPath path;
	PlSetupPath( path, true, "configs/%s.cfg.n", name );
	AcmBranch *root = com_acm_load_file( path, "config" );
	if ( root == NULL )
	{
		root = acm_push_object( nullptr, "config" );
	}

	return root;
}

bool com_write_config( struct AcmBranch *root, const char *name )
{
	PLPath path;
	PlSetupPath( path, true, "%s/configs/", com_get_app_data_directory() );
	if ( !PlCreatePath( path ) )
	{
		return false;
	}

	PlSetupPath( path, true, "%s/configs/%s.cfg.n", com_get_app_data_directory(), name );
	return acm_write_file( path, root, ACM_FILE_TYPE_UTF8 );
}

/////////////////////////////////////////////////////////////////////////////////////

void com_print_( const char *m, ... )
{
	va_list args;
	va_start( args, m );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), m, args );

	va_end( args );

	aux_log_push_message( logPrint, "%s", buf );
}

void com_warning_( const char *m, ... )
{
	va_list args;
	va_start( args, m );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), m, args );

	va_end( args );

	aux_log_push_message( logWarn, "%s", buf );
}

void com_error_( const char *m, ... )
{
	va_list args;
	va_start( args, m );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), m, args );

	va_end( args );

	aux_log_push_message( logError, "%s", buf );

	abort();
}

/////////////////////////////////////////////////////////////////////////////////////

QmMathVector2f com_acm_get_vector2( AcmBranch *root, const char *name, const QmMathVector2f *fallback )
{
	AcmBranch *child = acm_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return *fallback;
	}

	QmMathVector2f v;
	if ( acm_branch_get_float32_array( child, ( float * ) &v, 2 ) != ND_ERROR_SUCCESS )
	{
		return *fallback;
	}

	return v;
}

QmMathVector3f com_acm_get_vector3( AcmBranch *root, const char *name, const QmMathVector3f *fallback )
{
	AcmBranch *child = acm_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return *fallback;
	}

	QmMathVector3f v;
	if ( acm_branch_get_float32_array( child, ( float * ) &v, 3 ) != ND_ERROR_SUCCESS )
	{
		return *fallback;
	}

	return v;
}

QmMathVector4f com_acm_get_vector4( AcmBranch *root, const char *name, const QmMathVector4f *fallback )
{
	AcmBranch *child = acm_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return *fallback;
	}

	QmMathVector4f v;
	if ( acm_branch_get_float32_array( child, ( float * ) &v, 4 ) != ND_ERROR_SUCCESS )
	{
		return *fallback;
	}

	return v;
}

QmMathColour4f com_acm_get_colour_f32( AcmBranch *root, const char *name, const QmMathColour4f *fallback )
{
	QmMathVector4f v = com_acm_get_vector4( root, name, ( QmMathVector4f * ) fallback );
	return PlVector4ToColourF32( &v );
}

AcmBranch *com_acm_push_vector2( AcmBranch *parent, const char *name, const QmMathVector2f *vector, bool conditional )
{
	if ( conditional && qm_math_vector2f_compare( *vector, QM_MATH_VECTOR2F_ZERO ) )
	{
		return nullptr;
	}

	return acm_push_array_f32( parent, name, ( float * ) vector, 2 );
}

AcmBranch *com_acm_push_vector3( AcmBranch *parent, const char *name, const QmMathVector3f *vector, bool conditional )
{
	if ( conditional && qm_math_vector3f_compare( *vector, QM_MATH_VECTOR3F_ZERO ) )
	{
		return nullptr;
	}

	return acm_push_array_f32( parent, name, ( float * ) vector, 3 );
}

AcmBranch *com_acm_push_vector4( AcmBranch *parent, const char *name, const QmMathVector4f *vector, bool conditional )
{
	if ( conditional && qm_math_vector4f_compare( *vector, QM_MATH_VECTOR4F_ZERO ) )
	{
		return nullptr;
	}

	return acm_push_array_f32( parent, name, ( float * ) vector, 4 );
}

AcmBranch *com_acm_push_colour4f( AcmBranch *parent, const char *name, const QmMathColour4f *colour, bool conditional )
{
	if ( conditional && qm_math_colour4f_compare( *colour, ( QmMathColour4f ) {} ) )
	{
		return nullptr;
	}

	return acm_push_array_f32( parent, name, ( float * ) colour, 4 );
}

AcmBranch *com_acm_load_file( const char *path, const char *object )
{
	QmFsFile *file = qm_fs_file_open( path, false );
	if ( file == nullptr )
	{
		return nullptr;
	}

	AcmBranch *root = nullptr;

	size_t size = qm_fs_file_get_size( file );
	if ( size > 0 )
	{
		uint8_t *buf = QM_OS_MEMORY_NEW_( uint8_t, size + 1 );
		if ( qm_file_read( file, buf, sizeof( uint8_t ), size ) == size )
		{
			root = acm_load_from_memory( buf, size, object, path );
		}

		qm_os_memory_free( buf );
	}

	PlCloseFile( file );

	return root;
}
