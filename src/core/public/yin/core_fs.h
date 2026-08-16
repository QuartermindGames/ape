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

char *ape_fs_parse_string( QmFsFile *file, uint16_t *size );
char *ape_fs_parse_string_ex( QmFsFile *file, uint16_t *size, unsigned int version, unsigned int minVersion, unsigned int maxVersion );

uint8_t ape_fs_parse_byte( QmFsFile *file );
uint8_t ss_acl_fs_parse_byte_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, uint8_t fallback );

QmMathVector3f acl_fs_parse_vector( QmFsFile *file );
QmMathVector3f acl_fs_parse_vector_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const QmMathVector3f *fallback );

QmMathVector4f ss_acl_fs_parse_vector4( QmFsFile *file );
QmMathVector4f ss_acl_fs_parse_vector4_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const QmMathVector4f *fallback );

int ss_acl_fs_parse_int( QmFsFile *file );
int ss_acl_fs_parse_int_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, int fallback );

float acl_fs_parse_float_ex( QmFsFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, float fallback );
QmMathColour4ub ss_acl_fs_parse_colour( QmFsFile *file );
PLMatrix3 ss_acl_fs_parse_mat3( QmFsFile *file );
QmMathVector3f ss_acl_fs_parse_vector( QmFsFile *file );
float ss_acl_fs_parse_float( QmFsFile *file );

time_t ape_fs_get_timestamp( const char *path );

PL_EXTERN_C_END
