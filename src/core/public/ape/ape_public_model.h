// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

typedef struct ApeModelAnimationFrame ApeModelAnimationFrame;
typedef struct ApeModelAnimation      ApeModelAnimation;
typedef struct ApeModel               ApeModel;

typedef struct ApeModelAnimationState
{
	ApeModelAnimation *currentAnimation;
	ApeModelAnimation *oldAnimation;

	uint frame;
	uint oldFrame;
} ApeModelAnimationState;

/**
 * Load the specified model from disk.
 * Adds a new reference to the returned model.
 *
 * @param path 	Path of the model to load.
 * @return 		Pointer to instance of model, or null on fail.
 */
ApeModel *ape_model_load( const char *path );

/**
 * Release the reference to the model.
 *
 * @param model Pointer to instance of model.
 */
void ape_model_release( ApeModel *model );

/**
 * Draw the model.
 *
 * @param model Pointer to instance of model.
 * @param state	Current animation state of the model. Can be null if not animated.
 */
void ape_model_draw( ApeModel *model, const ApeModelAnimationState *state, const PLMatrix4 *transform, ApeLight *light );

/**
 * Draw an instanced version of the model.
 *
 * @param model 		Pointer to instance of model.
 * @param transforms 	List of transforms to use.
 * @param numTransforms Number of transforms in the list.
 */
void ape_model_draw_instanced( ApeModel *model, const PLMatrix4 **transforms, uint numTransforms );

PL_EXTERN_C_END
