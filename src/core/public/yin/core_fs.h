// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "core.h"

PL_EXTERN_C

typedef struct NdBranch NdBranch;

const char *acl_get_user_config_location( void );

void acl_setup_config( NdBranch *root );

void acl_fs_mount_base_locations( void );

char *acl_fs_parse_string( PLFile *file, uint16_t *size );
char *acl_fs_parse_string_ex( PLFile *file, uint16_t *size, unsigned int version, unsigned int minVersion, unsigned int maxVersion );

uint8_t acl_fs_parse_byte( PLFile *file );
uint8_t acl_fs_parse_byte_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, uint8_t fallback );

char *acl_fs_parse_string( PLFile *file, uint16_t *size );
char *acl_fs_parse_string_ex( PLFile *file, uint16_t *size, unsigned int version, unsigned int minVersion, unsigned int maxVersion );
PLColour acl_fs_parse_colour( PLFile *file );
PLMatrix3 acl_fs_parse_mat3( PLFile *file );

PLVector3 acl_fs_parse_vector( PLFile *file );
PLVector3 acl_fs_parse_vector_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const PLVector3 *fallback );

PLVector4 acl_fs_parse_vector4( PLFile *file );
PLVector4 acl_fs_parse_vector4_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, const PLVector4 *fallback );

int acl_fs_parse_int( PLFile *file );
int acl_fs_parse_int_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, int fallback );

float acl_fs_parse_float( PLFile *file );
float acl_fs_parse_float_ex( PLFile *file, unsigned int version, unsigned int minVersion, unsigned int maxVersion, float fallback );

PL_EXTERN_C_END
