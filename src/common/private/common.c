// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>
#include <plcore/pl_console.h>

#include <yin/node.h>

#include "common_private.h"

int logLevelPrint;
int logLevelWarn;

void comInitialize( void ) {
	logLevelPrint = PlAddLogLevel( "common", PL_COLOUR_WHITE, true );
	logLevelWarn = PlAddLogLevel( "common/warning", PL_COLOUR_YELLOW, true );

	Message( "Common Library initialized\n" );

	ndSetupLogs();

	comRegisterPkgInterface_();
	comRegisterVppInterface_();
}

const char *comGetDataDirectory( void ) {
	// cache it
	static PLPath dataPath = { '\0' };
	if ( *dataPath != '\0' ) {
		return dataPath;
	}

	PLPath exeDir;
	if ( PlGetExecutableDirectory( exeDir, sizeof( exeDir ) ) != NULL ) {
		PlSetupPath( dataPath, true, "%s/../../runtime", exeDir );
		if ( PlPathExists( dataPath ) ) {
			PlSetupPath( dataPath, true, "%s/../..", exeDir );
			return dataPath;
		}

		PlSetupPath( dataPath, true, "%s", exeDir );
		return dataPath;
	}

	// oh dear oh dear...

	const char *cwd = PlGetWorkingDirectory();
	PlSetupPath( dataPath, true, "%s/../../runtime", cwd );
	if ( PlPathExists( dataPath ) ) {
		PlSetupPath( dataPath, true, "%s/../..", cwd );
	} else {
		PlSetupPath( dataPath, true, "%s", cwd );
	}

	return dataPath;
}

const char *comGetAppDataDirectory( void ) {
	static PLPath appDataPath = "";
	if ( *appDataPath != '\0' ) {
		return appDataPath;
	}

	if ( PlGetApplicationDataDirectory( "ape", appDataPath, sizeof( appDataPath ) ) != NULL ) {
		return appDataPath;
	}

	Warning( "Failed to fetch application data directory: %s\n", PlGetError() );

	snprintf( appDataPath, sizeof( appDataPath ), "." );
	return appDataPath;
}

NdBranch *comGetConfig( const char *name ) {
	// first attempt to load from local dir
	PLPath configPath;
	snprintf( configPath, sizeof( configPath ), "%s/%s.cfg.n", comGetAppDataDirectory(), name );
	NdBranch *root = ndLoadFile( configPath, "config" );
	if ( root != NULL ) {
		return root;
	}

	// otherwise attempt to load from app data dir instead
	snprintf( configPath, sizeof( configPath ), "%s.cfg.n", name );
	root = ndLoadFile( configPath, "config" );
	if ( root == NULL ) {
		Warning( "Failed to load user config file: %s\n"
		         "Creating empty config.\n",
		         ndGetErrorMessage() );
		root = ndPushBackObject( NULL, "config" );
	}

	return root;
}

bool comWriteConfig( struct NdBranch *root, const char *name ) {
	PLPath configPath;
	snprintf( configPath, sizeof( configPath ), "%s/%s.cfg.n", comGetAppDataDirectory(), name );
	ndWriteFile( configPath, root, ND_FILE_UTF8 );
	return true;
}
