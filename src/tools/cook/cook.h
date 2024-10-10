// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_math.h>

#include "common.h"
#include "common_project.h"

#include "acm/public/acm/acm.h"
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

typedef struct CookModel CookModel;

typedef CookModel *( *CookModelLoadFunction )( const char *path );
typedef CookModel *( *CookModelConvertFunction )( const CookModel *model, CookModel *out );
typedef void ( *CookModelDeleteFunction )( CookModel *model );

typedef struct CookModelFormatInterface
{
	const char              *extension;
	CookModelLoadFunction    loadFunction;
	CookModelConvertFunction convertFunction;
	CookModelDeleteFunction  deleteFunction;
} CookModelFormatInterface;

void cook_world_process( const char *worldName );
void cook_model_process( const char *modelName );
