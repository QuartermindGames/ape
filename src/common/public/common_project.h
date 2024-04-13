// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "common.h"

PL_EXTERN_C

typedef struct NdBranch NdBranch;

NdBranch *com_project_mount( const char *name );
void com_project_unmount( void );

const char *com_project_get_local_path( void );
const char *com_project_get_base_name( void );
const char *com_project_get_name( void );

NdBranch *com_project_get_config();

PL_EXTERN_C_END
