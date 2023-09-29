// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "../ape_private.h"

typedef struct AclModelRfc
{

} AclModelRfc;

AclModelRfc *acl_model_rfc_parse_file( PLFile *file );
AclModelRfc *acl_model_rfc_load_file( const char *filename );
