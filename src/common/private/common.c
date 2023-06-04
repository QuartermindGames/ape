// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>
#include <plcore/pl_console.h>

#include <yin/node.h>

#include "common_private.h"

int logLevelPrint;
int logLevelWarn;

void cmnInitialize( void )
{
	logLevelPrint = PlAddLogLevel( "common", PL_COLOUR_WHITE, true );
	logLevelWarn  = PlAddLogLevel( "common/warning", PL_COLOUR_YELLOW, true );

	Message( "Common Library initialized\n" );

	ndSetupLogs();

	cmnPack_RegisterPkgInterface_();
	cmnPack_RegisterVppInterface_();
}

static PLPath appDataPath = "";

const char *cmnGetAppDataDirectory( void )
{
	if ( *appDataPath != '\0' )
	{
		return appDataPath;
	}

	if ( PlGetApplicationDataDirectory( "yin", appDataPath, sizeof( appDataPath ) ) != NULL )
	{
		return appDataPath;
	}

	Warning( "Failed to fetch application data directory: %s\n", PlGetError() );

	snprintf( appDataPath, sizeof( appDataPath ), "." );
	return appDataPath;
}

NdBranch *cmnGetConfig( const char *name )
{
	// first attempt to load from local dir
	PLPath configPath;
	snprintf( configPath, sizeof( configPath ), "%s/%s.cfg.n", cmnGetAppDataDirectory(), name );
	NdBranch *root = ndLoadFile( configPath, "config" );
	if ( root != NULL )
	{
		return root;
	}

	// otherwise attempt to load from app data dir instead
	snprintf( configPath, sizeof( configPath ), "%s.cfg.n", name );
	root = ndLoadFile( configPath, "config" );
	if ( root == NULL )
	{
		Warning( "Failed to load user config file: %s\n"
		         "Creating empty config.\n",
		         ndGetErrorMessage() );
		root = ndPushBackObject( NULL, "config" );
	}

	return root;
}

bool cmnWriteConfig( struct NdBranch *root, const char *name )
{
	PLPath configPath;
	snprintf( configPath, sizeof( configPath ), "%s/%s.cfg.n", cmnGetAppDataDirectory(), name );
	ndWriteFile( configPath, root, ND_FILE_UTF8 );
	return true;
}
