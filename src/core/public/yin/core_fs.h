// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "core.h"

PL_EXTERN_C

typedef struct AcmBranch AcmBranch;

const char *ape_fs_get_user_config_location( void );

void ape_fs_setup_config( AcmBranch *root );

/**
 * Returns contents of file in a null-terminated buffer.
 */
void *ape_fs_load_file_buffer( const char *path, size_t *outSize );

void ape_fs_mount_base_locations( void );

QmMathVector3f acl_fs_parse_vector( QmFsFile *file );

time_t ape_fs_get_timestamp( const char *path );

PL_EXTERN_C_END
