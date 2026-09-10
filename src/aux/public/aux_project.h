// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "aux.h"

QM_OS_EXTERN_C

typedef struct AcmBranch AcmBranch;

AcmBranch *com_project_mount( const char *name );
void       com_project_unmount( void );

const char *com_project_get_local_path( void );
const char *com_project_get_base_name( void );
const char *com_project_get_name( void );

AcmBranch *com_project_get_config();

/**
 * Returns true if a project has been successfully mounted.
 */
bool aux_project_is_mounted();

QM_OS_EXTERN_C_END
