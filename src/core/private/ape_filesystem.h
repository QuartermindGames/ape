// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

typedef struct NdBranch NdBranch;

const char *apeGetUserConfigLocation( void );

void apeSetupConfig( NdBranch *root );

void apeMountBaseLocations( void );
void apeMountLocations( void );
void apeClearMountedLocations( void );
