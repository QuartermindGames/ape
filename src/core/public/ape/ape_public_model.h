// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

PL_EXTERN_C

typedef struct IOModelAnimationFrame IOModelAnimationFrame;
typedef struct IOModelAnimation      IOModelAnimation;
typedef struct ApeModel              ApeModel;

typedef struct ApeModelAnimationState
{
	IOModelAnimation *currentAnimation;
	IOModelAnimation *oldAnimation;

	unsigned int frame;
	unsigned int oldFrame;
} ApeModelAnimationState;

APE_MEMORY_IMPLEMENT_INTERFACE_DECL( ape_model, ApeModel )

/**
 * Load the specified model from disk.
 * Adds a new reference to the returned model.
 */
ApeModel *ape_model_load( const char *path );

/**
 * Draw the model.
 */
void ape_model_draw( const ApeModel *model, const ApeModelAnimationState *state, const PLMatrix4 *transform, const ApeRendererPassState *passState );

/**
 * Draw an instanced version of the model.
 */
void ape_model_draw_instanced( ApeModel *model, const PLMatrix4 **transforms, unsigned int numTransforms );

/**
 * If the model doesn't have any animations, it's assumed to be static. Mind in this
 * case, the model has the potential to cast shadows in the environment, which may
 * not be desired!
 */
bool ape_model_is_static( const ApeModel *model );

/////////////////////////////////////////////////////////////////////////////////////
// Model World Node Class
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeModelNode ApeModelNode;

/**
 * Create a model node instance.
 */
ApeModelNode *ape_model_node_create( ApeWorldNode *parent, const char *name, const char *path );

/////////////////////////////////////////////////////////////////////////////////////

void ape_model_compute_models_lighting( double delta );

PL_EXTERN_C_END
