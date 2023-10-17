// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "../ape_private.h"

#define ACL_MODEL_RFM_MAX_MESHES            64
#define ACL_MODEL_RFM_MAX_MATERIALS         32
#define ACL_MODEL_RFM_MAX_BONES             64// hard limit of 49 in RF
#define ACL_MODEL_RFM_MAX_COLLISION_SPHERES 64

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
	PLVector3 boundsMins;
	PLVector3 boundsMaxs;
	PLVector3 boundsOrigin;
	float boundsRadius;

	unsigned int flags;

	unsigned int numChunks;
} AclModelRfmMesh;

typedef struct AclModelRfcBone
{
	char name[ 24 ];
	PLQuaternion rotation;
	PLVector3 transform;
	struct AclModelRfcBone *parent;
} AclModelRfcBone;

typedef struct AclModelRfmCollisionSphere
{
	char name[ 24 ];
	float radius;
	PLVector3 transform;
	struct AclModelRfmCollisionSphere *parent;
} AclModelRfmCollisionSphere;

typedef struct AclModelRfc
{
	unsigned int numLods;

	unsigned int numMeshes;
	AclModelRfmMesh meshes[ ACL_MODEL_RFM_MAX_MESHES ];

	unsigned int numCollisionSpheres;
	AclModelRfmCollisionSphere collisionSpheres[ ACL_MODEL_RFM_MAX_COLLISION_SPHERES ];

	unsigned int numAttachments;

	unsigned int numMaterials;
	AclModelRfcMaterial materials[ ACL_MODEL_RFM_MAX_MATERIALS ];

	unsigned int numBones;
	AclModelRfcBone bones[ ACL_MODEL_RFM_MAX_BONES ];
	AclModelRfcBone *rootBone;
} AclModelRfm;

AclModelRfm *acl_model_rfm_parse_file_( PLFile *file );
AclModelRfm *acl_model_rfm_load_file_( const char *filename );
void acl_model_rfm_destroy_( AclModelRfm *model );
