// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

#define SS_APE_MODEL_MAX_MATERIALS 32
#define SS_APE_MODEL_MAX_BONES     256

typedef struct PLHashTableNode PLHashTableNode;

typedef struct SSApeModelBone
{
	char name[ 64 ];
	unsigned int parent;
	PLVector3 position;
	PLQuaternion orientation;
} SSApeModelBone;

typedef struct SSApeModel
{
	ApeMaterial *materials[ SS_APE_MODEL_MAX_MATERIALS ];
	unsigned int numMaterials;

	PLGMesh *meshes[ SS_APE_MODEL_MAX_MATERIALS ];
	unsigned int numMeshes;

	SSApeModelBone bones[ SS_APE_MODEL_MAX_BONES ];
	SSApeModelBone *rootBone;
	unsigned int numBones;

	PLCollisionSphere visSphere;

	PLHashTableNode *node;

	ApeMemoryReference mem;
} SSApeModel;

SSApeModel *ss_ape_model_load( const char *path );
void ss_ape_model_release( SSApeModel *model );

PL_EXTERN_C_END
