/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "core_private.h"
#include "core_filesystem.h"

#include <yin/node.h>

/****************************************
 * PRIVATE
 ****************************************/

static YNNodeBranch *fileSystemConfig;

#define MAX_FILESYSTEM_MOUNTS 255
static PLFileSystemMount *fileSystemMounts[ MAX_FILESYSTEM_MOUNTS ];
static unsigned int       numMountedLocations = 0;

static void ParseMountConfig( YNNodeBranch *root )
{
	unsigned int numChildren = YnNode_GetNumOfChildren( root );
	if ( numChildren == 0 )
	{ /* nothing to mount, okay then */
		return;
	}

	YNNodeBranch *child = YnNode_GetFirstChild( root );
	if ( YnNode_GetType( child ) != YN_NODE_PROP_STR )
	{
		PRINT_WARNING( "Invalid child type found in config!\n" );
		return;
	}

	for ( unsigned int i = 0; i < numChildren; ++i )
	{
		PLPath path;
		YnNode_GetStr( child, path, sizeof( PLPath ) );
		child = YnNode_GetNextChild( child );

		if ( ( fileSystemMounts[ numMountedLocations ] = PlMountLocation( path ) ) == NULL )
		{
			PRINT_WARNING( "Failed to mount \"%s\": %s\n", path, PlGetError() );
			continue;
		}

		numMountedLocations++;
	}
}

static const char *GetDataDirectory( void )
{
	static PLPath dataPath = { '\0' };
	if ( *dataPath != '\0' )
		return dataPath;

	if ( PlLocalFileExists( "./" ENGINE_BASE_CONFIG ) )
	{
		snprintf( dataPath, sizeof( dataPath ), "./" );
		return dataPath;
	}

	snprintf( dataPath, sizeof( dataPath ), "../../" );
	return dataPath;
}

static void ParseAliases( YNNodeBranch *root )
{
	unsigned int numAliases = YnNode_GetNumOfChildren( root ) / 2;
	if ( numAliases == 0 )
	{
		return;
	}

	YNNodeBranch *child = YnNode_GetFirstChild( root );
	if ( YnNode_GetType( child ) != YN_NODE_PROP_STR )
	{
		PRINT_WARNING( "Invalid child type found in config!\n" );
		return;
	}

	for ( unsigned int i = 0; i < numAliases; i++ )
	{
		PLPath aliasPath;
		YnNode_GetStr( child, aliasPath, sizeof( PLPath ) );
		child = YnNode_GetNextChild( child );
		if ( child == NULL )
		{
			PRINT_WARNING( "Encountered alias with no path: %u\n", i );
			break;
		}

		PLPath targetPath;
		YnNode_GetStr( child, targetPath, sizeof( PLPath ) );

		PlAddFileAlias( aliasPath, targetPath );
		PRINT( "Registered alias: \"%s\" > \"%s\"\n", aliasPath, targetPath );

		child = YnNode_GetNextChild( child );
	}
}

#define USER_CONFIG "user" YN_NODE_DEFAULT_EXTENSION
static char configPath[ PL_SYSTEM_MAX_PATH ] = { '\0' };

/****************************************
 * PUBLIC
 ****************************************/

const char *FileSystem_GetUserConfigLocation( void )
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

void ogeFileSystem_SetupConfig( YNNodeBranch *root )
{
	PlClearFileAliases();

	YnCore_FileSystem_ClearMountedLocations();

	fileSystemConfig = YnNode_GetChildByName( root, "fileSystem" );
	if ( fileSystemConfig == NULL )
	{
		// If it's not found, push it on
		fileSystemConfig = YnNode_PushBackObject( root, "fileSystem" );
		return;
	}

	YNNodeBranch *child;
	if ( ( child = YnNode_GetChildByName( fileSystemConfig, "aliases" ) ) != NULL )
		ParseAliases( child );
}

void YnCore_FileSystem_MountBaseLocations( void )
{
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) == NULL )
	{
		snprintf( exePath, sizeof( exePath ), "./" );
		PRINT_WARNING( "Failed to get executable directory, using fallback!\n" );
	}

	PlMountLocalLocation( exePath );
	PlMountLocalLocation( GetDataDirectory() );
}

void ogeFileSystem_MountLocations( void )
{
	PlRegisterStandardPackageLoaders();

	PL_ZERO( fileSystemMounts, sizeof( PLFileSystemMount * ) * MAX_FILESYSTEM_MOUNTS );

	/* now attempt to load in the mount config file, and mount */
	YNNodeBranch *mountRoot = YnNode_GetChildByName( fileSystemConfig, "mountLocations" );
	if ( mountRoot != NULL )
	{
		ParseMountConfig( mountRoot );
		YnNode_DestroyBranch( mountRoot );
	}

	/* mount any packages under each of our mounted locations */
	for ( unsigned int i = 0; i < numMountedLocations; ++i )
	{
		if ( PlGetMountLocationType( fileSystemMounts[ i ] ) != PL_FS_MOUNT_DIR )
			continue;

		const char *mountPath = PlGetMountLocationPath( fileSystemMounts[ i ] );
		if ( mountPath == NULL )
			continue;

		/* attempt to automatically mount any packages */
		unsigned int max = ( MAX_FILESYSTEM_MOUNTS - numMountedLocations ) - 1;
		for ( unsigned int j = 0; j < max; ++j )
		{
			PLPath pkgPath;
			snprintf( pkgPath, sizeof( pkgPath ), "%s/data%d.pkg", mountPath, j );
			if ( PlFileExists( pkgPath ) && ( fileSystemMounts[ j ] = PlMountLocalLocation( pkgPath ) ) != NULL )
				continue;

			break;
		}
	}

	/* now attempt to mount any base packages */
	unsigned int max = ( MAX_FILESYSTEM_MOUNTS - numMountedLocations ) - 1;
	for ( unsigned int i = 0; i < max; ++i )
	{
		PLPath pkgPath;
		snprintf( pkgPath, sizeof( pkgPath ), "base%d.pkg", i );
		if ( PlFileExists( pkgPath ) && ( fileSystemMounts[ i ] = PlMountLocation( pkgPath ) ) != NULL )
			continue;

		break;
	}
}

void YnCore_FileSystem_ClearMountedLocations( void )
{
	for ( unsigned int i = 0; i < numMountedLocations; ++i )
	{
		if ( fileSystemMounts[ i ] == NULL )
			continue;

		PlClearMountedLocation( fileSystemMounts[ i ] );
		fileSystemMounts[ i ] = NULL;
	}
	numMountedLocations = 0;
}
