// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_filesystem.h>
#include <plcore/pl_console.h>

#include <yin/node.h>

#include "common_private.h"

enum ComLogLevel
{
	COM_LOG_LEVEL_INFO,
	COM_LOG_LEVEL_DEBUG,
	COM_LOG_LEVEL_WARN,
	COM_LOG_LEVEL_ERROR,

	COM_MAX_LOG_LEVELS
};

static int com_logLevels_[ COM_MAX_LOG_LEVELS ];

void com_initialize( void )
{
	com_logLevels_[ COM_LOG_LEVEL_INFO ] = PlAddLogLevel( "common", PL_COLOUR_WHITE, true );
	com_logLevels_[ COM_LOG_LEVEL_WARN ] = PlAddLogLevel( "common/warning", PL_COLOUR_YELLOW, true );
	com_logLevels_[ COM_LOG_LEVEL_ERROR ] = PlAddLogLevel( "common/error", PL_COLOUR_RED, true );
	com_logLevels_[ COM_LOG_LEVEL_DEBUG ] = PlAddLogLevel( "common/debug", PL_COLOUR_WHITE, true );

	com_print_( "Common Library initialized\n" );

	nd_setup_logs();

	com_pack_pkg_register_();

	// Initialize directory lookups
	com_get_local_data_directory();
	com_get_app_data_directory();
}

const char *com_get_local_data_directory( void )
{
	// cache it
	static PLPath dataPath = { '\0' };
	if ( *dataPath != '\0' )
		return dataPath;

	PLPath exeDir;
	if ( PlGetExecutableDirectory( exeDir, sizeof( exeDir ) ) != NULL )
	{
		PlSetupPath( dataPath, true, "%s/../../runtime", exeDir );
		if ( PlPathExists( dataPath ) )
		{
			PlSetupPath( dataPath, true, "%s/../..", exeDir );
			return dataPath;
		}

		PlSetupPath( dataPath, true, "%s", exeDir );
		return dataPath;
	}

	// oh dear oh dear...

	const char *cwd = PlGetWorkingDirectory();
	PlSetupPath( dataPath, true, "%s/../../runtime", cwd );
	if ( PlPathExists( dataPath ) )
		PlSetupPath( dataPath, true, "%s/../..", cwd );
	else
		PlSetupPath( dataPath, true, "%s", cwd );

	return dataPath;
}

const char *com_get_app_data_directory( void )
{
	static PLPath appDataPath = "";
	if ( *appDataPath != '\0' )
		return appDataPath;

	if ( PlGetApplicationDataDirectory( "ape", appDataPath, sizeof( appDataPath ) ) != NULL )
		return appDataPath;

	com_warning_( "Failed to fetch application data directory: %s\n", PlGetError() );

	PlSetupPath( appDataPath, true, "." );
	return appDataPath;
}

NdBranch *com_get_config( const char *name )
{
	PLPath path;
	PlSetupPath( path, true, "configs/%s.cfg.n", name );
	NdBranch *root = nd_load_file( path, "config" );
	if ( root == NULL )
	{
		com_warning_( "Failed to load user config file (%s)! Creating empty config.\n", nd_get_error_message() );
		root = nd_branch_push_back_object( NULL, "config" );
	}

	return root;
}

bool com_write_config( struct NdBranch *root, const char *name )
{
	PLPath path;
	PlSetupPath( path, true, "%s/configs/", com_get_app_data_directory() );
	if ( !PlCreatePath( path ) )
	{
		com_warning_( "Failed to create configs path (%s): %s\n", path, PlGetError() );
		return false;
	}

	PlSetupPath( path, true, "%s/configs/%s.cfg.n", com_get_app_data_directory(), name );
	return nd_write_file( path, root, ND_FILE_UTF8 );
}

/////////////////////////////////////////////////////////////////////////////////////

void com_print_( const char *m, ... )
{
	va_list args;
	va_start( args, m );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), m, args );

	va_end( args );

	PlLogMessage( com_logLevels_[ COM_LOG_LEVEL_INFO ], "%s", buf );
}

void com_warning_( const char *m, ... )
{
	va_list args;
	va_start( args, m );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), m, args );

	va_end( args );

	PlLogMessage( com_logLevels_[ COM_LOG_LEVEL_WARN ], "%s", buf );
}

void com_error_( const char *m, ... )
{
	va_list args;
	va_start( args, m );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), m, args );

	va_end( args );

	PlLogMessage( com_logLevels_[ COM_LOG_LEVEL_ERROR ], "%s", buf );

	abort();
}
