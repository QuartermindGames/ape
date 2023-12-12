// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>
#include <plcore/pl_console.h>

#include <yin/node.h>

#include "common_private.h"

int com_logLevels_[ COM_MAX_LOG_LEVELS ];

void com_initialize( void )
{
	com_logLevels_[ COM_LOG_LEVEL_INFO ] = PlAddLogLevel( "common", PL_COLOUR_WHITE, true );
	com_logLevels_[ COM_LOG_LEVEL_WARN ] = PlAddLogLevel( "common/warning", PL_COLOUR_YELLOW, true );
	com_logLevels_[ COM_LOG_LEVEL_ERROR ] = PlAddLogLevel( "common/error", PL_COLOUR_RED, true );
	com_logLevels_[ COM_LOG_LEVEL_DEBUG ] = PlAddLogLevel( "common/debug", PL_COLOUR_WHITE, true );

	Message( "Common Library initialized\n" );

	ndSetupLogs();

	com_pack_pkg_register_();
	com_pack_vpp_register_();

	// Initialize directory lookups
	ss_com_get_local_data_directory();
	ss_com_get_app_data_directory();
}

const char *ss_com_get_local_data_directory( void )
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

const char *ss_com_get_app_data_directory( void )
{
	static PLPath appDataPath = "";
	if ( *appDataPath != '\0' )
		return appDataPath;

	if ( PlGetApplicationDataDirectory( "ape", appDataPath, sizeof( appDataPath ) ) != NULL )
		return appDataPath;

	Warning( "Failed to fetch application data directory: %s\n", PlGetError() );

	PlSetupPath( appDataPath, true, "." );
	return appDataPath;
}

NdBranch *ss_com_get_config( const char *name )
{
	PLPath path;
	PlSetupPath( path, true, "configs/%s.cfg.n", name );
	NdBranch *root = ndLoadFile( path, "config" );
	if ( root == NULL )
	{
		Warning( "Failed to load user config file (%s)! Creating empty config.\n", ndGetErrorMessage() );
		root = ndPushBackObject( NULL, "config" );
	}

	return root;
}

bool ss_com_write_config( struct NdBranch *root, const char *name )
{
	PLPath path;
	PlSetupPath( path, true, "%s/configs/", ss_com_get_app_data_directory() );
	if ( !PlCreatePath( path ) )
	{
		Warning( "Failed to create configs path (%s): %s\n", path, PlGetError() );
		return false;
	}

	PlSetupPath( path, true, "%s/configs/%s.cfg.n", ss_com_get_app_data_directory(), name );
	return ndWriteFile( path, root, ND_FILE_UTF8 );
}
