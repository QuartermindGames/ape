// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Loader for PSI model format.
// Author:  Mark E. Sowden

#include "../cook.h"

#include "model.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct PsiHeader
{
	char magic[ 4 ];

	uint32_t version;
	uint32_t flags;

	char name[ 32 ];

	uint32_t numMeshes;
	uint32_t numVertices;

	uint32_t numPrimitives;
	uint32_t primOffset;

	uint16_t frameStart;
	uint16_t frameEnd;

	uint32_t numSegments;
	uint32_t segmentOffset;

	uint32_t numTextures;
	uint32_t textureOffset;

	uint32_t meshOffset;

	uint32_t radius;

	uint8_t padding[ 104 ];
} PsiHeader;

typedef struct PsiModel
{
	PsiHeader header;
} PsiModel;

static PsiModel *parse_psi( PLFile *file )
{
}

static PsiModel *load_psi( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == nullptr )
	{
		WARN( "Failed to open PSI (%s): %s\n", path, PlGetError() );
		return nullptr;
	}

	PsiHeader header = {};
	if ( PlReadFile( file, &header, sizeof( header ), 1 ) != 1 )
	{
		WARN( "Failed to read PSI: %s\n", PlGetError() );
		return nullptr;
	}

	PsiModel *model = PL_NEW( PsiModel );
	model->header = header;

	return model;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

CookModelFormatInterface modelPsiInterface = {
        "psi",
        .loadFunction = ( CookModelLoadFunction ) load_psi,
};
