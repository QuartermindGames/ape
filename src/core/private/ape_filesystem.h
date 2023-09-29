// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

typedef struct NdBranch NdBranch;

const char *acl_get_user_config_location( void );

void acl_setup_config( NdBranch *root );

void apeMountBaseLocations( void );
