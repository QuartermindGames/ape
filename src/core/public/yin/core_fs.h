// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "core.h"

PL_EXTERN_C

typedef struct AcmBranch AcmBranch;

const char *ss_acl_fs_get_user_config_location( void );

void ape_fs_setup_config( AcmBranch *root );

/**
 * Returns contents of file in a null-terminated buffer.
 */
void *ss_acl_fs_load_file_buffer( const char *path, size_t *outSize );

void ape_fs_mount_base_locations( void );

char *ss_acl_fs_parse_string( PLFile *file, uint16_t *size );
char *ss_acl_fs_parse_string_ex( PLFile *file, uint16_t *size, unsigned int version, unsigned int minVersion, unsigned int maxVersion );

uint8_t ss_acl_fs_parse_byte( PLFile *file );
uint8_t ss_acl_fs_parse_byte_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, uint8_t fallback );

QmMathVector3f acl_fs_parse_vector( PLFile *file );
QmMathVector3f acl_fs_parse_vector_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const QmMathVector3f *fallback );

QmMathVector4f ss_acl_fs_parse_vector4( PLFile *file );
QmMathVector4f ss_acl_fs_parse_vector4_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const QmMathVector4f *fallback );

int ss_acl_fs_parse_int( PLFile *file );
int ss_acl_fs_parse_int_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, int fallback );

float acl_fs_parse_float_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, float fallback );
QmMathColour4ub ss_acl_fs_parse_colour( PLFile *file );
PLMatrix3 ss_acl_fs_parse_mat3( PLFile *file );
QmMathVector3f ss_acl_fs_parse_vector( PLFile *file );
float ss_acl_fs_parse_float( PLFile *file );

PL_EXTERN_C_END
