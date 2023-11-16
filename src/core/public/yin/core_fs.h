// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "core.h"

PL_EXTERN_C

typedef struct NdBranch NdBranch;

const char *ss_acl_fs_get_user_config_location( void );

void ss_acl_fs_setup_config( NdBranch *root );

void ss_acl_fs_mount_base_locations( void );

PLColour ss_acl_fs_parse_colour( PLFile *file );
PLMatrix3 ss_acl_fs_parse_mat3( PLFile *file );
PLVector3 ss_acl_fs_parse_vector( PLFile *file );
float ss_acl_fs_parse_float( PLFile *file );
char *ss_acl_fs_parse_string( PLFile *file, uint16_t *size );

PL_EXTERN_C_END
