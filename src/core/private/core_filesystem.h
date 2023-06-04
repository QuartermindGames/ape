// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

typedef struct NdBranch NdBranch;

const char *FileSystem_GetUserConfigLocation( void );

void ogeFileSystem_SetupConfig( NdBranch *root );

void apeMountBaseLocations( void );
void apeMountLocations( void );
void ogeFileSystem_ClearMountedLocations( void );
