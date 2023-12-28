// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl.h>
#include <plmodel/plm.h>

#include "common.h"
#include "common_project.h"

#include "yin/node.h"
#include "ape/ape_formats.h"

#define WARN( ... ) \
	printf( "WARNING: " __VA_ARGS__ );
#define ERROR( ... )           \
	{                          \
		printf( __VA_ARGS__ ); \
		exit( EXIT_FAILURE );  \
	}

typedef struct CookState
{
	const char *projectName;
} CookState;
extern CookState cook_state;

struct CookModel;
typedef struct CookModel CookModel;

typedef CookModel *( *CookModelLoadFunction )( const char *path );
typedef SSApeFormatModel *( *CookModelConvertFunction )( const CookModel *model, SSApeFormatModel *out );
typedef void ( *CookModelDeleteFunction )( CookModel *model );

typedef struct CookModelFormatInterface
{
	const char *extension;
	CookModelLoadFunction loadFunction;
	CookModelConvertFunction convertFunction;
	CookModelDeleteFunction deleteFunction;
} CookModelFormatInterface;

void cook_world_process( const char *worldName );
void cook_model_process( const char *modelName );
