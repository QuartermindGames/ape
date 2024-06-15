// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

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

#if !defined( NDEBUG )
#	define DPRINT( ... ) printf( __VA_ARGS__ )
#else
#	define DPRINT( ... )
#endif

typedef struct CookState
{
	const char *projectName;
} CookState;
extern CookState cook_state;

#define COOK_WORLD_EXTENSION "cwf.n"
#define COOK_MODEL_EXTENSION "cmf.n"

struct CookModel;
typedef struct CookModel CookModel;

typedef CookModel *( *CookModelLoadFunction )( const char *path );
typedef ApeFormatModel *( *CookModelConvertFunction )( const CookModel *model, ApeFormatModel *out );
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
