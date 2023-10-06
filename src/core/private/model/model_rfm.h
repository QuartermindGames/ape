// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "../ape_private.h"

#define ACL_MODEL_RFC_MAX_MESHES    64
#define ACL_MODEL_RFC_MAX_MATERIALS 32

#define ACL_MODEL_RFX_FLAG_TRANSPARENT 8

typedef struct AclModelRfcMaterial
{
	char diffuseTexture[ 32 ];
	char reflectionTexture[ 32 ];

	float specular;
	float gloss;
	float reflectivity;
	float illumination;

	unsigned int flags;
} AclModelRfcMaterial;

typedef struct AclModelRfcMesh
{

} AclModelRfcMesh;

typedef struct AclModelRfc
{
	unsigned int numLods;

	unsigned int numMeshes;
	AclModelRfcMesh meshes[ ACL_MODEL_RFC_MAX_MESHES ];

	unsigned int numCollisionSpheres;
	unsigned int numAttachments;

	unsigned int numMaterials;
	AclModelRfcMaterial materials[ ACL_MODEL_RFC_MAX_MATERIALS ];
} AclModelRfm;

AclModelRfm *acl_model_rfm_parse_file_( PLFile *file );
AclModelRfm *acl_model_rfm_load_file_( const char *filename );
