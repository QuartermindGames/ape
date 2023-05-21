// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

typedef struct YNNodeBranch YNNodeBranch;

const char *FileSystem_GetUserConfigLocation( void );

void ogeFileSystem_SetupConfig( YNNodeBranch *root );

void YnCore_FileSystem_MountBaseLocations( void );
void ogeFileSystem_MountLocations( void );
void YnCore_FileSystem_ClearMountedLocations( void );
