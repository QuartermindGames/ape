// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "core.h"

PL_EXTERN_C

typedef struct NdBranch NdBranch;

const char *acl_get_user_config_location( void );

void acl_setup_config( NdBranch *root );

void apeMountBaseLocations( void );

PLColour acl_fs_parse_colour( PLFile *file );
PLMatrix3 acl_fs_parse_mat3( PLFile *file );
PLVector3 acl_fs_parse_vector( PLFile *file );
float acl_fs_parse_float( PLFile *file );
char *acl_fs_parse_string( PLFile *file, uint16_t *size );

PL_EXTERN_C_END
